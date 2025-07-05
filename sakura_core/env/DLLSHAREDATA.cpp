﻿/*! @file */
/*
	Copyright (C) 2008, kobake
	Copyright (C) 2018-2022, Sakura Editor Organization

	SPDX-License-Identifier: Zlib
*/

#include "StdAfx.h"
#include "DLLSHAREDATA.h"
#include "_main/CMutex.h"
#include "dlg/CDlgCancel.h"
#include "uiparts/CWaitCursor.h"
#include "util/os.h"
#include "util/window.h"
#include "apiwrap/CommonControl.h"
#include "CSelectLang.h"
#include "sakura_rc.h"
#include "config/system_constants.h"
#include "String_define.h"

//GetDllShareData用グローバル変数
DLLSHAREDATA* g_theDLLSHAREDATA = nullptr;

static CMutex g_cKeywordMutex( FALSE, GSTR_MUTEX_SAKURA_KEYWORD );

CShareDataLockCounter::CShareDataLockCounter(){
	LockGuard<CMutex> guard( g_cKeywordMutex );
	assert_warning( 0 <= GetDllShareData().m_nLockCount );
	GetDllShareData().m_nLockCount++;
}

CShareDataLockCounter::~CShareDataLockCounter(){
	LockGuard<CMutex> guard( g_cKeywordMutex );
	GetDllShareData().m_nLockCount--;
	assert_warning( 0 <= GetDllShareData().m_nLockCount );
}

int CShareDataLockCounter::GetLockCounter(){
	LockGuard<CMutex> guard( g_cKeywordMutex );
	assert_warning( 0 <= GetDllShareData().m_nLockCount );
	return GetDllShareData().m_nLockCount;
}

class CLockCancel final: public CDlgCancel{
public:
	BOOL OnInitDialog( HWND hwnd, WPARAM wParam, LPARAM lParam ) override{
		BOOL ret = CDlgCancel::OnInitDialog(hwnd, wParam, lParam);
		HWND hwndCancel = GetHwnd();
		HWND hwndMsg = ::GetDlgItem(hwndCancel, IDC_STATIC_MSG);
		HWND hwndCancelButton = ::GetDlgItem(hwndCancel, IDCANCEL);
		HWND hwndKensuu = ::GetDlgItem(hwndCancel, IDC_STATIC_KENSUU);
		LPCWSTR msg = LS(STR_PRINT_WAITING);
		CTextWidthCalc calc(hwndMsg);
		calc.SetTextWidthIfMax(msg);
		RECT rc;
		GetItemClientRect(IDC_STATIC_MSG, rc);
		rc.right = rc.left + calc.GetCx() + 2;
		::MoveWindow(hwndMsg, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, FALSE);
		::SetWindowText(hwndMsg, msg);
		::ShowWindow(hwndCancelButton, SW_HIDE);
		::ShowWindow(hwndKensuu, SW_HIDE);
		if( GetComctl32Version() >= PACKVERSION(6, 0) ){
			// マーキーにする(CommCtrl 6.0以上)
			HWND hwndProgress = GetItemHwnd(IDC_PROGRESS);
			// スタイル変更+メッセージでないと機能しない
			LONG_PTR style = ::GetWindowLongPtr(hwndProgress, GWL_STYLE);
			::SetWindowLongPtr(hwndProgress, GWL_STYLE, style | PBS_MARQUEE);
			Progress_SetMarquee(hwndProgress, TRUE, 100);
		}else{
			HWND hwndProgress = ::GetDlgItem(hwndCancel, IDC_PROGRESS);
			::ShowWindow(hwndProgress, SW_HIDE);
		}
		return ret;
	}
};

// countが0だったらLockして返す
static int GetCountIf0Lock( CShareDataLockCounter** ppLock )
{
	LockGuard<CMutex> guard(g_cKeywordMutex);
	int count = GetDllShareData().m_nLockCount;
	if( count <= 0 ){
		if( ppLock ){
			*ppLock = new CShareDataLockCounter();
		}
	}
	return count;
}

void CShareDataLockCounter::WaitLock( HWND hwndParent, CShareDataLockCounter** ppLock ){
	if( 0 < GetCountIf0Lock(ppLock) ){
		DWORD dwTime = ::GetTickCount();
		CWaitCursor cWaitCursor(hwndParent);
		CLockCancel* pDlg = nullptr;
		HWND hwndCancel = nullptr;
		::EnableWindow(hwndParent, FALSE);
		while( 0 < GetCountIf0Lock(ppLock) ){
			DWORD dwResult = MsgWaitForMultipleObjects(0, nullptr, FALSE, 100, QS_ALLEVENTS);
			if( dwResult == 0xFFFFFFFF ){
				break;
			}
			if( !BlockingHook( hwndCancel ) ){
				break;
			}
			if( nullptr == pDlg ){
				DWORD dwTimeNow = ::GetTickCount();
				if( 2000 < dwTimeNow - dwTime ){
					pDlg = new CLockCancel();
					hwndCancel = pDlg->DoModeless(::GetModuleHandle( nullptr ), hwndParent, IDD_OPERATIONRUNNING);
				}
			}
		}
		if( pDlg ){
			pDlg->CloseDialog(0);
			delete pDlg;
		}
		::EnableWindow(hwndParent, TRUE);
	}
}
