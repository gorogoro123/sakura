﻿/*!	@file
@brief CViewCommanderクラスのコマンド(タグジャンプ)関数群

	2012/12/17	CViewCommander.cppから分離
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2000-2001, genta
	Copyright (C) 2001, YAZAKI
	Copyright (C) 2002, MIK
	Copyright (C) 2003, MIK, genta, じゅうじ
	Copyright (C) 2004, Moca, novice
	Copyright (C) 2005, MIK
	Copyright (C) 2006, genta
	Copyright (C) 2007, ryoji, maru, Uchi
	Copyright (C) 2008, MIK
	Copyright (C) 2010, Moca
	Copyright (C) 2018-2022, Sakura Editor Organization

	This source code is designed for sakura editor.
	Please contact the copyright holders to use this code for other purpose.
*/

#include "StdAfx.h"
#include "CViewCommander.h"
#include "CViewCommander_inline.h"

#include "uiparts/CWaitCursor.h"
#include "dlg/CDlgCancel.h"// 2002/2/8 hor
#include "dlg/CDlgTagJumpList.h"
#include "dlg/CDlgTagsMake.h"	//@@@ 2003.05.12 MIK
#include "CEditApp.h"
#include "util/window.h"
#include "util/module.h"
#include "util/string_ex2.h"
#include "env/CSakuraEnvironment.h"
#include "CGrepAgent.h"
#include "apiwrap/StdApi.h"
#include "CSelectLang.h"
#include "sakura_rc.h"
#include "config/system_constants.h"
#include "String_define.h"

//! "までを切り取る
// sizeは切り出し文字列のNULL終端を含む長さ(wstring::length()+1の値)
static bool GetQuoteFilePath(const wchar_t* pLine, std::wstring& str, size_t size)
{
	const wchar_t* pFileEnd = wcschr( pLine, L'\"' );
	str.clear();
	if (pFileEnd == nullptr) {
		return false;
	}
	size_t nFileLen = pFileEnd - pLine;
	if (0 == nFileLen || size <= nFileLen) {
		return false;
	}
	str.assign(pLine, nFileLen);
	return !str.empty();
}

static bool IsFileExists2( const wchar_t* pszFile )
{
	for(int i = 0; pszFile[i]; i++ ){
		if( !WCODE::IsValidFilenameChar(pszFile[i]) ){
			return false;
		}
	}
	if( _IS_REL_PATH(pszFile) ){
		return false;
	}
	return IsFileExists(pszFile, true);
}

static bool IsHWNDTag( const wchar_t* pLine, std::wstring& str, int* pnLen = nullptr )
{
	if( 0 == wcsncmp(pLine, L":HWND:[", 7) ){
		const wchar_t* pFileEnd = wcschr( pLine, L']' );
		if( pFileEnd ){
			const int nLen = pFileEnd - pLine + 1;
			int i = 7;
			for( ; i < nLen; i++ ){
				if( !(WCODE::Is09(pLine[i]) || (L'a' <= pLine[i] && L'f' <= pLine[i])) ){
					break;
				}
			}
			if( i != nLen && nLen <= 16 + 8 ){
				if( pnLen ){
					*pnLen = nLen;
					return true;
				}
				str.assign(pLine, nLen);
				return true;
			}
		}
	}
	return false;
}

static int GetLineColumnPos(const wchar_t* pLine)
{
	// filename(1234,56): str
	const wchar_t* pTagEnd = wcsstr( pLine, L"): " );
	if( !pTagEnd ){
		// filename(1234,56)  [SJIS]: str
		pTagEnd = wcsstr( pLine, L"]: " );
		if( pTagEnd ){
			int fileEnd = pTagEnd - pLine - 1;
			for( ; 1 < fileEnd; fileEnd-- ){
				if( L'[' == pLine[fileEnd] ){
					fileEnd--;
					break;
				}
			}
			for( ; 1 < fileEnd && L' ' == pLine[fileEnd]; fileEnd-- ){}
			if( ')' == pLine[fileEnd] ){
				pTagEnd = &pLine[fileEnd];
			}else{
				pTagEnd = nullptr;
			}
		}
	}
	if( pTagEnd ){
		int fileEnd = pTagEnd - pLine - 1;
		for( ; 1 < fileEnd && (L'0' <= pLine[fileEnd] && pLine[fileEnd] <= L'9'); fileEnd-- ){}
		if(    1 < fileEnd && (L',' == pLine[fileEnd]) ){ fileEnd--; }
		for( ; 1 < fileEnd && (L'0' <= pLine[fileEnd] && pLine[fileEnd] <= L'9'); fileEnd-- ){}
		if( 1 < fileEnd && L'(' == pLine[fileEnd] ){
			return fileEnd;
		}
	}
	return 0;
}

