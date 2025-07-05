﻿/*!	@file
	@brief インポート、エクスポートマネージャ

	@author Uchi
	@date 2010/4/22 新規作成
*/
/*
	Copyright (C) 2010, Uchi, Moca
	Copyright (C) 2018-2022, Sakura Editor Organization

	SPDX-License-Identifier: Zlib
*/

#ifndef SAKURA_CIMPEXPMANAGER_12EC6C8E_1661_485E_8972_A7A9AE419BC8_H_
#define SAKURA_CIMPEXPMANAGER_12EC6C8E_1661_485E_8972_A7A9AE419BC8_H_
#pragma once

#include "CDataProfile.h"
#include "env/DLLSHAREDATA.h"

class CImpExpManager
{
public:
	bool ImportUI(HINSTANCE hInstance, HWND hwndParent);
	bool ExportUI(HINSTANCE hInstance, HWND hwndParent);
	virtual bool ImportAscertain(HINSTANCE hInstance, HWND hwndParent, const std::wstring& sFileName, std::wstring& sErrMsg);
	virtual bool Import(const std::wstring& sFileName, std::wstring& sErrMsg) = 0;
	virtual bool Export(const std::wstring& sFileName, std::wstring& sErrMsg) = 0;
	// ファイル名の初期値を設定
	void SetBaseName(const std::wstring& sBase);
	// フルパス名を取得
	inline std::wstring GetFullPath()
	{
		return { LPCWSTR(GetDllShareData().m_sHistory.m_szIMPORTFOLDER) + m_sOriginName };
	}
	// フルパス名を取得
	inline std::wstring MakeFullPath( std::wstring sFileName )
	{
		return { LPCWSTR(GetDllShareData().m_sHistory.m_szIMPORTFOLDER) + sFileName };
	}
	// ファイル名を取得
	inline std::wstring GetFileName()	{ return m_sOriginName; }

protected:
	// Import Folderの設定
	inline void SetImportFolder( const WCHAR* szPath ) 
	{
		/* ファイルのフルパスをフォルダーとファイル名に分割 */
		/* [c:\work\test\aaa.txt] → [c:\work\test] + [aaa.txt] */
		::SplitPath_FolderAndFile( szPath, GetDllShareData().m_sHistory.m_szIMPORTFOLDER, nullptr );
		wcscat( GetDllShareData().m_sHistory.m_szIMPORTFOLDER, L"\\" );
	}

	// デフォルト拡張子の取得(「*.txt」形式)
	virtual const WCHAR* GetDefaultExtension();
	// デフォルト拡張子の取得(「txt」形式)
	virtual const wchar_t* GetOriginExtension();

protected:
	std::wstring		m_sBase;
	std::wstring		m_sOriginName;
};

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                          タイプ別設定                       //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
class CImpExpType : public CImpExpManager
{
public:
	// Constructor
	CImpExpType( int nIdx, STypeConfig& types, HWND hwndList )
		: m_nIdx( nIdx )
		, m_Types( types )
		, m_hwndList( hwndList )
	{
		/* 共有データ構造体のアドレスを返す */
		m_pShareData = &GetDllShareData();
	}

public:
	bool ImportAscertain( HINSTANCE, HWND, const std::wstring&, std::wstring& ) override;
	bool Import( const std::wstring&, std::wstring& ) override;
	bool Export( const std::wstring&, std::wstring& ) override;

public:
	// デフォルト拡張子の取得
	const WCHAR* GetDefaultExtension() override	{ return L"*.ini"; }
	const wchar_t* GetOriginExtension() override	{ return L"ini"; }
	bool IsAddType(){ return m_bAddType; }

private:
	// インターフェース用
	int 			m_nIdx;
	STypeConfig&	m_Types;
	HWND			m_hwndList;

