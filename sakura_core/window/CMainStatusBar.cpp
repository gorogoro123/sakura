﻿/*! @file */
/*
	Copyright (C) 2018-2022, Sakura Editor Organization

	SPDX-License-Identifier: Zlib
*/
#include "StdAfx.h"
#include "CMainStatusBar.h"
#include "window/CEditWnd.h"
#include "CEditApp.h"
#include "apiwrap/CommonControl.h"

CMainStatusBar::CMainStatusBar(CEditWnd* pOwner)
: m_pOwner(pOwner)
, m_hwndStatusBar( nullptr )
, m_hwndProgressBar( nullptr )
{
}

//	キーワード：ステータスバー順序
/* ステータスバー作成 */
void CMainStatusBar::CreateStatusBar()
{
	if( m_hwndStatusBar )return;

	/* ステータスバー */
	m_hwndStatusBar = ::CreateWindowEx(
		WS_EX_RIGHT | WS_EX_COMPOSITED,
		STATUSCLASSNAME,
		nullptr,
		WS_CHILD/* | WS_VISIBLE*/ | SBARS_SIZEGRIP,	// 2007.03.08 ryoji WS_VISIBLE 除去
		0, 0, 0, 0, // X, Y, nWidth, nHeight
		m_pOwner->GetHwnd(),
		(HMENU)IDW_STATUSBAR,
		CEditApp::getInstance()->GetAppInstance(),
		nullptr
	);

	/* プログレスバー */
	m_hwndProgressBar = ::CreateWindowEx(
		WS_EX_TOOLWINDOW,
		PROGRESS_CLASS,
		nullptr,
		WS_CHILD /*|  WS_VISIBLE*/,
		3,
		5,
		150,
		13,
		m_hwndStatusBar,
		nullptr,
		CEditApp::getInstance()->GetAppInstance(),
		nullptr
	);

	if( nullptr != m_pOwner->m_cFuncKeyWnd.GetHwnd() ){
		m_pOwner->m_cFuncKeyWnd.SizeBox_ONOFF( FALSE );
	}

	//スプリッターの、サイズボックスの位置を変更
	m_pOwner->m_cSplitterWnd.DoSplit( -1, -1);
}

/* ステータスバー破棄 */
void CMainStatusBar::DestroyStatusBar()
{
	if( nullptr != m_hwndProgressBar ){
		::DestroyWindow( m_hwndProgressBar );
		m_hwndProgressBar = nullptr;
	}
	::DestroyWindow( m_hwndStatusBar );
	m_hwndStatusBar = nullptr;

	if( nullptr != m_pOwner->m_cFuncKeyWnd.GetHwnd() ){
		bool bSizeBox;
		if( GetDllShareData().m_Common.m_sWindow.m_nFUNCKEYWND_Place == 0 ){	/* ファンクションキー表示位置／0:上 1:下 */
			/* サイズボックスの表示／非表示切り替え */
			bSizeBox = false;
		}
		else{
			bSizeBox = true;
			/* ステータスパーを表示している場合はサイズボックスを表示しない */
			if( nullptr != m_hwndStatusBar ){
				bSizeBox = false;
			}
		}
		m_pOwner->m_cFuncKeyWnd.SizeBox_ONOFF( bSizeBox );
	}
	//スプリッターの、サイズボックスの位置を変更
	m_pOwner->m_cSplitterWnd.DoSplit( -1, -1 );
}

/*!
	@brief メッセージの表示
	
	指定されたメッセージをステータスバーに表示する．
	メニューバー右端に入らないものや，桁位置表示を隠したくないものに使う
	
	呼び出し前にSendStatusMessage2IsEffective()で処理の有無を
	確認することで無駄な処理を省くことが出来る．

	@param msg [in] 表示するメッセージ
	@date 2005.07.09 genta 新規作成
	
	@sa SendStatusMessage2IsEffective
*/
void CMainStatusBar::SendStatusMessage2( const WCHAR* msg )
{
	if( nullptr != m_hwndStatusBar ){
		SetStatusText(0, SBT_NOBORDERS, msg);
	}
}

