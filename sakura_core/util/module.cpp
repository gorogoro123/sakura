﻿/*! @file */
/*
	Copyright (C) 2008, kobake
	Copyright (C) 2018-2022, Sakura Editor Organization

	SPDX-License-Identifier: Zlib
*/

#include "StdAfx.h"
#include "module.h"
#include "util/os.h"
#include "util/file.h"
#include <Shlwapi.h>	// 2006.06.17 ryoji

/*! 
	カレントディレクトリを実行ファイルの場所に移動
	@date 2010.08.28 Moca 新規作成
*/
void ChangeCurrentDirectoryToExeDir()
{
	WCHAR szExeDir[_MAX_PATH];
	szExeDir[0] = L'\0';
	GetExedir( szExeDir, nullptr );
	if( szExeDir[0] ){
		::SetCurrentDirectory( szExeDir );
	}else{
		// 移動できないときはSYSTEM32(9xではSYSTEM)に移動
		szExeDir[0] = L'\0';
		int n = ::GetSystemDirectory( szExeDir, _MAX_PATH );
		if( n && n < _MAX_PATH ){
			::SetCurrentDirectory( szExeDir );
		}
	}
}

/*! 
	@date 2010.08.28 Moca 新規作成
*/
HMODULE LoadLibraryExedir(LPCWSTR pszDll)
{
	CCurrentDirectoryBackupPoint dirBack;
	// DLL インジェクション対策としてEXEのフォルダーに移動する
	ChangeCurrentDirectoryToExeDir();
	return ::LoadLibrary( pszDll );
}

/*!	シェルやコモンコントロール DLL のバージョン番号を取得

	@param[in] lpszDllName DLL ファイルのパス
	@return DLL のバージョン番号（失敗時は 0）

	@author ? (from MSDN Library document)
	@date 2006.06.17 ryoji MSDNライブラリから引用
*/
DWORD GetDllVersion(LPCWSTR lpszDllName)
{
	HINSTANCE hinstDll;
	DWORD dwVersion = 0;

	/* For security purposes, LoadLibrary should be provided with a
	   fully-qualified path to the DLL. The lpszDllName variable should be
	   tested to ensure that it is a fully qualified path before it is used. */
	hinstDll = LoadLibraryExedir(lpszDllName);

	if(hinstDll)
	{
		DLLGETVERSIONPROC pDllGetVersion;
		pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstDll,
						  "DllGetVersion");

		/* Because some DLLs might not implement this function, you
		must test for it explicitly. Depending on the particular
		DLL, the lack of a DllGetVersion function can be a useful
		indicator of the version. */

		if(pDllGetVersion)
		{
			DLLVERSIONINFO dvi;
			HRESULT hr;

			ZeroMemory(&dvi, sizeof(dvi));
			dvi.cbSize = sizeof(dvi);

			hr = (*pDllGetVersion)(&dvi);

			if(SUCCEEDED(hr))
			{
			   dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
			}
		}

		FreeLibrary(hinstDll);
	}
	return dwVersion;
}

/*!
	@brief アプリケーションアイコンの取得
	
	アイコンファイルが存在する場合はそこから，無い場合は
	リソースファイルから取得する
	
	@param hInst [in] Instance Handle
	@param nResource [in] デフォルトアイコン用Resource ID
	@param szFile [in] アイコンファイル名
	@param bSmall [in] true: small icon (SM_CXSMICON) / false: large icon (SM_CXICON)
	
	@return アイコンハンドル．失敗した場合はNULL．
	
	@date 2002.12.02 genta 新規作成
	@date 2007.05.20 ryoji iniファイルパスを優先
	@author genta
*/
HICON GetAppIcon( HINSTANCE hInst, int nResource, const WCHAR* szFile, bool bSmall )
{
	// サイズの設定
	int size = GetSystemMetrics( bSmall ? SM_CXSMICON : SM_CXICON );

	WCHAR szPath[_MAX_PATH];
	HICON hIcon;

	// ファイルからの読み込みをまず試みる
	GetInidirOrExedir( szPath, szFile );

	hIcon = (HICON)::LoadImage(
		nullptr,
		szPath,
		IMAGE_ICON,
		size,
		size,
		LR_SHARED | LR_LOADFROMFILE
	);
	if( hIcon != nullptr ){
		return hIcon;
	}

	//	ファイルからの読み込みに失敗したらリソースから取得
	hIcon = (HICON)::LoadImage(
		hInst,
		MAKEINTRESOURCE(nResource),
		IMAGE_ICON,
		size,
		size,
		LR_SHARED
	);
	
	return hIcon;
}

struct VS_VERSION_INFO_HEAD {
	WORD	wLength;
	WORD	wValueLength;
	WORD	bText;
	WCHAR	szKey[16];
	VS_FIXEDFILEINFO Value;
};

/*! リソースから製品バージョンの取得
	@date 2004.05.13 Moca 一度取得したらキャッシュする
*/
void GetAppVersionInfo(
	HINSTANCE	hInstance,
	int			nVersionResourceID,
	DWORD*		pdwProductVersionMS,
	DWORD*		pdwProductVersionLS
)
{
	HRSRC					hRSRC;
	HGLOBAL					hgRSRC;
	VS_VERSION_INFO_HEAD*	pVVIH;
	/* リソースから製品バージョンの取得 */
	*pdwProductVersionMS = 0;
	*pdwProductVersionLS = 0;
	static bool bLoad = false;
	static DWORD dwVersionMS = 0;
	static DWORD dwVersionLS = 0;
	if( hInstance == nullptr && bLoad ){
		*pdwProductVersionMS = dwVersionMS;
		*pdwProductVersionLS = dwVersionLS;
		return;
	}
	if( nullptr != ( hRSRC = ::FindResource( hInstance, MAKEINTRESOURCE(nVersionResourceID), RT_VERSION ) )
	 && nullptr != ( hgRSRC = ::LoadResource( hInstance, hRSRC ) )
	 && nullptr != ( pVVIH = (VS_VERSION_INFO_HEAD*)::LockResource( hgRSRC ) )
	){
		*pdwProductVersionMS = pVVIH->Value.dwProductVersionMS;
		*pdwProductVersionLS = pVVIH->Value.dwProductVersionLS;
		if( hInstance == nullptr ){
			dwVersionMS = pVVIH->Value.dwProductVersionMS;
			dwVersionLS = pVVIH->Value.dwProductVersionLS;
			bLoad = true;
		}
	}
	return;
}