/*! タグジャンプ

	@param bClose [in] true:元ウィンドウを閉じる

	@date 2003.04.03 genta 元ウィンドウを閉じるかどうかの引数を追加
	@date 2004.05.13 Moca 行桁位置の指定が無い場合は、行桁を移動しない
	@date 2011.11.24 Moca Grepフォルダー毎表示対応
*/
bool CViewCommander::Command_TAGJUMP( bool bClose )
{
	bool ret = Command_TagJumpNoMessage(bClose);
	if(ret == false){
		m_pCommanderView->SendStatusMessage(LS(STR_ERR_TAGJMP1));	//@@@ 2003.04.13
	}
	return ret;
}

bool CViewCommander::Command_TagJumpNoMessage( bool bClose )
{
	//	2004.05.13 Moca 初期値を1ではなく元の位置を継承するように
	// 0以下は未指定扱い。(1開始)
	int			nJumpToLine;
	int			nJumpToColumn;
	nJumpToLine = 0;
	nJumpToColumn = 0;

	//ファイル名バッファ
	size_t		nBgn;
	size_t		nPathLen;

	/*
	  カーソル位置変換
	  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
	  →
	  物理位置(行頭からのバイト数、折り返し無し行位置)
	*/
	CLogicPoint ptXY, ptXYOrg;
	GetDocument()->m_cLayoutMgr.LayoutToLogic(
		GetCaret().GetCaretLayoutPos(),
		&ptXY
	);
	ptXYOrg = ptXY;

	/* 現在行のデータを取得 */
	CLogicInt		nLineLen;
	const wchar_t*	pLine = CDocLine::GetDocLineStrWithEOL_Safe(GetDocument()->m_cDocLineMgr.GetLine(ptXY.GetY2()), &nLineLen);
	if( nullptr == pLine ){
		return false;
	}

	// ノーマル
	// C:\RootFolder\SubFolders\FileName.ext(5395,11): str
	// :HWND:[01234567] 無題1(1234,56): str
	// :HWND:[01234567] 無題1(1234,56)  [SJIS]: str

	// ノーマル/ベースフォルダー/フォルダー毎
	// ◎"C:\RootFolder"
	// ■
	// ・FileName.ext(5395,11): str
	// ■"SubFolders"
	// ・FileName.ext(5395,11): str
	// ・FileName.ext(5396,11): str
	// ・FileName2.ext(123,12): str

	// ノーマル/ベースフォルダー
	// ■"C:\RootFolder"
	// ・FileName.ext(5395,11): str
	// ・SubFolders\FileName2.ext(5395,11): str
	// ・SubFolders\FileName2.ext(5396,11): str
	// ・SubFolders\FileName3.ext(123,11): str

	// ノーマル/フォルダー毎
	// ■"C:\RootFolder"
	// ・FileName.cpp(5395,11): str
	// ■"C:\RootFolder\SubFolders"
	// ・FileName2.ext(5395,11): str
	// ・FileName2.ext(5396,11): str
	// ・FileName3.ext(123,11): str

	// ファイル毎(WZ風)
	// ■"C:\RootFolder\FileName.ext"
	// ・(  5395,11   ): str
	// ■"C:\RootFolder\SubFolders\FileName2.ext"
	// ・(  5395,11   ): str
	// ・(  5396,11   ): str
	// ■"C:\RootFolder\SubFolders\FileName3.ext"
	// ・(   123,12   ): str

	// ファイル毎/ベースフォルダー
	// ◎"C:\RootFolder"
	// ■"FileName.ext"
	// ・(  5395,11   ): str
	// ■"SubFolders\FileName2.ext"
	// ・(  5395,11   ): str
	// ・(  5396,11   ): str
	// ■"SubFolders\FileName3.ext"
	// ・(   123,12   ): str

	// ファイル毎/ベースフォルダー/フォルダー毎
	// ◎"C:\RootFolder"
	// ■
	// ◆"FileName.ext"
	// ・(  5395,11   ): str
	// ■"SubFolders"
	// ◆"FileName2.ext"
	// ・(  5395,11   ): str
	// ・(  5396,11   ): str
	// ◆"FileName3.ext"
	// ・(   123,12   ): str

	// ファイル毎/フォルダー毎
	// ■"C:\RootFolder"
	// ◆"FileName.ext"
	// ・(  5395,11   ): str
	// ■"C:\RootFolder\SubFolders"
	// ◆"FileName2.ext"
	// ・(  5395,11   ): str
	// ・(  5396,11   ): str


	// Grep結果のタグジャンプ検索
	// ・→◆→■→◎ の順に検索してパスを結合する

	// 自動選択を選ぶとエンコード名表示
	// C:\RootFolder\SubFolders\FileName.ext(150,6)  [UTF-8]: str
	// ・SubFolders\FileName.ext(150,6)  [UTF-8]: str
	// ・FileName.ext(150,6)  [UTF-8]: str

	// ■"C:\RootFolder\SubFolders\FileName.ext"  [UTF-8]
	// ・(   150,6    ): str

	// ◎"C:\RootFolder"
	// ■"FileName.ext"  [UTF-8]
	// ・(  5395,11   ): str
	// ■"SubFolders\FileName2.ext"  [UTF-8]
	// ・(  5395,11   ): str

	// ■"C:\RootFolder\SubFolders\"
	// ◆"FileName.ext"  [UTF-8]
	// ・(   150,60   ): str

	std::wstring	strJumpToFile;
	std::wstring	strFile;
	constexpr int MAX_TAG_PATH = MAX_PATH;

	do{
		enum TagListSeachMode{
			TAGLIST_FILEPATH,
			TAGLIST_SUBPATH,
			TAGLIST_ROOT,
		} searchMode = TAGLIST_FILEPATH;
		if( 0 == wmemcmp( pLine, L"■\"", 2 ) ){
			/* WZ風のタグリストか */
			if( IsFilePath( &pLine[2], &nBgn, &nPathLen ) && !_IS_REL_PATH( &pLine[2] ) ){
				strJumpToFile.assign(&pLine[2 + nBgn], nPathLen);
				GetLineColumn( &pLine[2 + nPathLen], &nJumpToLine, &nJumpToColumn );
				break;
			}else if (!GetQuoteFilePath(&pLine[2], strFile, MAX_TAG_PATH)) {
				break;
			}
			searchMode = TAGLIST_ROOT;
		}else if( 0 == wmemcmp( pLine, L"◆\"", 2 ) ){
			if (!GetQuoteFilePath(&pLine[2], strFile, MAX_TAG_PATH)) {
				break;
			}else if( IsHWNDTag(&pLine[2], strJumpToFile) ){
				break;
			}
			searchMode = TAGLIST_SUBPATH;
		}else if( 0 == wmemcmp( pLine, L"・", 1 ) ){
			if( pLine[1] == L'"' ){
				// ・"FileName.ext"
				if (!GetQuoteFilePath(&pLine[2], strFile, MAX_TAG_PATH)) {
					break;
				}
			if( IsHWNDTag(&pLine[2], strJumpToFile) ){
				break;
			}
				searchMode = TAGLIST_SUBPATH;
			}else if( pLine[1] == L'(' ){
				// ファイル毎(WZ風)
				// ・(    12,34   ): str
				GetLineColumn( &pLine[1], &nJumpToLine, &nJumpToColumn );
				searchMode = TAGLIST_FILEPATH;
			}else{
				// ノーマル/ファイル相対パス
				// ・FileName.ext(123,45): str
				// ・FileName.ext(123,45)  [SJIS]: str
				// ・subpath\FileName.ext(123,45): str
				// ・:HWND:[01234567](無題)2(123,45): str
				int fileEnd = GetLineColumnPos(pLine);
				if (1 < fileEnd && L'(' == pLine[fileEnd] && fileEnd - 1 < MAX_TAG_PATH) {
					strFile.assign(pLine + 1, fileEnd - 1);
					GetLineColumn( &pLine[fileEnd + 1], &nJumpToLine, &nJumpToColumn );
					if( IsHWNDTag(pLine + 1, strJumpToFile) ){
						break;
					}
					searchMode = TAGLIST_SUBPATH;
				}else{
					break;
				}
			}
		}else{
			break;
		}
		ptXY.y--;

		for( ; 0 <= ptXY.y; ptXY.y-- ){
			pLine = GetDocument()->m_cDocLineMgr.GetLine(ptXY.GetY2())->GetDocLineStrWithEOL(&nLineLen);
			if( nullptr == pLine ){
				break;
			}
			if( 0 == wmemcmp( pLine, L"・", 1 ) ){
				continue;
			}else if( 3 <= nLineLen && 0 == wmemcmp( pLine, L"◆\"", 2 ) ){
				if( searchMode == TAGLIST_SUBPATH || searchMode == TAGLIST_ROOT ){
					continue;
				}
				if( IsHWNDTag(&pLine[2], strJumpToFile) ){
					break;
				}
				// フォルダー毎：ファイル名
				if (GetQuoteFilePath(&pLine[2], strFile, MAX_TAG_PATH)) {
					searchMode = TAGLIST_SUBPATH;
					continue;
				}
				break;
			}else if( 2 <= nLineLen && pLine[0] == L'■' && (pLine[1] == L'\r' || pLine[1] == L'\n') ){
				// ルートフォルダー
				if( searchMode == TAGLIST_ROOT ){
					continue;
				}
				searchMode = TAGLIST_ROOT;
			}else if( 3 <= nLineLen && 0 == wmemcmp( pLine, L"■\"", 2 ) ){
				if( searchMode == TAGLIST_ROOT ){
					continue;
				}
				if( IsHWNDTag(&pLine[2], strJumpToFile) ){
					break;
				}
				// ファイル毎(WZ風)：フルパス
				if( IsFilePath( &pLine[2], &nBgn, &nPathLen ) && !_IS_REL_PATH( &pLine[2] ) ){
					strJumpToFile.assign(&pLine[2 + nBgn], nPathLen);
					break;
				}
				// 相対フォルダーorファイル名
				std::wstring strPath;
				if (GetQuoteFilePath(&pLine[2], strPath, MAX_TAG_PATH)) {
					if (strFile.empty() == false) {
						strPath = AddLastYenPath(strPath);
					}
					strPath.append(strFile);
					if( MAX_TAG_PATH <= strPath.size() ){
						break;
					}
					if( IsFileExists2(strPath.c_str()) ){
						strJumpToFile = strPath;
						break;
					}
					// 相対パスだった→◎”を探す
					strFile = std::move(strPath);
					searchMode = TAGLIST_ROOT;
					continue;
				}
				break;
			}else if( 3 <= nLineLen && 0 == wmemcmp( pLine, L"◎\"", 2 ) ){
				std::wstring strPath;
				if (GetQuoteFilePath(&pLine[2], strPath, MAX_TAG_PATH)) {
					strPath = AddLastYenPath(strPath);
					strPath.append(strFile);
					if (MAX_TAG_PATH <= strPath.size()) {
						break;
					}
					if (IsFileExists2(strPath.c_str())) {
						strJumpToFile = std::move(strPath);
						break;
					}
				}
				break;
			}
			else{
				break;
			}
		}
	}while(0);

	if (strJumpToFile.empty()) {
		pLine = GetDocument()->m_cDocLineMgr.GetLine(ptXYOrg.GetY2())->GetDocLineStrWithEOL(&nLineLen);
		if( nullptr == pLine ){
			return false;
		}
		int nLen = 0;
		std::wstring test;
		if (IsHWNDTag(pLine, test, &nLen)) {
			int nFileEnd = GetLineColumnPos(pLine);
			if( nFileEnd ){
				strJumpToFile.assign(pLine, nLen);
				GetLineColumn(&pLine[nFileEnd + 1], &nJumpToLine, &nJumpToColumn);
			}
		}
	}
	if (strJumpToFile.empty()) {
		//@@@ 2001.12.31 YAZAKI
		const wchar_t *p = pLine;
		const wchar_t *p_end = p + nLineLen;

		//	From Here Aug. 27, 2001 genta
		//	Borland 形式のメッセージからのTAG JUMP
		while( p < p_end ){
			//	skip space
			for( ; p < p_end && ( *p == L' ' || *p == L'\t' || WCODE::IsLineDelimiter(*p, GetDllShareData().m_Common.m_sEdit.m_bEnableExtEol) ); ++p )
				;
			if( p >= p_end )
				break;

			//	Check Path
			if( IsFilePath( p, &nBgn, &nPathLen ) ){
				strJumpToFile.assign(&p[nBgn], nPathLen);
				GetLineColumn( &p[nBgn + nPathLen], &nJumpToLine, &nJumpToColumn );
				break;
			}
			//	Jan. 04, 2001 genta Directoryを対象外にしたので文字列には柔軟に対応
			//	break;	//@@@ 2001.12.31 YAZAKI 「working ...」問題に対処
			//	skip non-space
			for( ; p < p_end && ( *p != L' ' && *p != L'\t' ); ++p )
				;
		}
	}

	// 2011.11.29 Grep形式で失敗した後もTagsを検索する
	if (strJumpToFile.empty() && Command_TagJumpByTagsFile(bClose) ){	//@@@ 2003.04.13
		return true;
		//	From Here Aug. 27, 2001 genta
	}

	//	Apr. 21, 2003 genta bClose追加
	if (strJumpToFile.empty() == false &&
		m_pCommanderView->TagJumpSub(strJumpToFile.c_str(), CMyPoint(nJumpToColumn, nJumpToLine), bClose ) ){	//@@@ 2003.04.13
		return true;
	}

	return false;
}