	// 内部使用
	DLLSHAREDATA*	m_pShareData;
	int				m_nColorType;
	std::wstring 	m_sColorFile;
	bool			m_bAddType;
	CDataProfile	m_cProfile;
};

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                          カラー                             //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
class CImpExpColors : public CImpExpManager
{
public:
	// Constructor
	CImpExpColors( ColorInfo * psColorInfoArr )
		: m_ColorInfoArr( psColorInfoArr )
	{
	}

public:
	bool Import( const std::wstring&, std::wstring& ) override;
	bool Export( const std::wstring&, std::wstring& ) override;

public:
	// デフォルト拡張子の取得
	const WCHAR* GetDefaultExtension() override	{ return L"*.col"; }
	const wchar_t* GetOriginExtension() override	{ return L"col"; }

private:
	ColorInfo*		m_ColorInfoArr;
};

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                    正規表現キーワード                       //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
class CImpExpRegex : public CImpExpManager
{
public:
	// Constructor
	CImpExpRegex( STypeConfig& types )
		: m_Types( types )
	{
	}

public:
	bool Import( const std::wstring&, std::wstring& ) override;
	bool Export( const std::wstring&, std::wstring& ) override;

public:
	// デフォルト拡張子の取得
	const WCHAR* GetDefaultExtension() override	{ return L"*.rkw"; }
	const wchar_t* GetOriginExtension() override	{ return L"rkw"; }

private:
	STypeConfig&	m_Types;
};

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                     キーワードヘルプ                        //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
class CImpExpKeyHelp : public CImpExpManager
{
public:
	// Constructor
	CImpExpKeyHelp( STypeConfig& types )
		: m_Types( types )
	{
	}

public:
	bool Import( const std::wstring&, std::wstring& ) override;
	bool Export( const std::wstring&, std::wstring& ) override;

public:
	// デフォルト拡張子の取得
	const WCHAR* GetDefaultExtension() override	{ return L"*.txt"; }
	const wchar_t* GetOriginExtension() override	{ return L"txt"; }

private:
	STypeConfig&	m_Types;
};

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                     キー割り当て                            //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
class CImpExpKeybind : public CImpExpManager
{
public:
	// Constructor
	CImpExpKeybind( CommonSetting& common )
		: m_Common( common )
	{
	}

public:
	bool Import( const std::wstring&, std::wstring& ) override;
	bool Export( const std::wstring&, std::wstring& ) override;

public:
	// デフォルト拡張子の取得
	const WCHAR* GetDefaultExtension() override	{ return L"*.key"; }
	const wchar_t* GetOriginExtension() override	{ return L"key"; }

private:
	CommonSetting&		m_Common;
};

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                     カスタムメニュー                        //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
class CImpExpCustMenu : public CImpExpManager
{
public:
	// Constructor
	CImpExpCustMenu( CommonSetting& common )
		: m_Common( common )
	{
	}

public:
	bool Import( const std::wstring&, std::wstring& ) override;
	bool Export( const std::wstring&, std::wstring& ) override;

public:
	// デフォルト拡張子の取得
	const WCHAR* GetDefaultExtension() override	{ return L"*.mnu"; }
	const wchar_t* GetOriginExtension() override	{ return L"mnu"; }

private:
	CommonSetting&		m_Common;
};

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                     強調キーワード                          //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
class CImpExpKeyWord : public CImpExpManager
{
public:
	// Constructor
	CImpExpKeyWord( CommonSetting& common, int nKeyWordSetIdx, bool& bCase )
		: m_Common( common )
		, m_nIdx( nKeyWordSetIdx )
		, m_bCase( bCase )
	{
	}

public:
	bool Import( const std::wstring&, std::wstring& ) override;
	bool Export( const std::wstring&, std::wstring& ) override;

public:
	// デフォルト拡張子の取得
	const WCHAR* GetDefaultExtension() override	{ return L"*.kwd"; }
	const wchar_t* GetOriginExtension() override	{ return L"kwd"; }

private:
	CommonSetting&		m_Common;
	int 				m_nIdx;
	bool&				m_bCase;
};

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                     メインメニュー                          //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
class CImpExpMainMenu : public CImpExpManager
{
public:
	// Constructor
	CImpExpMainMenu( CommonSetting& common )
		: m_Common( common )
	{
	}

public:
	bool Import( const std::wstring&, std::wstring& ) override;
	bool Export( const std::wstring&, std::wstring& ) override;

public:
	// デフォルト拡張子の取得
	const WCHAR* GetDefaultExtension() override	{ return L"*.ini"; }
	const wchar_t* GetOriginExtension() override	{ return L"ini"; }

private:
	CommonSetting&		m_Common;
};

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                     メインメニュー                          //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
class CImpExpFileTree : public CImpExpManager
{
public:
	// Constructor
	CImpExpFileTree( std::vector<SFileTreeItem>& items )
		: m_aFileTreeItems( items )
	{
	}

public:
	bool Import( const std::wstring&, std::wstring& ) override;
	bool Export( const std::wstring&, std::wstring& ) override;
	static void IO_FileTreeIni( CDataProfile&, std::vector<SFileTreeItem>& );

public:
	// デフォルト拡張子の取得
	const WCHAR* GetDefaultExtension() override	{ return L"*.ini"; }
	const wchar_t* GetOriginExtension() override	{ return L"ini"; }

private:
	std::vector<SFileTreeItem>&		m_aFileTreeItems;
};
#endif /* SAKURA_CIMPEXPMANAGER_12EC6C8E_1661_485E_8972_A7A9AE419BC8_H_ */
