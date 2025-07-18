﻿/*!	@file
	@brief プラグイン管理クラス

*/
/*
	Copyright (C) 2009, syat
	Copyright (C) 2018-2022, Sakura Editor Organization

	SPDX-License-Identifier: Zlib
*/
#include "StdAfx.h"
#include "plugin/CPluginManager.h"
#include "plugin/CJackManager.h"
#include "plugin/CWSHPlugin.h"
#include "plugin/CDllPlugin.h"
#include "util/module.h"
#include "io/CZipFile.h"
#include "CSelectLang.h"
#include "String_define.h"

//コンストラクタ
CPluginManager::CPluginManager()
{
	//pluginsフォルダーの場所を取得
	WCHAR szPluginPath[_MAX_PATH];
	GetInidir( szPluginPath, L"plugins\\" );	//iniと同じ階層のpluginsフォルダーを検索
	m_sBaseDir.append(szPluginPath);

	//Exeフォルダー配下pluginsフォルダーのパスを取得
	WCHAR	szPath[_MAX_PATH];
	WCHAR	szFolder[_MAX_PATH];
	WCHAR	szFname[_MAX_PATH];

	::GetModuleFileName( nullptr, szPath, _countof(szPath)	);
	SplitPath_FolderAndFile(szPath, szFolder, szFname);
	Concat_FolderAndFile(szFolder, L"plugins\\", szPluginPath);

	m_sExePluginDir.append(szPluginPath);
}

//全プラグインを解放する
void CPluginManager::UnloadAllPlugin()
{
	for( CPlugin::ListIter it = m_plugins.begin(); it != m_plugins.end(); it++ ){
		UnRegisterPlugin( *it );
	}

	for( CPlugin::ListIter it = m_plugins.begin(); it != m_plugins.end(); it++ ){
		delete *it;
	}
	
	// 2010.08.04 Moca m_plugins.claerする
	m_plugins.clear();
}

//新規プラグインを追加する
bool CPluginManager::SearchNewPlugin( CommonSetting& common, HWND hWndOwner )
{
	DEBUG_TRACE(L"Enter SearchNewPlugin\n");

	HANDLE hFind;
	CZipFile	cZipFile;

	//プラグインフォルダーの配下を検索
	WIN32_FIND_DATA wf;
	hFind = FindFirstFile( (m_sBaseDir + L"*").c_str(), &wf );
	if (hFind == INVALID_HANDLE_VALUE) {
		//プラグインフォルダーが存在しない
		if (!CreateDirectory(m_sBaseDir.c_str(), nullptr)) {
			InfoMessage( hWndOwner, L"%s", LS(STR_PLGMGR_FOLDER));
			return true;
		}
	}
	::FindClose(hFind);

	bool	bCancel = false;
	//プラグインフォルダーの配下を検索
	bool bFindNewDir = SearchNewPluginDir(common, hWndOwner, m_sBaseDir, bCancel);
	if (!bCancel && m_sBaseDir != m_sExePluginDir) {
		bFindNewDir |= SearchNewPluginDir(common, hWndOwner, m_sExePluginDir, bCancel);
	}
	if (!bCancel && cZipFile.IsOk()) {
		bFindNewDir |= SearchNewPluginZip(common, hWndOwner, m_sBaseDir, bCancel);
		if (!bCancel && m_sBaseDir != m_sExePluginDir) {
			bFindNewDir |= SearchNewPluginZip(common, hWndOwner, m_sExePluginDir, bCancel);
		}
	}

	if (bCancel) {
		InfoMessage( hWndOwner, L"%s", LS(STR_PLGMGR_CANCEL));
	}
	else if (!bFindNewDir) {
		InfoMessage( hWndOwner, L"%s", LS(STR_PLGMGR_NEWPLUGIN));
	}

	return true;
}