/* タグジャンプバック */
void CViewCommander::Command_TAGJUMPBACK( void )
{
// 2004/06/21 novice タグジャンプ機能追加
	TagJump tagJump;

	/* タグジャンプ情報の参照 */
	if( !CTagJumpManager().PopTagJump(&tagJump) || !IsSakuraMainWindow(tagJump.hwndReferer) ){
		m_pCommanderView->SendStatusMessage(LS(STR_ERR_TAGJMPBK1));
		// 2004.07.10 Moca m_TagJumpNumを0にしなくてもいいと思う
		// GetDllShareData().m_TagJumpNum = 0;
		return;
	}

	/* アクティブにする */
	ActivateFrameWindow( tagJump.hwndReferer );

	/* カーソルを移動させる */
	GetDllShareData().m_sWorkBuffer.m_LogicPoint = tagJump.point;
	::SendMessageAny( tagJump.hwndReferer, MYWM_SETCARETPOS, 0, 0 );

	return;
}

/*
	タグファイルを作成する。

	@author	MIK
	@date	2003.04.13	新規作成
	@date	2003.05.12	ダイアログ表示でフォルダー等を細かく指定できるようにした。
	@date 2008.05.05 novice GetModuleHandle(NULL)→NULLに変更
*/
bool CViewCommander::Command_TagsMake( void )
{
#define	CTAGS_COMMAND	L"ctags.exe"

	WCHAR	szTargetPath[1024 /*_MAX_PATH+1*/ ];
	if( GetDocument()->m_cDocFile.GetFilePathClass().IsValidPath() )
	{
		wcscpy( szTargetPath, GetDocument()->m_cDocFile.GetFilePath() );
		szTargetPath[ wcslen( szTargetPath ) - wcslen( GetDocument()->m_cDocFile.GetFileName() ) ] = L'\0';
	}
	else
	{
		// 20100722 Moca サクラのフォルダーからカレントディレクトリに変更
		::GetCurrentDirectory( _countof(szTargetPath), szTargetPath );
	}

	//ダイアログを表示する
	CDlgTagsMake	cDlgTagsMake;
	if( !cDlgTagsMake.DoModal( G_AppInstance(), m_pCommanderView->GetHwnd(), 0, szTargetPath ) ) return false;

	WCHAR	cmdline[1024];
	/* exeのあるフォルダー */
	WCHAR	szExeFolder[_MAX_PATH + 1];

	GetExedir( cmdline, CTAGS_COMMAND );
	SplitPath_FolderAndFile( cmdline, szExeFolder, nullptr );

	//ctags.exeの存在チェック
	if( (DWORD)-1 == ::GetFileAttributes( cmdline ) )
	{
		WarningMessage( m_pCommanderView->GetHwnd(), LS(STR_ERR_CEDITVIEW_CMD03) );
		return false;
	}

	HANDLE	hStdOutWrite, hStdOutRead;
	CDlgCancel	cDlgCancel;
	CWaitCursor	cWaitCursor( m_pCommanderView->GetHwnd() );

	PROCESS_INFORMATION	pi;
	ZeroMemory( &pi, sizeof(pi) );

	//子プロセスの標準出力と接続するパイプを作成
	SECURITY_ATTRIBUTES	sa;
	ZeroMemory( &sa, sizeof(sa) );
	sa.nLength              = sizeof(sa);
	sa.bInheritHandle       = TRUE;
	sa.lpSecurityDescriptor = nullptr;
	hStdOutRead = hStdOutWrite = nullptr;
	if( CreatePipe( &hStdOutRead, &hStdOutWrite, &sa, 1000 ) == FALSE )
	{
		//エラー
		return false;
	}

	//継承不能にする
	DuplicateHandle( GetCurrentProcess(), hStdOutRead,
				GetCurrentProcess(), nullptr,
				0, FALSE, DUPLICATE_SAME_ACCESS );

	//CreateProcessに渡すSTARTUPINFOを作成
	STARTUPINFO	sui;
	ZeroMemory( &sui, sizeof(sui) );
	sui.cb          = sizeof(sui);
	sui.dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	sui.wShowWindow = SW_HIDE;
	sui.hStdInput   = GetStdHandle( STD_INPUT_HANDLE );
	sui.hStdOutput  = hStdOutWrite;
	sui.hStdError   = hStdOutWrite;

	//	To Here Dec. 28, 2002 MIK

	WCHAR	options[1024];
	wcscpy( options, L"--excmd=n" );	//デフォルトのオプション
	if( cDlgTagsMake.m_nTagsOpt & 0x0001 ) wcscat( options, L" -R" );	//サブフォルダーも対象
	if( cDlgTagsMake.m_szTagsCmdLine[0] != L'\0' )	//個別指定のコマンドライン
	{
		wcscat( options, L" " );
		wcscat( options, cDlgTagsMake.m_szTagsCmdLine );
	}
	wcscat( options, L" *" );	//配下のすべてのファイル

	//コマンドライン文字列作成(MAX:1024)
	{
		// 2010.08.28 Moca システムディレクトリ付加
		WCHAR szCmdDir[_MAX_PATH];
		::GetSystemDirectory(szCmdDir, _countof(szCmdDir));
		//	2006.08.04 genta add /D to disable autorun
		auto_sprintf( cmdline, L"\"%s\\cmd.exe\" /D /C \"\"%s\\%s\" %s\"",
				szCmdDir,
				szExeFolder,	//sakura.exeパス
				CTAGS_COMMAND,	//ctags.exe
				options			//ctagsオプション
			);
	}

	//コマンドライン実行
	BOOL bProcessResult = CreateProcess(
		nullptr, cmdline, nullptr, nullptr, TRUE,
		CREATE_NEW_CONSOLE, nullptr, cDlgTagsMake.m_szPath, &sui, &pi
	);
	if( !bProcessResult )
	{
		WarningMessage( m_pCommanderView->GetHwnd(), LS(STR_ERR_CEDITVIEW_CMD04), cmdline );
		goto finish;
	}

	{
		DWORD	read_cnt;
		DWORD	new_cnt;
		char	work[1024];
		bool	bLoopFlag = true;

		//中断ダイアログ表示
		HWND	hwndCancel;
		HWND	hwndMsg;
		hwndCancel = cDlgCancel.DoModeless( G_AppInstance(), m_pCommanderView->m_hwndParent, IDD_EXECRUNNING );
		hwndMsg = ::GetDlgItem( hwndCancel, IDC_STATIC_CMD );
		SetWindowText( hwndMsg, LS(STR_ERR_CEDITVIEW_CMD05) );

		//実行結果の取り込み
		do {
			// Jun. 04, 2003 genta CPU消費を減らすために200msec待つ
			// その間メッセージ処理が滞らないように待ち方をWaitForSingleObjectから
			// MsgWaitForMultipleObjectに変更
			switch( MsgWaitForMultipleObjects( 1, &pi.hProcess, FALSE, 200, QS_ALLEVENTS )){
				case WAIT_OBJECT_0:
					//終了していればループフラグをFALSEとする
					//ただしループの終了条件は プロセス終了 && パイプが空
					bLoopFlag = FALSE;
					break;
				case WAIT_OBJECT_0 + 1:
					//処理中のユーザー操作を可能にする
					if( !::BlockingHook( cDlgCancel.GetHwnd() ) ){
						break;
					}
					break;
				default:
					break;
			}

			//中断ボタン押下チェック
			if( cDlgCancel.IsCanceled() )
			{
				//指定されたプロセスと、そのプロセスが持つすべてのスレッドを終了させます。
				::TerminateProcess( pi.hProcess, 0 );
				break;
			}

			new_cnt = 0;
			if( PeekNamedPipe( hStdOutRead, nullptr, 0, nullptr, &new_cnt, nullptr ) )	//パイプの中の読み出し待機中の文字数を取得
			{
				if( new_cnt > 0 )												//待機中のものがある
				{
					if( new_cnt >= _countof(work) - 2 )							//パイプから読み出す量を調整
					{
						new_cnt = _countof(work) - 2;
					}
					::ReadFile( hStdOutRead, &work[0], new_cnt, &read_cnt, nullptr );	//パイプから読み出し
					if( read_cnt == 0 )
					{
						continue;
					}
					// 2003.11.09 じゅうじ
					//	正常終了の時はメッセージが出力されないので
					//	何か出力されたらエラーメッセージと見なす．
					else {
						//終了処理
						CloseHandle( hStdOutWrite );
						CloseHandle( hStdOutRead  );
						if( pi.hProcess ) CloseHandle( pi.hProcess );
						if( pi.hThread  ) CloseHandle( pi.hThread  );

						cDlgCancel.CloseDialog( TRUE );

						work[ read_cnt ] = L'\0';	// Nov. 15, 2003 genta 表示用に0終端する
						WarningMessage( m_pCommanderView->GetHwnd(), LS(STR_ERR_CEDITVIEW_CMD06), work ); // 2003.11.09 じゅうじ

						return true;
					}
				}
			}
			Sleep(0);
		} while( bLoopFlag || new_cnt > 0 );
	}

finish:
	//終了処理
	CloseHandle( hStdOutWrite );
	CloseHandle( hStdOutRead  );
	if( pi.hProcess ) CloseHandle( pi.hProcess );
	if( pi.hThread  ) CloseHandle( pi.hThread  );

	cDlgCancel.CloseDialog( TRUE );

	InfoMessage(m_pCommanderView->GetHwnd(), LS(STR_ERR_CEDITVIEW_CMD07));

	return true;
}

