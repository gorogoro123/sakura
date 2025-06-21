﻿/*!	@file
	@brief プラグイン設定ダイアログボックス

	@author Uchi
	@date 2010/3/22
*/
/*
	Copyright (C) 2010, Uchi
	Copyright (C) 2018-2022, Sakura Editor Organization

	SPDX-License-Identifier: Zlib
*/
#ifndef SAKURA_CDLGPLUGINOPTION_AF75E03E_99DB_4AEF_93AF_BFF669310DE6_H_
#define SAKURA_CDLGPLUGINOPTION_AF75E03E_99DB_4AEF_93AF_BFF669310DE6_H_
#pragma once

#include "dlg/CDialog.h"
#include "plugin/CPluginManager.h"

class CPropPlugin;

/*!	@brief 「プラグイン設定」ダイアログ

	共通設定のプラグイン設定で，指定プラグインのオプションを設定するために
	使用されるダイアログボックス
*/

// 編集最大長
#define MAX_LENGTH_VALUE	1024

// 型 
static const std::wstring	OPTION_TYPE_BOOL = std::wstring( L"bool" );
static const std::wstring	OPTION_TYPE_INT  = std::wstring( L"int" );
static const std::wstring	OPTION_TYPE_SEL  = std::wstring( L"sel" );
static const std::wstring	OPTION_TYPE_DIR  = std::wstring( L"dir" );

class CDlgPluginOption final : public CDialog
{
public:
	/*
	||  Constructors
	*/
	CDlgPluginOption();
	~CDlgPluginOption();

	/*
	||  Attributes & Operations
	*/
	int DoModal( HINSTANCE hInstance, HWND hwndParent, CPropPlugin* cPropPlugin, int ID );	/* モーダルダイアログの表示 */

protected:
	/*
	||  実装ヘルパ関数
	*/
	BOOL	OnInitDialog( HWND hwndDlg, WPARAM wParam, LPARAM lParam ) override;
	BOOL	OnBnClicked(int wID) override;
	BOOL	OnNotify(NMHDR* pNMHDR) override;
	BOOL	OnCbnSelChange( HWND hwndCtl, int wID ) override;
	BOOL	OnEnChange( HWND hwndCtl, int wID ) override;
	BOOL	OnActivate( WPARAM wParam, LPARAM lParam ) override;
	LPVOID	GetHelpIdTable( void ) override;

	void	SetData( void ) override;	/* ダイアログデータの設定 */
	int		GetData( void ) override;	/* ダイアログデータの取得 */

	void	ChangeListPosition( void );					// 編集領域をリストビューに合せて切替える
	void	MoveFocusToEdit( void );					// 編集領域にフォーカスを移す
	void	SetToEdit(int iLine);
	void	SetFromEdit(int iLine);
	void	SelectEdit(int IDCenable);							// 編集領域の切り替え
	void	SepSelect(std::wstring sTrg, std::wstring* spView, std::wstring* spValue);	// 選択用文字列分解
	void	SelectDirectory( int iLine );				// ディレクトリを選択する

private:
	CPlugin*		m_cPlugin;
	CPropPlugin*	m_cPropPlugin;
	int 			m_ID;			// プラグイン番号（エディタがふる番号）
	int				m_Line;			// 現在編集中のオプション行番号
	std::wstring	m_sReadMeName;	// ReadMe ファイル名
};
#endif /* SAKURA_CDLGPLUGINOPTION_AF75E03E_99DB_4AEF_93AF_BFF669310DE6_H_ */
