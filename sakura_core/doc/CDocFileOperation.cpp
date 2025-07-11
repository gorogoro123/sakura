﻿/*! @file */
/*
	Copyright (C) 2008, kobake
	Copyright (C) 2018-2022, Sakura Editor Organization

	SPDX-License-Identifier: Zlib
*/
#include "StdAfx.h"
#include "CDocFileOperation.h"
#include "CDocVisitor.h"
#include "CEditDoc.h"

#include "recent/CMRUFile.h"
#include "recent/CMRUFolder.h"
#include "dlg/CDlgOpenFile.h"
#include "_main/CAppMode.h" 
#include "_main/CControlTray.h"
#include "CEditApp.h"
#include "window/CEditWnd.h"
#include "uiparts/CWaitCursor.h"
#include "util/window.h"
#include "env/DLLSHAREDATA.h"
#include "env/CSakuraEnvironment.h"
#include "plugin/CPlugin.h"
#include "plugin/CJackManager.h"
#include "util/format.h"
#include "CSelectLang.h"
#include "String_define.h"

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                          ロック                             //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

bool CDocFileOperation::_ToDoLock() const
{
	// ファイルを開いていない
	if( !m_pcDocRef->m_cDocFile.GetFilePathClass().IsValidPath() )return false;

	// ビューモード
	if( CAppMode::getInstance()->IsViewMode() )return false;

	// 排他設定
	if( GetDllShareData().m_Common.m_sFile.m_nFileShareMode == SHAREMODE_NOT_EXCLUSIVE )return false;

	return true;
}

void CDocFileOperation::DoFileLock(bool bMsg)
{
	if(this->_ToDoLock()){
		m_pcDocRef->m_cDocFile.FileLock(GetDllShareData().m_Common.m_sFile.m_nFileShareMode, bMsg);
	}
}