/*
	ダイレクトタグジャンプ(メッセージ付)

	@date	2010.07.22	新規作成
*/
bool CViewCommander::Command_TagJumpByTagsFileMsg( bool bMsg )
{
	bool ret = Command_TagJumpByTagsFile(false);
	if( false == ret && bMsg ){
		m_pCommanderView->SendStatusMessage(LS(STR_ERR_TAGJMP1));
	}
	return ret;
}

/*
	ダイレクトタグジャンプ

	@author	MIK
	@date	2003.04.13	新規作成
	@date	2003.05.12	フォルダー階層も考慮して探す
	@date
*/
bool CViewCommander::Command_TagJumpByTagsFile( bool bClose )
{
	CNativeW	cmemKeyW;
	m_pCommanderView->GetCurrentTextForSearch( cmemKeyW, true, true );
	if( 0 == cmemKeyW.GetStringLength() ){
		return false;
	}

	WCHAR	szDirFile[1024];
	if( false == Sub_PreProcTagJumpByTagsFile( szDirFile, _countof(szDirFile) ) ){
		return false;
	}
	CDlgTagJumpList	cDlgTagJumpList(true);	//タグジャンプリスト

	cDlgTagJumpList.SetFileName( szDirFile );
	cDlgTagJumpList.SetKeyword(cmemKeyW.GetStringPtr());

	int nMatchAll = cDlgTagJumpList.FindDirectTagJump();

	//複数あれば選択してもらう。
	if( 1 < nMatchAll ){
		if( ! cDlgTagJumpList.DoModal( G_AppInstance(), m_pCommanderView->GetHwnd(), (LPARAM)0 ) ){
			nMatchAll = 0;
			return true;	//キャンセル
		}
	}

	//タグジャンプする。
	if( 0 < nMatchAll ){
		//	@@ 2005.03.31 MIK 階層パラメータ追加
		WCHAR fileName[1024];
		int   fileLine;

		if( false == cDlgTagJumpList.GetSelectedFullPathAndLine( fileName, _countof(fileName), &fileLine , nullptr ) ){
			return false;
		}
		return m_pCommanderView->TagJumpSub( fileName, CMyPoint(0, fileLine), bClose );
	}

	return false;
}

