﻿/*!	@file
	@brief キーボードマクロ(直接実行用)

	@author genta
	@date Sep. 29, 2001 作成
*/
/*
	Copyright (C) 2001, genta
	Copyright (C) 2002, YAZAKI, genta
	Copyright (C) 2005, FILE
	Copyright (C) 2007, ryoji
	Copyright (C) 2018-2022, Sakura Editor Organization

	SPDX-License-Identifier: Zlib
*/

#ifndef SAKURA_CSMACROMGR_9F01D007_5F13_4963_887B_B37E861070DA_H_
#define SAKURA_CSMACROMGR_9F01D007_5F13_4963_887B_B37E861070DA_H_
#pragma once

#include <Windows.h>
#include <WTypes.h> //VARTYPE

#include "CMacroManagerBase.h"
#include "env/DLLSHAREDATA.h"
#include "config/maxdata.h"
#include "util/design_template.h"

class CEditView;

const int STAND_KEYMACRO	= -1;	//!< 標準マクロ(キーマクロ)
const int TEMP_KEYMACRO		= -2;	//!< 一時マクロ(名前を指定してマクロ実行)
const int INVALID_MACRO_IDX	= -3;	//!< 無効なマクロのインデックス番号 @date Sep. 15, 2005 FILE

struct MacroFuncInfoEx
{
	int			m_nArgMinSize;
	int			m_nArgMaxSize;
	VARTYPE*	m_pVarArgEx;
};

//マクロ関数情報構造体
//	関数名はCSMacroMgrが持つ
struct MacroFuncInfo {
	int				m_nFuncID;
	const WCHAR*	m_pszFuncName;
	VARTYPE			m_varArguments[4];	//!< 引数の型の配列
	VARTYPE			m_varResult;		//!< 戻り値の型 VT_EMPTYならprocedureということで
	MacroFuncInfoEx*	m_pData;
};
//マクロ関数情報構造体配列
typedef MacroFuncInfo* MacroFuncInfoArray;

/*-----------------------------------------------------------------------
クラスの宣言

@date 2002.2.17 YAZAKI CShareDataのインスタンスは、CProcessにひとつあるのみ。
-----------------------------------------------------------------------*/
class CSMacroMgr
{
	//	データの型宣言
	CMacroManagerBase* m_cSavedKeyMacro[MAX_CUSTMACRO];	//	キーマクロをカスタムメニューの数だけ管理
	//	Jun. 16, 2002 genta
	//	キーマクロに標準マクロ以外のマクロを読み込めるように
	CMacroManagerBase* m_pKeyMacro;	//	標準の（保存ができる）キーマクロも管理

	//　一時マクロ（名前を指定してマクロ実行）を管理
	CMacroManagerBase* m_pTempMacro;

public:

	/*
	||  Constructors
	*/
	CSMacroMgr();
	~CSMacroMgr();

	/*
	||  Attributes & Operations
	*/
	void Clear( int idx );
	void ClearAll( void );	/* キーマクロのバッファをクリアする */

	//! キーボードマクロの実行
	BOOL Exec( int idx, HINSTANCE hInstance, CEditView* pcEditView, int flags );
	
	//!	実行可能か？CShareDataに問い合わせ
	bool IsEnabled(int idx) const {
		return ( 0 <= idx && idx < MAX_CUSTMACRO ) ?
		m_pShareData->m_Common.m_sMacro.m_MacroTable[idx].IsEnabled() : false;
	}
	
	//!	表示する名前の取得
	const WCHAR* GetTitle(int idx) const
	{
		return ( 0 <= idx && idx < MAX_CUSTMACRO ) ?
		m_pShareData->m_Common.m_sMacro.m_MacroTable[idx].GetTitle() : nullptr;	// 2007.11.02 ryoji
	}
	
	//!	表示名の取得
	const WCHAR* GetName(int idx) const
	{
		return ( 0 <= idx && idx < MAX_CUSTMACRO ) ?
		m_pShareData->m_Common.m_sMacro.m_MacroTable[idx].m_szName : nullptr;
	}
	
	/*!	@brief ファイル名の取得
	
		@param idx [in] マクロ番号
	*/
	const WCHAR* GetFile(int idx) const
	{
		return ( 0 <= idx && idx < MAX_CUSTMACRO ) ?
		m_pShareData->m_Common.m_sMacro.m_MacroTable[idx].m_szFile : 
		( (idx == STAND_KEYMACRO || idx == TEMP_KEYMACRO) && m_sMacroPath.length() ) ?
		m_sMacroPath.c_str() : nullptr;
	}

	/*! キーボードマクロの読み込み */
	BOOL Load( int idx, HINSTANCE hInstance, const WCHAR* pszPath, const WCHAR* pszType );
	BOOL Save( int idx, HINSTANCE hInstance, const WCHAR* pszPath );
	void UnloadAll(void);

	/*! キーマクロのバッファにデータ追加 */
	int Append( int idx, EFunctionCode nFuncID, const LPARAM* lParams, CEditView* pcEditView );

	/*
	||  Attributes & Operations
	*/
	static WCHAR* GetFuncInfoByID( HINSTANCE hInstance, int nFuncID, WCHAR* pszFuncName, WCHAR* pszFuncNameJapanese );	/* 機能ID→関数名，機能名日本語 */
	static EFunctionCode GetFuncInfoByName( HINSTANCE hInstance, const WCHAR* pszFuncName, WCHAR* pszFuncNameJapanese );	/* 関数名→機能ID，機能名日本語 */
	static BOOL CanFuncIsKeyMacro( int nFuncID );	/* キーマクロに記録可能な機能かどうかを調べる */
	
	//	Jun. 16, 2002 genta
	static const MacroFuncInfo* GetFuncInfoByID( int nFuncID );
	
	bool IsSaveOk(void);

	//	Sep. 15, 2005 FILE	実行中マクロのインデックス番号操作 (INVALID_MACRO_IDX:無効)
	int GetCurrentIdx( void ) const {
		return m_CurrentIdx;
	}
	int SetCurrentIdx( int idx ) {
		int oldIdx = m_CurrentIdx;
		m_CurrentIdx = idx;
		return oldIdx;
	}

	//  Oct. 22, 2008 syat 一時マクロ導入
	CMacroManagerBase* SetTempMacro( CMacroManagerBase *newMacro );

private:
	DLLSHAREDATA*	m_pShareData;
	CMacroManagerBase** Idx2Ptr(int idx);

	/*!	実行中マクロのインデックス番号 (INVALID_MACRO_IDX:無効)
		@date Sep. 15, 2005 FILE
	*/
	int m_CurrentIdx;

	std::wstring	m_sMacroPath;	// Loadしたマクロ名

public:
	static MacroFuncInfo	m_MacroFuncInfoCommandArr[];	// コマンド情報(戻り値なし)
	static MacroFuncInfo	m_MacroFuncInfoArr[];		// 関数情報(戻り値あり)

	DISALLOW_COPY_AND_ASSIGN(CSMacroMgr);
};
#endif /* SAKURA_CSMACROMGR_9F01D007_5F13_4963_887B_B37E861070DA_H_ */