//新規プラグインを追加する(下請け)
bool CPluginManager::SearchNewPluginDir( CommonSetting& common, HWND hWndOwner, const std::wstring& sSearchDir, bool& bCancel )
{
	DEBUG_TRACE(L"Enter SearchNewPluginDir\n");

	PluginRec* plugin_table = common.m_sPlugin.m_PluginTable;
	HANDLE hFind;

	WIN32_FIND_DATA wf;
	hFind = FindFirstFile( (sSearchDir + L"*").c_str(), &wf );
	if (hFind == INVALID_HANDLE_VALUE) {
		//プラグインフォルダーが存在しない
		return false;
	}
	bool bFindNewDir = false;
	do {
		if( (wf.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY &&
			(wf.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == 0 &&
			wcscmp(wf.cFileName, L".")!=0 && wcscmp(wf.cFileName, L"..")!=0 &&
			wmemicmp(wf.cFileName, L"unuse") !=0 )
		{
			//インストール済みチェック。フォルダー名＝プラグインテーブルの名前ならインストールしない
			// 2010.08.04 大文字小文字同一視にする
			bool isNotInstalled = true;
			for( int iNo=0; iNo < MAX_PLUGIN; iNo++ ){
				if( wmemicmp( wf.cFileName, plugin_table[iNo].m_szName ) == 0 ){
					isNotInstalled = false;
					break;
				}
			}
			if( !isNotInstalled ){ continue; }

			// 2011.08.20 syat plugin.defが存在しないフォルダーは飛ばす
			if( ! IsFileExists( (sSearchDir + wf.cFileName + L"\\" + PII_FILENAME).c_str(), true ) ){
				continue;
			}

			bFindNewDir = true;
			int nRes = Select3Message( hWndOwner, LS(STR_PLGMGR_INSTALL), wf.cFileName );
			if (nRes == IDYES) {
				std::wstring errMsg;
				int pluginNo = InstallPlugin( common, wf.cFileName, hWndOwner, errMsg );
				if( pluginNo < 0 ){
					WarningMessage( hWndOwner, LS(STR_PLGMGR_INSTALL_ERR),
						wf.cFileName, errMsg.c_str() );
				}
			}
			else if (nRes == IDCANCEL) {
				bCancel = true;
				break;	// for loop
			}
		}
	} while( FindNextFile( hFind, &wf ));

	FindClose( hFind );
	return bFindNewDir;
}

//新規プラグインを追加する(下請け)Zip File
bool CPluginManager::SearchNewPluginZip( CommonSetting& common, HWND hWndOwner, const std::wstring& sSearchDir, bool& bCancel )
{
	DEBUG_TRACE(L"Enter SearchNewPluginZip\n");

	HANDLE hFind;

	WIN32_FIND_DATA wf;
	bool		bNewPlugin = false;
	bool		bFound;
	CZipFile	cZipFile;

	hFind = INVALID_HANDLE_VALUE;

	// Zip File 検索解凍
	if (cZipFile.IsOk()) {
		hFind = FindFirstFile( (sSearchDir + L"*.zip").c_str(), &wf );

		for (bFound = (hFind != INVALID_HANDLE_VALUE); bFound; bFound = (FindNextFile( hFind, &wf ) != 0)) {
			if( (wf.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN)) == 0)
			{
				bNewPlugin |= InstZipPluginSub(common, hWndOwner, sSearchDir + wf.cFileName, wf.cFileName, true, bCancel);
				if (bCancel) {
					break;
				}
			}
		}
	}

	FindClose( hFind );
	return bNewPlugin;
}

//Zipプラグインを導入する
bool CPluginManager::InstZipPlugin( CommonSetting& common, HWND hWndOwner, const std::wstring& sZipFile, bool bInSearch )
{
	DEBUG_TRACE(L"Entry InstZipPlugin\n");

	CZipFile		cZipFile;
	WCHAR			msg[512];

	// ZIPファイルが扱えるか
	if (!cZipFile.IsOk()) {
		wcsncpy_s( msg, _countof(msg), LS(STR_PLGMGR_ERR_ZIP), _TRUNCATE );
		InfoMessage( hWndOwner, L"%s", msg);
		return false;
	}

	//プラグインフォルダーの存在を確認
	WIN32_FIND_DATA wf;
	HANDLE		hFind;
	if ((hFind = ::FindFirstFile( (m_sBaseDir + L"*").c_str(), &wf )) == INVALID_HANDLE_VALUE) {
		//プラグインフォルダーが存在しない
		if (m_sBaseDir == m_sExePluginDir) {
			InfoMessage( hWndOwner, LS(STR_PLGMGR_ERR_FOLDER));
			::FindClose(hFind);
			return false;
		}
		else {
			if (!CreateDirectory(m_sBaseDir.c_str(), nullptr)) {
				WarningMessage( hWndOwner, LS(STR_PLGMGR_ERR_CREATEDIR) );
				::FindClose(hFind);
				return false;
			}
		}
	}
	::FindClose(hFind);

	bool	bCancel;
	return CPluginManager::InstZipPluginSub( common, hWndOwner, sZipFile, sZipFile, false, bCancel );
}

//Zipプラグインを導入する(下請け)
bool CPluginManager::InstZipPluginSub( CommonSetting& common, HWND hWndOwner, const std::wstring& sZipFile, const std::wstring& sDispName, bool bInSearch, bool& bCancel )
{
	PluginRec*		plugin_table = common.m_sPlugin.m_PluginTable;
	CZipFile		cZipFile;
	std::wstring	sFolderName;
	WCHAR			msg[512];
	std::wstring	errMsg;
	bool			bOk = true;
	bool			bSkip = false;
	bool			bNewPlugin = false;

	// Plugin フォルダー名の取得,定義ファイルの確認
	if (bOk && !cZipFile.SetZip(sZipFile)) {
		auto_snprintf_s( msg, _countof(msg), LS(STR_PLGMGR_INST_ZIP_ACCESS), sDispName.c_str() );
		bOk = false;
		bSkip = bInSearch;
	}

	// Plgin フォルダー名の取得,定義ファイルの確認
	if (bOk && !cZipFile.ChkPluginDef(PII_FILENAME, sFolderName)) {
		auto_snprintf_s( msg, _countof(msg), LS(STR_PLGMGR_INST_ZIP_DEF), sDispName.c_str() );
		bOk = false;
		bSkip = bInSearch;
	}

	if (!bInSearch) {
		// 単独インストール
		//インストール済みチェック。
		bool	isNotInstalled = true;
		int		iNo;
		if (bOk) {
			for( iNo=0; iNo < MAX_PLUGIN; iNo++ ){
				if( wmemicmp( sFolderName.c_str(), plugin_table[iNo].m_szName ) == 0 ){
					isNotInstalled = false;
					break;
				}
			}
			if (isNotInstalled) {
				bNewPlugin = true;
			}
			else {
				if( ConfirmMessage( hWndOwner, LS(STR_PLGMGR_INST_ZIP_ALREADY),
						sDispName.c_str() ) != IDYES ){
					// Yesで無いなら終了
					return false;
				}
			}
		}
	}
	else {
		// pluginsフォルダー検索中
		// フォルダー チェック。すでに解凍されていたならインストールしない(前段でインストール済み或は可否を確認済み)
		if (bOk && (fexist((m_sBaseDir + sFolderName).c_str())
			|| fexist((m_sExePluginDir + sFolderName).c_str())) ) {
			bOk = false;
			bSkip = true;
		}
		if (bOk) {
			bNewPlugin= true;
			int nRes = Select3Message( hWndOwner, LS(STR_PLGMGR_INST_ZIP_INST),
				sDispName.c_str(), sFolderName.c_str() );
			switch (nRes) {
			case IDCANCEL:
				bCancel = true;
				// through
			case IDNO:
				bOk = false;
				bSkip = true;
				break;
			}
		}
	}

	// Zip解凍
	if (bOk && !cZipFile.Unzip(m_sBaseDir)) {
		auto_snprintf_s( msg, _countof(msg), LS(STR_PLGMGR_INST_ZIP_UNZIP), sDispName.c_str() );
		bOk = false;
	}
	if (bOk) {
		int pluginNo = InstallPlugin( common, sFolderName.c_str(), hWndOwner, errMsg, true );
		if( pluginNo < 0 ){
			auto_snprintf_s( msg, _countof(msg), LS(STR_PLGMGR_INST_ZIP_ERR), sDispName.c_str(), errMsg.c_str() );
			bOk = false;
		}
	}

	if (!bOk && !bSkip) {
		// エラーメッセージ出力
		WarningMessage( hWndOwner, L"%s", msg);
	}

	return bNewPlugin;
}

//プラグインの初期導入をする
//	common			共有設定変数
//	pszPluginName	プラグイン名
//	hWndOwner		
//	errorMsg		エラーメッセージを返す
//	bUodate			すでに登録していた場合、確認せず上書きする
int CPluginManager::InstallPlugin( CommonSetting& common, const WCHAR* pszPluginName, HWND hWndOwner, std::wstring& errorMsg, bool bUpdate )
{
	CDataProfile cProfDef;				//プラグイン定義ファイル

	//プラグイン定義ファイルを読み込む
	cProfDef.SetReadingMode();
	if( !cProfDef.ReadProfile( (m_sBaseDir + pszPluginName + L"\\" + PII_FILENAME).c_str() )
		&& !cProfDef.ReadProfile( (m_sExePluginDir + pszPluginName + L"\\" + PII_FILENAME).c_str() ) ){
		errorMsg = LS(STR_PLGMGR_INST_DEF);
		return -1;
	}

	std::wstring sId;
	cProfDef.IOProfileData( PII_PLUGIN, PII_PLUGIN_ID, sId );
	if( sId.length() == 0 ){
		errorMsg = LS(STR_PLGMGR_INST_ID);
		return -1;
	}
	//2010.08.04 ID使用不可の文字を確認
	//  後々ファイル名やiniで使うことを考えていくつか拒否する
	static const WCHAR szReservedChars[] = L"/\\,[]*?<>&|;:=\" \t";
	for( int x = 0; x < _countof(szReservedChars); ++x ){
		if( sId.npos != sId.find(szReservedChars[x]) ){
			errorMsg = LS(STR_PLGMGR_INST_RESERVE1);
			errorMsg += szReservedChars;
			errorMsg += LS(STR_PLGMGR_INST_RESERVE2);
			return -1;
		}
	}
	if( WCODE::Is09(sId[0]) ){
		errorMsg = LS(STR_PLGMGR_INST_IDNUM);
		return -1;
	}

	//ID重複・テーブル空きチェック
	PluginRec* plugin_table = common.m_sPlugin.m_PluginTable;
	int nEmpty = -1;
	bool isDuplicate = false;
	for( int iNo=0; iNo < MAX_PLUGIN; iNo++ ){
		if( nEmpty == -1 && plugin_table[iNo].m_state == PLS_NONE ){
			nEmpty = iNo;
			// break してはいけない。後ろで同一IDがあるかも
		}
		if( wcscmp( sId.c_str(), plugin_table[iNo].m_szId ) == 0 ){	//ID一致
			if (!bUpdate) {
				const WCHAR* msg = LS(STR_PLGMGR_INST_NAME);
				// 2010.08.04 削除中のIDは元の位置へ追加(復活させる)
				if( plugin_table[iNo].m_state != PLS_DELETED &&
				  ConfirmMessage( hWndOwner, msg, static_cast<const WCHAR*>(pszPluginName), static_cast<const WCHAR*>(plugin_table[iNo].m_szName) ) != IDYES ){
					errorMsg = LS(STR_PLGMGR_INST_USERCANCEL);
					return -1;
				}
			}
			nEmpty = iNo;
			isDuplicate = plugin_table[iNo].m_state != PLS_DELETED;
			break;
		}
	}

	if( nEmpty == -1 ){
		errorMsg = LS(STR_PLGMGR_INST_MAX);
		return -1;
	}

	wcsncpy( plugin_table[nEmpty].m_szName, pszPluginName, MAX_PLUGIN_NAME );
	plugin_table[nEmpty].m_szName[ MAX_PLUGIN_NAME-1 ] = '\0';
	wcsncpy( plugin_table[nEmpty].m_szId, sId.c_str(), MAX_PLUGIN_ID );
	plugin_table[nEmpty].m_szId[ MAX_PLUGIN_ID-1 ] = '\0';
	plugin_table[nEmpty].m_state = isDuplicate ? PLS_UPDATED : PLS_INSTALLED;

	// コマンド数の設定	2010/7/11 Uchi
	int			i;
	WCHAR		szPlugKey[10];
	std::wstring		sPlugCmd;

	plugin_table[nEmpty].m_nCmdNum = 0;
	for (i = 1; i < MAX_PLUG_CMD; i++) {
		auto_sprintf( szPlugKey, L"C[%d]", i);
		sPlugCmd.clear();
		cProfDef.IOProfileData( PII_COMMAND, szPlugKey, sPlugCmd );
		if (sPlugCmd.empty()) {
			break;
		}
		plugin_table[nEmpty].m_nCmdNum = i;
	}

	return nEmpty;
}

//全プラグインを読み込む
bool CPluginManager::LoadAllPlugin(CommonSetting* common)
{
	DEBUG_TRACE(L"Enter LoadAllPlugin\n");

	CommonSetting_Plugin& pluginSetting = (common ? common->m_sPlugin : GetDllShareData().m_Common.m_sPlugin);

	if( ! pluginSetting.m_bEnablePlugin ) return true;

	std::wstring szLangName;
	{
		std::wstring szDllName = GetDllShareData().m_Common.m_sWindow.m_szLanguageDll;
		if( szDllName.empty() ){
			szLangName = L"ja_JP";
		}else{
			// "sakura_lang_*.dll"
			int nStartPos = 0;
			int nEndPos = szDllName.length();
			if( szDllName.substr( 0, 12 ) == L"sakura_lang_" ){
				nStartPos = 12;
			}
			if( 4 < szDllName.length() && szDllName.substr( szDllName.length() - 4, 4 ) == L".dll" ){
				nEndPos = szDllName.length() - 4;
			}
			szLangName = szDllName.substr( nStartPos, nEndPos - nStartPos );
		}
		DEBUG_TRACE( L"lang = %s\n", szLangName.c_str() );
	}

	//プラグインテーブルに登録されたプラグインを読み込む
	PluginRec* plugin_table = pluginSetting.m_PluginTable;
	for( int iNo=0; iNo < MAX_PLUGIN; iNo++ ){
		if( plugin_table[iNo].m_szName[0] == '\0' ) continue;
		// 2010.08.04 削除状態を見る(今のところ保険)
		if( plugin_table[iNo].m_state == PLS_DELETED ) continue;
		if( nullptr != GetPlugin( iNo ) ) continue; // 2013.05.31 読み込み済み
		std::wstring name = plugin_table[iNo].m_szName;
		CPlugin* plugin = LoadPlugin( m_sBaseDir.c_str(), name.c_str(), szLangName.c_str() );
		if( !plugin ){
			plugin = LoadPlugin( m_sExePluginDir.c_str(), name.c_str(), szLangName.c_str() );
		}
		if( plugin ){
			// 要検討：plugin.defのidとsakuraw.iniのidの不一致処理
			assert_warning( 0 == wcscmp( plugin_table[iNo].m_szId, plugin->m_sId.c_str() ) );
			plugin->m_id = iNo;		//プラグインテーブルの行番号をIDとする
			m_plugins.push_back( plugin );
			plugin_table[iNo].m_state = PLS_LOADED;
			// コマンド数設定
			plugin_table[iNo].m_nCmdNum = plugin->GetCommandCount();

			RegisterPlugin( plugin );
		}
	}
	
	return true;
}

//プラグインを読み込む
CPlugin* CPluginManager::LoadPlugin( const WCHAR* pszPluginDir, const WCHAR* pszPluginName, const WCHAR* pszLangName )
{
	WCHAR pszBasePath[_MAX_PATH];
	WCHAR pszPath[_MAX_PATH];
	std::wstring strMlang;
	CDataProfile cProfDef;				//プラグイン定義ファイル
	CDataProfile cProfDefMLang;			//プラグイン定義ファイル(L10N)
	CDataProfile* pcProfDefMLang = &cProfDefMLang; 
	CDataProfile cProfOption;			//オプションファイル
	CPlugin* plugin = nullptr;

	DEBUG_TRACE(L"Load Plugin %s\n",  pszPluginName );

	//プラグイン定義ファイルを読み込む
	Concat_FolderAndFile( pszPluginDir, pszPluginName, pszBasePath );
	Concat_FolderAndFile( pszBasePath, PII_FILENAME, pszPath );
	cProfDef.SetReadingMode();
	if( !cProfDef.ReadProfile( pszPath ) ){
		//プラグイン定義ファイルが存在しない
		return nullptr;
	}
	DEBUG_TRACE(L"  定義ファイル読込 %s\n",  pszPath );

	//L10N定義ファイルを読む
	//プラグイン定義ファイルを読み込む base\pluginname\local\plugin_en_us.def
	strMlang = pszBasePath;
	strMlang += L"\\" PII_L10NDIR L"\\" PII_L10NFILEBASE;
	strMlang += pszLangName;
	strMlang += PII_L10NFILEEXT;
	cProfDefMLang.SetReadingMode();
	if( !cProfDefMLang.ReadProfile( strMlang.c_str() ) ){
		//プラグイン定義ファイルが存在しない
		pcProfDefMLang = nullptr;
		DEBUG_TRACE(L"  L10N定義ファイル読込 %s Not Found\n",  strMlang.c_str() );
	}else{
		DEBUG_TRACE(L"  L10N定義ファイル読込 %s\n",  strMlang.c_str() );
	}

	std::wstring sPlugType;
	cProfDef.IOProfileData( PII_PLUGIN, PII_PLUGIN_PLUGTYPE, sPlugType );

	if( _wcsicmp( sPlugType.c_str(), L"wsh" ) == 0 ){
		plugin = new CWSHPlugin( std::wstring(pszBasePath) );
	}else if( _wcsicmp( sPlugType.c_str(), L"dll" ) == 0 ){
		plugin = new CDllPlugin( std::wstring(pszBasePath) );
	}else{
		return nullptr;
	}
	plugin->m_sOptionDir = m_sBaseDir + pszPluginName;
	plugin->m_sLangName = pszLangName;
	plugin->ReadPluginDef( &cProfDef, pcProfDefMLang );
	DEBUG_TRACE(L"  プラグインタイプ %ls\n", sPlugType.c_str() );

	//オプションファイルを読み込む
	cProfOption.SetReadingMode();
	if( cProfOption.ReadProfile( plugin->GetOptionPath().c_str() ) ){
		//オプションファイルが存在する場合、読み込む
		plugin->ReadPluginOption( &cProfOption );
	}
	DEBUG_TRACE(L"  オプションファイル読込 %s\n",  plugin->GetOptionPath().c_str() );

	return plugin;
}

//プラグインをCJackManagerに登録する
bool CPluginManager::RegisterPlugin( CPlugin* plugin )
{
	CJackManager* pJackMgr = CJackManager::getInstance();
	CPlug::Array plugs = plugin->GetPlugs();

	for( CPlug::ArrayIter plug = plugs.begin() ; plug != plugs.end(); plug++ ){
		pJackMgr->RegisterPlug( (*plug)->m_sJack.c_str(), *plug );
	}

	return true;
}

//プラグインのCJackManagerの登録を解除する
bool CPluginManager::UnRegisterPlugin( CPlugin* plugin )
{
	CJackManager* pJackMgr = CJackManager::getInstance();
	CPlug::Array plugs = plugin->GetPlugs();

	for( CPlug::ArrayIter plug = plugs.begin() ; plug != plugs.end(); plug++ ){
		pJackMgr->UnRegisterPlug( (*plug)->m_sJack.c_str(), *plug );
	}

	return true;
}

//プラグインを取得する
CPlugin* CPluginManager::GetPlugin( int id )
{
	for( CPlugin::ListIter plugin = m_plugins.begin() ; plugin != m_plugins.end(); plugin++ ){
		if( (*plugin)->m_id == id ) return *plugin;
	}
	return nullptr;
}

//プラグインを削除する
void CPluginManager::UninstallPlugin( CommonSetting& common, int id )
{
	PluginRec* plugin_table = common.m_sPlugin.m_PluginTable;

	// 2010.08.04 ここではIDを保持する。後で再度追加するときに同じ位置に追加
	// PLS_DELETEDのm_szId/m_szNameはiniを保存すると削除されます
//	plugin_table[id].m_szId[0] = '\0';
	plugin_table[id].m_szName[0] = '\0';
	plugin_table[id].m_state = PLS_DELETED;
	plugin_table[id].m_nCmdNum = 0;
}