void CDocFileOperation::DoFileUnlock()
{
	m_pcDocRef->m_cDocFile.FileUnlock();
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                         ロードUI                            //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

/* 「ファイルを開く」ダイアログ */
//	Mar. 30, 2003 genta	ファイル名未定時の初期ディレクトリをカレントフォルダーに
bool CDocFileOperation::OpenFileDialog(
	HWND				hwndParent,		//!< [in]
	const WCHAR*		pszOpenFolder,	//!< [in]     NULL以外を指定すると初期フォルダーを指定できる
	SLoadInfo*			pLoadInfo,		//!< [in,out] ロード情報
	std::vector<std::wstring>&	files
)
{
	/* アクティブにする */
	ActivateFrameWindow( hwndParent );

	// ファイルオープンダイアログを表示
	CDlgOpenFile cDlgOpenFile;
	cDlgOpenFile.Create(
		G_AppInstance(),
		hwndParent,
		L"*.*",
		pszOpenFolder ? pszOpenFolder : CSakuraEnvironment::GetDlgInitialDir().c_str(),	// 初期フォルダー
		CMRUFile().GetPathList(),															// MRUリストのファイルのリスト
		CMRUFolder().GetPathList()														// OPENFOLDERリストのファイルのリスト
	);
	return cDlgOpenFile.DoModalOpenDlg( pLoadInfo, &files );
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                       ロードフロー                          //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

bool CDocFileOperation::DoLoadFlow(SLoadInfo* pLoadInfo)
{
	ELoadResult eLoadResult = LOADED_FAILURE;

	try{
		//ロード前チェック
		if(CALLBACK_INTERRUPT==m_pcDocRef->NotifyCheckLoad(pLoadInfo))throw CFlowInterruption();

		//ロード処理
		m_pcDocRef->NotifyBeforeLoad(pLoadInfo);			//前処理
		eLoadResult = m_pcDocRef->NotifyLoad(*pLoadInfo);	//本処理
		m_pcDocRef->NotifyAfterLoad(*pLoadInfo);			//後処理
	}
	catch(const CFlowInterruption&){
		eLoadResult = LOADED_INTERRUPT;
	}
	catch(...){
		//予期せぬ例外が発生した場合も NotifyFinalLoad は必ず呼ぶ！
		m_pcDocRef->NotifyFinalLoad(LOADED_FAILURE);
		throw;
	}

	//最終処理
	m_pcDocRef->NotifyFinalLoad(eLoadResult);

	return eLoadResult==LOADED_OK;
}

//! ファイルを開く
bool CDocFileOperation::FileLoad(
	SLoadInfo*	pLoadInfo		//!< [in,out]
)
{
	bool bRet = DoLoadFlow(pLoadInfo);
	// 2006.09.01 ryoji オープン後自動実行マクロを実行する
	if( bRet ){
		m_pcDocRef->RunAutoMacro( GetDllShareData().m_Common.m_sMacro.m_nMacroOnOpened );

		//プラグイン：DocumentOpenイベント実行
		CJackManager::getInstance()->InvokePlugins( PP_DOCUMENT_OPEN, &GetEditWnd().GetActiveView() );
	}
	return bRet;
}

//! ファイルを開く（自動実行マクロを実行しない）
//	2009.08.11 ryoji FileLoadへのパラメータ追加にしてもいいがANSI版と整合がとりやすいので当面は別関数にしておく
bool CDocFileOperation::FileLoadWithoutAutoMacro(
	SLoadInfo*	pLoadInfo		//!< [in,out]
)
{
	return DoLoadFlow(pLoadInfo);
}

//! 同一ファイルの再オープン
void CDocFileOperation::ReloadCurrentFile(
	ECodeType	nCharCode		//!< [in] 文字コード種別
)
{
	//プラグイン：DocumentCloseイベント実行
	CJackManager::getInstance()->InvokePlugins( PP_DOCUMENT_CLOSE, &GetEditWnd().GetActiveView() );

	if( !fexist(m_pcDocRef->m_cDocFile.GetFilePath()) ){
		/* ファイルが存在しない */
		//	Jul. 26, 2003 ryoji BOMを標準設定に	// IsBomDefOn使用 2013/5/17	Uchi
		m_pcDocRef->m_cDocFile.SetCodeSet( nCharCode,  CCodeTypeName( nCharCode ).IsBomDefOn() );
		// カーソル位置表示を更新する	// 2008.07.22 ryoji
		GetEditWnd().GetActiveView().GetCaret().ShowCaretPosInfo();
		return;
	}

	//カーソル位置保存
	CLayoutPoint	ptCaretPosXY = GetEditWnd().GetActiveView().GetCaret().GetCaretLayoutPos();

	//ロード
	SLoadInfo sLoadInfo;
	sLoadInfo.cFilePath=m_pcDocRef->m_cDocFile.GetFilePath();
	sLoadInfo.eCharCode=nCharCode;
	sLoadInfo.bViewMode=CAppMode::getInstance()->IsViewMode(); // 2014.06.13 IsEditable->IsViewModeに戻す。かわりに bForceNoMsgを追加
	sLoadInfo.bWritableNoMsg = !m_pcDocRef->IsEditable(); // すでに編集できない状態ならファイルロックのメッセージを表示しない
	sLoadInfo.bRequestReload=true;
	bool bRet = this->DoLoadFlow(&sLoadInfo);

	// カーソル位置復元 (※ここではオプションのカーソル位置復元（＝改行単位）が指定されていない場合でも復元する)
	// 2007.08.23 ryoji 表示領域復元
	if( ptCaretPosXY.GetY2() < m_pcDocRef->m_cLayoutMgr.GetLineCount() ){
		CLayoutInt nViewTopLine = GetEditWnd().GetActiveView().GetTextArea().GetViewTopLine();	/* 表示域の一番上の行(0開始) */
		CLayoutInt nViewLeftCol = GetEditWnd().GetActiveView().GetTextArea().GetViewLeftCol();	/* 表示域の一番左の桁(0開始) */
		GetEditWnd().GetActiveView().GetTextArea().SetViewTopLine(nViewTopLine);
		GetEditWnd().GetActiveView().GetTextArea().SetViewLeftCol(nViewLeftCol);
	}
	GetEditWnd().GetActiveView().GetCaret().MoveCursorProperly( ptCaretPosXY, true );	// 2007.08.23 ryoji MoveCursor()->MoveCursorProperly()
	GetEditWnd().GetActiveView().GetCaret().m_nCaretPosX_Prev = GetEditWnd().GetActiveView().GetCaret().GetCaretLayoutPos().GetX2();

	// 2006.09.01 ryoji オープン後自動実行マクロを実行する
	if( bRet ){
		m_pcDocRef->RunAutoMacro( GetDllShareData().m_Common.m_sMacro.m_nMacroOnOpened );

		//プラグイン：DocumentOpenイベント実行
		CJackManager::getInstance()->InvokePlugins( PP_DOCUMENT_OPEN, &GetEditWnd().GetActiveView() );
	}
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                         セーブUI                            //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

/*! 「ファイル名を付けて保存」ダイアログ
	@date 2001.02.09 genta	改行コードを示す引数追加
	@date 2003.03.30 genta	ファイル名未定時の初期ディレクトリをカレントフォルダーに
	@date 2003.07.20 ryoji	BOMの有無を示す引数追加
	@date 2006.11.10 ryoji	ユーザー指定の拡張子を状況依存で変化させる
*/
bool CDocFileOperation::SaveFileDialog(
	SSaveInfo*	pSaveInfo	//!< [out]
)
{
	//拡張子指定
	// 一時適用や拡張子なしの場合の拡張子をタイプ別設定から持ってくる
	// 2008/6/14 大きく改造 Uchi
	std::wstring strDefaultWildCard;	// ユーザー指定拡張子
	
	{
		LPCWSTR	szExt;

		const STypeConfig& type = m_pcDocRef->m_cDocType.GetDocumentAttribute();
		//ファイルパスが無い場合は *.txt とする
		if(!this->m_pcDocRef->m_cDocFile.GetFilePathClass().IsValidPath()){
			szExt = L"";
		}
		else {
			szExt = this->m_pcDocRef->m_cDocFile.GetFilePathClass().GetExt();
		}
		if (type.m_nIdx == 0) {
			// 基本
			if (szExt[0] == L'\0') { 
				// ファイルパスが無いまたは拡張子なし
				strDefaultWildCard = L"*.txt";
			}
			else {
				// 拡張子あり
				strDefaultWildCard = L"*";
				strDefaultWildCard += szExt;
			}
		}
		else {
			strDefaultWildCard = CDocTypeManager::ConvertTypesExtToDlgExt(type.m_szTypeExts, szExt);
		}

		if(!this->m_pcDocRef->m_cDocFile.GetFilePathClass().IsValidPath()){
			//「新規から保存時は全ファイル表示」オプション	// 2008/6/15 バグフィックス Uchi
			if( GetDllShareData().m_Common.m_sFile.m_bNoFilterSaveNew )
				strDefaultWildCard += L";*.*";	// 全ファイル表示
		}
		else {
			//「新規以外から保存時は全ファイル表示」オプション
			if( GetDllShareData().m_Common.m_sFile.m_bNoFilterSaveFile )
				strDefaultWildCard += L";*.*";	// 全ファイル表示
		}
	}

	// 無題の時は日時を付ける
	if( pSaveInfo->cFilePath[0] == L'\0' ){
		SYSTEMTIME localTime = {};
		::GetLocalTime( &localTime );
		auto dateTimeString = GetDateTimeFormat( L"_%Y%m%d_%H%M%S", localTime );
		const EditNode* node = CAppNodeManager::getInstance()->GetEditNode( GetEditWnd().GetHwnd() );
		const int nId = (node != nullptr && 0 < node->m_nId) ? node->m_nId : 0;
		auto_snprintf_s( pSaveInfo->cFilePath, pSaveInfo->cFilePath.GetBufferCount(), L"%s%.0d%s", LS(STR_NO_TITLE2), nId, dateTimeString.c_str() );
	}

	// ダイアログを表示
	CDlgOpenFile cDlgOpenFile;
	cDlgOpenFile.Create(
		G_AppInstance(),
		CEditWnd::getInstance()->GetHwnd(),
		strDefaultWildCard.c_str(),
		CSakuraEnvironment::GetDlgInitialDir().c_str(),	// 初期フォルダー
		CMRUFile().GetPathList(),		//	最近のファイル
		CMRUFolder().GetPathList()	//	最近のフォルダー
	);
	return cDlgOpenFile.DoModalSaveDlg( pSaveInfo, pSaveInfo->eCharCode == CODE_CODEMAX );
}

//!「ファイル名を付けて保存」ダイアログ
bool CDocFileOperation::SaveFileDialog(LPWSTR szPath)
{
	SSaveInfo sSaveInfo;
	sSaveInfo.cFilePath = szPath;
	sSaveInfo.eCharCode = CODE_CODEMAX; //###トリッキー
	bool bRet = SaveFileDialog(&sSaveInfo);
	wcscpy_s(szPath, _MAX_PATH, sSaveInfo.cFilePath);
	return bRet;
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                       セーブフロー                          //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

bool CDocFileOperation::DoSaveFlow(SSaveInfo* pSaveInfo)
{
	ESaveResult eSaveResult = SAVED_FAILURE;

	try{
		//オプション：無変更でも上書きするか
		// 2009.04.12 ryoji CSaveAgent::OnCheckSave()から移動
		// ### 無変更なら上書きしないで抜ける処理はどの CDocListener の OnCheckSave() よりも前に
		// ### （保存するかどうか問い合わせたりするよりも前に）やるぺきことなので、
		// ### スマートじゃない？かもしれないけど、とりあえずここに配置しておく
		if( !GetDllShareData().m_Common.m_sFile.m_bEnableUnmodifiedOverwrite ){
			// 上書きの場合
			if(pSaveInfo->bOverwriteMode){
				// 無変更の場合は警告音を出し、終了
				if (!m_pcDocRef->m_cDocEditor.IsModified() &&
					pSaveInfo->cEol.IsNone() &&	//※改行コード指定保存がリクエストされた場合は、「変更があったもの」とみなす
					!pSaveInfo->bChgCodeSet) {		// 文字コードセットの変更が有った場合は、「変更があったもの」とみなす
					CEditApp::getInstance()->m_cSoundSet.NeedlessToSaveBeep();
					throw CFlowInterruption();
				}
			}
		}

		//セーブ前チェック
		if(CALLBACK_INTERRUPT==m_pcDocRef->NotifyCheckSave(pSaveInfo))throw CFlowInterruption();

		//セーブ前おまけ処理
		if(CALLBACK_INTERRUPT==m_pcDocRef->NotifyPreBeforeSave(pSaveInfo))throw CFlowInterruption();

		// 2006.09.01 ryoji 保存前自動実行マクロを実行する
		m_pcDocRef->RunAutoMacro( GetDllShareData().m_Common.m_sMacro.m_nMacroOnSave, pSaveInfo->cFilePath );

		//プラグイン：DocumentBeforeSaveイベント実行
		CJackManager::getInstance()->InvokePlugins( PP_DOCUMENT_BEFORE_SAVE, &GetEditWnd().GetActiveView() );

		if(!pSaveInfo->bOverwriteMode){	//上書きでなければ前文書のクローズイベントを呼ぶ
			//プラグイン：DocumentCloseイベント実行
			CJackManager::getInstance()->InvokePlugins( PP_DOCUMENT_CLOSE, &GetEditWnd().GetActiveView() );
		}

		//セーブ処理
		m_pcDocRef->NotifyBeforeSave(*pSaveInfo);	//前処理
		m_pcDocRef->NotifySave(*pSaveInfo);			//本処理
		m_pcDocRef->NotifyAfterSave(*pSaveInfo);	//後処理

		//プラグイン：DocumentAfterSaveイベント実行
		CJackManager::getInstance()->InvokePlugins( PP_DOCUMENT_AFTER_SAVE, &GetEditWnd().GetActiveView() );

		//結果
		eSaveResult = SAVED_OK; //###仮
	}
	catch(const CFlowInterruption&){
		eSaveResult = SAVED_INTERRUPT;
	}
	catch(...){
		//予期せぬ例外が発生した場合も NotifyFinalSave は必ず呼ぶ！
		m_pcDocRef->NotifyFinalSave(SAVED_FAILURE);
		throw;
	}

	//最終処理
	m_pcDocRef->NotifyFinalSave(eSaveResult);

	return eSaveResult==SAVED_OK;
}

/*! 上書き保存

	@return 保存が行われたor保存不要のため何も行わなかったときにtrueを返す

	@date 2004.06.05 genta  ビューモードのチェックをCEditDocから上書き保存処理に移動
	@date 2006.12.30 ryoji  CEditView::Command_FILESAVE()から処理本体を切り出し
	@date 2008.03.20 kobake 戻り値の仕様を定義
*/
bool CDocFileOperation::FileSave()
{
	//ファイル名が指定されていない場合は「名前を付けて保存」のフローへ遷移
	if( !m_pcDocRef->m_cDocFile.GetFilePathClass().IsValidPath() ){
		return FileSaveAs();
	}

	//セーブ情報
	SSaveInfo sSaveInfo;
	m_pcDocRef->GetSaveInfo(&sSaveInfo);
	sSaveInfo.cEol = EEolType::none; //改行コード無変換
	sSaveInfo.bOverwriteMode = true; //上書き要求

	//上書き処理
	return m_pcDocRef->m_cDocFileOperation.DoSaveFlow(&sSaveInfo);
}

/*! 名前を付けて保存フロー

	@date 2006.12.30 ryoji CEditView::Command_FILESAVEAS_DIALOG()から処理本体を切り出し
*/
bool CDocFileOperation::FileSaveAs( const WCHAR* filename,ECodeType eCodeType, EEolType eEolType, bool bDialog )
{
	//セーブ情報
	SSaveInfo sSaveInfo;
	m_pcDocRef->GetSaveInfo(&sSaveInfo);
	sSaveInfo.cEol = EEolType::none; // 初期値は変換しない
	if( filename ){
		// ダイアログなし保存、またはマクロの引数あり
		sSaveInfo.cFilePath = filename;
		if( CEol::IsNoneOrValid( eEolType ) ){
			sSaveInfo.cEol = eEolType;
		}
		if( IsValidCodeType(eCodeType) && eCodeType != sSaveInfo.eCharCode ){
			sSaveInfo.eCharCode = eCodeType;
			sSaveInfo.bBomExist = CCodeTypeName(eCodeType).IsBomDefOn();
		}
	}
	if( bDialog ){
		if(!filename && CAppMode::getInstance()->IsViewMode()){
			sSaveInfo.cFilePath = L""; //※読み込み専用モードのときはファイル名を指定しない
		}

		//ダイアログ表示
		if(!SaveFileDialog(&sSaveInfo))return false;
	}

	//セーブ処理
	if( DoSaveFlow(&sSaveInfo) ){
		// オープン後自動実行マクロを実行する（ANSI版ではここで再ロード実行→自動実行マクロが実行される）
		// 提案時の Patches#1550557 に、「名前を付けて保存」でオープン後自動実行マクロが実行されることの是非について議論の経緯あり
		//   →”ファイル名に応じて表示を変化させるマクロとかを想定すると、これはこれでいいように思います。”
		m_pcDocRef->RunAutoMacro( GetDllShareData().m_Common.m_sMacro.m_nMacroOnOpened );

		//プラグイン：DocumentOpenイベント実行
		CJackManager::getInstance()->InvokePlugins( PP_DOCUMENT_OPEN, &GetEditWnd().GetActiveView() );

		return true;
	}

	return false;
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                         クローズ                            //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

/*
	閉じて(無題)。
	ユーザーキャンセル操作等によりクローズされなかった場合は false を返す。

	@date 2006.12.30 ryoji CEditView::Command_FILESAVEAS()から処理本体を切り出し
*/
bool CDocFileOperation::FileClose()
{
	/* ファイルを閉じるときのMRU登録 & 保存確認 & 保存実行 */
	if( !m_pcDocRef->OnFileClose(false) ){
		return false;
	}

	//プラグイン：DocumentCloseイベント実行
	CJackManager::getInstance()->InvokePlugins( PP_DOCUMENT_CLOSE, &GetEditWnd().GetActiveView() );

	/* 既存データのクリア */
	m_pcDocRef->InitDoc();

	/* 全ビューの初期化 */
	m_pcDocRef->InitAllView();

	m_pcDocRef->SetCurDirNotitle();

	// 無題番号取得
	CAppNodeManager::getInstance()->GetNoNameNumber( GetEditWnd().GetHwnd() );

	/* 親ウィンドウのタイトルを更新 */
	GetEditWnd().UpdateCaption();

	// 2006.09.01 ryoji オープン後自動実行マクロを実行する
	m_pcDocRef->RunAutoMacro( GetDllShareData().m_Common.m_sMacro.m_nMacroOnOpened );

	return true;
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                          その他                             //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

/* 閉じて開く
	@date 2006.12.30 ryoji CEditView::Command_FILESAVEAS()から処理本体を切り出し
*/
void CDocFileOperation::FileCloseOpen( const SLoadInfo& _sLoadInfo )
{
	/* ファイルを閉じるときのMRU登録 & 保存確認 & 保存実行 */
	if( !m_pcDocRef->OnFileClose(false) ){
		return;
	}

	//プラグイン：DocumentCloseイベント実行
	CJackManager::getInstance()->InvokePlugins( PP_DOCUMENT_CLOSE, &GetEditWnd().GetActiveView() );

	//ファイル名指定が無い場合はダイアログで入力させる
	SLoadInfo sLoadInfo = _sLoadInfo;
	if( sLoadInfo.cFilePath.Length()==0 ){
		std::vector<std::wstring> files;
		if( !OpenFileDialog( CEditWnd::getInstance()->GetHwnd(), nullptr, &sLoadInfo, files ) ){
			return;
		}
		sLoadInfo.cFilePath = files[0].c_str();
		// 他のファイルは新規ウィンドウ
		size_t nSize = files.size();
		for( size_t i = 1; i < nSize; i++ ){
			SLoadInfo sFilesLoadInfo = sLoadInfo;
			sFilesLoadInfo.cFilePath = files[i].c_str();
			CControlTray::OpenNewEditor(
				G_AppInstance(),
				CEditWnd::getInstance()->GetHwnd(),
				sFilesLoadInfo,
				nullptr,
				true
			);
		}
	}

	/* 既存データのクリア */
	m_pcDocRef->InitDoc();

	/* 全ビューの初期化 */
	m_pcDocRef->InitAllView();

	//開く
	FileLoadWithoutAutoMacro(&sLoadInfo);

	if( !m_pcDocRef->m_cDocFile.GetFilePathClass().IsValidPath() ){
		m_pcDocRef->SetCurDirNotitle();
		CAppNodeManager::getInstance()->GetNoNameNumber( GetEditWnd().GetHwnd() );
	}

	/* 親ウィンドウのタイトルを更新 */
	GetEditWnd().UpdateCaption();

	// オープン後自動実行マクロを実行する
	// ※ロードしてなくても(無題)には変更済み
	m_pcDocRef->RunAutoMacro( GetDllShareData().m_Common.m_sMacro.m_nMacroOnOpened );

	//プラグイン：DocumentOpenイベント実行
	CJackManager::getInstance()->InvokePlugins( PP_DOCUMENT_OPEN, &GetEditWnd().GetActiveView() );
}
