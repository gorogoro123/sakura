﻿/*!	@file
	@brief タイプ別設定インポート確認ダイアログ

	@author Uchi
	@date 2010/4/17 新規作成
*/
/*
	Copyright (C) 2010, Uchi
	Copyright (C) 2018-2022, Sakura Editor Organization

	SPDX-License-Identifier: Zlib
*/
#include "StdAfx.h"
#include "CDlgTypeAscertain.h"
#include "env/CDocTypeManager.h"
#include "util/shell.h"
#include "util/window.h"
#include "apiwrap/StdControl.h"
#include "CSelectLang.h"
#include "env/DLLSHAREDATA.h"
#include "sakura.hh"
#include "sakura_rc.h"
#include "String_define.h"

// タイプ別設定インポート確認 CDlgTypeAscertain.cpp
const DWORD p_helpids[] = {
	IDC_RADIO_TYPE_TO,		HIDC_RADIO_TYPE_TO,		//タイプ別名
	IDC_RADIO_TYPE_ADD,		HIDC_RADIO_TYPE_ADD,	//タイプ別追加
	IDC_COMBO_COLORS,		HIDC_COMBO_COLORS,		//色指定
	IDOK,					HIDOK_DTA,				//OK
	IDCANCEL,				HIDCANCEL_DTA,			//キャンセル
	IDC_BUTTON_HELP,		HIDC_DTA_BUTTON_HELP,	//ヘルプ
//	IDC_STATIC,				-1,
	0, 0
};

//  Constructors
CDlgTypeAscertain::CDlgTypeAscertain()
	: m_psi(nullptr)
{
}

// モーダルダイアログの表示
int CDlgTypeAscertain::DoModal( HINSTANCE hInstance, HWND hwndParent, SAscertainInfo* psAscertainInfo )
{
	m_psi = psAscertainInfo;

	m_psi->nColorType = -1;

	return (int)CDialog::DoModal( hInstance, hwndParent, IDD_TYPE_ASCERTAIN, (LPARAM)nullptr );
}

// ボタンクリック
BOOL CDlgTypeAscertain::OnBnClicked( int wID )
{
	switch( wID ){
	case IDC_BUTTON_HELP:
		/* 「タイプ別設定インポート」のヘルプ */
		MyWinHelp( GetHwnd(), HELP_CONTEXT, HLP000338 );
		return TRUE;
	case IDOK:
		WCHAR	buff1[_MAX_PATH + 20];
		wchar_t	buff2[_MAX_PATH + 20];

		m_psi->bAddType = IsDlgButtonCheckedBool( GetHwnd(), IDC_RADIO_TYPE_ADD );
		m_psi->sColorFile.clear();
		m_psi->nColorType = Combo_GetCurSel( GetItemHwnd( IDC_COMBO_COLORS ) ) - 1;
		if (m_psi->nColorType >= MAX_TYPES && Combo_GetLBText( GetItemHwnd( IDC_COMBO_COLORS ), m_psi->nColorType + 1, buff1)) {
			if (swscanf( buff1, L"File -- %ls", buff2 ) > 0) {
				m_psi->sColorFile = buff2;
				m_psi->nColorType = MAX_TYPES;
			}
		}
		::EndDialog( GetHwnd(), TRUE );
		return TRUE;
	case IDCANCEL:
		::EndDialog( GetHwnd(), FALSE );
		return TRUE;
	}
	/* 基底クラスメンバ */
	return CDialog::OnBnClicked( wID );
}

/* ダイアログデータの設定 */
void CDlgTypeAscertain::SetData( void )
{
	// タイプ名設定
	std::wstring typeNameTo = m_psi->sTypeNameTo + L"(&B)";
	::SetWindowText( GetItemHwnd( IDC_RADIO_TYPE_TO    ), typeNameTo.c_str() );
	::SetWindowText( GetItemHwnd( IDC_STATIC_TYPE_FILE ), m_psi->sTypeNameFile.c_str() );

	::CheckDlgButton( GetHwnd(), IDC_RADIO_TYPE_ADD, TRUE );

	int		nIdx;
	HWND	hwndCombo;
	WCHAR	szText[_MAX_PATH + 10];
	hwndCombo = GetItemHwnd( IDC_COMBO_COLORS );
	/* コンボボックスを空にする */
	Combo_ResetContent( hwndCombo );
	/* 一行目はそのまま */
	Combo_AddString( hwndCombo, LS(STR_DLGTYPEASC_IMPORT) );

	// エディタ内の設定
	for (nIdx = 0; nIdx < GetDllShareData().m_nTypesCount; ++nIdx) {
		const STypeConfigMini* type = nullptr;
		if( !CDocTypeManager().GetTypeConfigMini( CTypeConfig(nIdx), &type ) ){
			continue;
		}
		if (type->m_szTypeExts[0] != L'\0' ) {		/* タイプ属性：拡張子リスト */
			auto_sprintf( szText, L"%s (%s)",
				type->m_szTypeName,	/* タイプ属性：名称 */
				type->m_szTypeExts	/* タイプ属性：拡張子リスト */
			);
		}
		else{
			auto_sprintf( szText, L"%s",
				type->m_szTypeName	/* タイプ属性：拡称 */
			);
		}
		::Combo_AddString( hwndCombo, szText );
	}
	// 読込色設定ファイル設定
	HANDLE	hFind;
	WIN32_FIND_DATA	wf;
	BOOL	bFind;
	WCHAR	sTrgCol[_MAX_PATH + 1];

	::SplitPath_FolderAndFile( m_psi->sImportFile.c_str(), sTrgCol, nullptr );
	wcscat( sTrgCol, L"\\*.col" );
	for (bFind = ( ( hFind = FindFirstFile( sTrgCol, &wf ) ) != INVALID_HANDLE_VALUE );
		bFind;
		bFind = FindNextFile( hFind, &wf )) {
		if ( (wf.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
			// 読込色設定ファイル発見
			auto_sprintf( szText, L"File -- %s", wf.cFileName );
			::Combo_AddString( hwndCombo, szText );
		}
	}
	FindClose( hFind );

	// コンボボックスのデフォルト選択
	Combo_SetCurSel( hwndCombo, 0 );
	return;
}

LPVOID CDlgTypeAscertain::GetHelpIdTable(void)
{
	return (LPVOID)p_helpids;
}