/*!
	@brief 文字列をステータスバーの指定されたパートに表示する
	
	@param nIndex [in] パートのインデクス
	@param nOption [in] 描画オペレーションタイプ
	@param pszText [in] 表示テキスト
	@param textLen [in] 表示テキストの文字数
*/
bool CMainStatusBar::SetStatusText(int nIndex, int nOption, const WCHAR* pszText, size_t textLen /* = SIZE_MAX */)
{
	if( !m_hwndStatusBar ){
		assert(m_hwndStatusBar != nullptr);
		return false;
	}
	// StatusBar_SetText 関数を呼びだすかどうかを判定する
	// （StatusBar_SetText は SB_SETTEXT メッセージを SendMessage で送信する）
	bool bDraw = true;
	do {
		// オーナードローの場合は SB_SETTEXT メッセージを無条件に発行するように判定
		// 本来表示に変化が無い場合には呼び出さない方が表示のちらつきが減るので好ましいが
		// 判定が難しいので諦める
		if( nOption == SBT_OWNERDRAW ){
			break;
		}
		// オーナードローではない場合で NULLの場合は空文字に置き換える
		// NULL を渡しても問題が無いのかどうか公式ドキュメントに記載されていない
		// NULL のままでも問題は発生しないようだが念の為に対策を追加
		if( pszText == nullptr ){
			static const wchar_t emptyStr[] = L"";
			pszText = emptyStr;
			textLen = 0;
		}
		LRESULT res = ::StatusBar_GetTextLength( m_hwndStatusBar, nIndex );
		// 表示オペレーション値が変化する場合は SB_SETTEXT メッセージを発行
		if( HIWORD(res) != nOption ){
			break;
		}
		size_t prevTextLen = LOWORD(res);
		WCHAR prev[1024];
		// 設定済みの文字列長が長過ぎて取得できない場合は、SB_SETTEXT メッセージを発行
		if( prevTextLen >= _countof(prev) ){
			break;
		}
		// 設定する文字列長パラメータが SIZE_MAX（引数のデフォルト値）な場合は文字列長を取得
		if( textLen == SIZE_MAX ){
			textLen = wcslen(pszText);
		}
		// 設定済みの文字列長と設定する文字列長が異なる場合は、SB_SETTEXT メッセージを発行
		if( prevTextLen != textLen ){
			break;
		}
		if( prevTextLen > 0 ){
			::StatusBar_GetText( m_hwndStatusBar, nIndex, prev );
			// 設定済みの文字列と設定する文字列を比較して異なる場合は、SB_SETTEXT メッセージを発行
			bDraw = wcscmp(prev, pszText) != 0;
		}
		else {
			// 設定する文字列長が0の場合は設定する文字列長が0より大きい場合のみ設定する（既に空文字なら空文字を設定する必要は無い為）
			bDraw = textLen > 0;
		}
	}while (false);
	if (bDraw) {
		StatusBar_SetText(m_hwndStatusBar, nIndex | nOption, pszText);
	}
	return bDraw;
}

//! プログレスバーの表示/非表示を切り替える
void CMainStatusBar::ShowProgressBar(bool bShow) const {
	if (m_hwndStatusBar && m_hwndProgressBar) {
		// プログレスバーを表示するステータスバー上の領域を取得
		RECT rcProgressArea = {};
		ApiWrap::StatusBar_GetRect(m_hwndStatusBar, 0, &rcProgressArea);
		if (bShow) {
			::ShowWindow(m_hwndProgressBar, SW_SHOW);
		} else {
			::ShowWindow(m_hwndProgressBar, SW_HIDE);
		}
		// プログレスバー表示領域を再描画
		::InvalidateRect(m_hwndStatusBar, &rcProgressArea, TRUE);
		::UpdateWindow(m_hwndStatusBar);
	}
}