/*!
	キーワードを指定してタグジャンプ(ダイアログ)
	@param keyword NULL許容
	@author MIK
	@date 2005.03.31 新規作成
	@date 2010.04.02 Moca 無題でも使えるように
*/
bool CViewCommander::Command_TagJumpByTagsFileKeyword( const wchar_t* keyword )
{
	CDlgTagJumpList	cDlgTagJumpList(false);
	WCHAR	fileName[1024];
	int		fileLine;	// 行番号
	WCHAR	szCurrentPath[1024];

	if( false == Sub_PreProcTagJumpByTagsFile( szCurrentPath, _countof(szCurrentPath) ) ){
		return false;
	}

	cDlgTagJumpList.SetFileName( szCurrentPath );
	cDlgTagJumpList.SetKeyword( keyword );

	if( ! cDlgTagJumpList.DoModal( G_AppInstance(), m_pCommanderView->GetHwnd(), 0 ) )
	{
		return true;	//キャンセル
	}

	//タグジャンプする。
	if( false == cDlgTagJumpList.GetSelectedFullPathAndLine( fileName, _countof(fileName), &fileLine, nullptr ) )
	{
		return false;
	}

	return m_pCommanderView->TagJumpSub( fileName, CMyPoint(0, fileLine) );
}

/*!
	タグジャンプの前処理
	実行可能確認と、基準ファイル名の設定
*/
bool CViewCommander::Sub_PreProcTagJumpByTagsFile( WCHAR* szCurrentPath, int count )
{
	if( count ) szCurrentPath[0] = L'\0';

	// 実行可能確認
	if( ! GetDocument()->m_cDocFile.GetFilePathClass().IsValidPath() ){
		// 2010.04.02 (無題)でもタグジャンプできるように
		// Grep、アウトプットは行番号タグジャンプがあるので無効にする(要検討)
		if( CEditApp::getInstance()->m_pcGrepAgent->m_bGrepMode ||
		    CAppMode::getInstance()->IsDebugMode() ){
		    return false;
		}
	}

	// 基準ファイル名の設定
	if( GetDocument()->m_cDocFile.GetFilePathClass().IsValidPath() ){
		wcscpy( szCurrentPath, GetDocument()->m_cDocFile.GetFilePath() );
	}else{
		if( 0 == ::GetCurrentDirectory( count - _countof(L"\\dmy") - MAX_TYPES_EXTS, szCurrentPath ) ){
			return false;
		}
		// (無題)でもファイル名を要求してくるのでダミーをつける
		// 現在のタイプ別の1番目の拡張子を拝借
		WCHAR szExts[MAX_TYPES_EXTS];
		CDocTypeManager::GetFirstExt(m_pCommanderView->m_pTypeData->m_szTypeExts, szExts, _countof(szExts));
		auto nExtLen = wcsnlen_s( szExts, _countof(szExts) );
		wcscat( szCurrentPath, L"\\dmy" );
		if( nExtLen ){
			wcscat( szCurrentPath, L"." );
			wcscat( szCurrentPath, szExts );
		}
	}
	return true;
}
