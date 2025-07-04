﻿/*!	@file
	@brief プラグイン基本クラス

*/
/*
	Copyright (C) 2009, syat
	Copyright (C) 2018-2022, Sakura Editor Organization

	SPDX-License-Identifier: Zlib
*/
#include "StdAfx.h"
#include <vector>		// wstring_split用 2010/4/4 Uchi
#include "CPlugin.h"
#include "CJackManager.h"

/////////////////////////////////////////////
// CPlug メンバ関数
bool CPlug::Invoke( CEditView* view, CWSHIfObj::List& params ){
	return m_cPlugin.InvokePlug( view, *this, params );
}

EFunctionCode CPlug::GetFunctionCode() const{
	return GetPluginFunctionCode(m_cPlugin.m_id, m_id);
}

/////////////////////////////////////////////
// CPlugin メンバ関数

//コンストラクタ
CPlugin::CPlugin( const std::wstring& sBaseDir )
	: m_sBaseDir( sBaseDir )
{
	m_nCommandCount = 0;
}

//デストラクタ
CPlugin::~CPlugin(void)
{
	for( CPluginOption::ArrayIter it = m_options.begin(); it != m_options.end(); it++ ){
		delete *it;
	}
}

//プラグイン定義ファイルのCommonセクションを読み込む
bool CPlugin::ReadPluginDefCommon( CDataProfile *cProfile, CDataProfile *cProfileMlang )
{
	cProfile->IOProfileData( PII_PLUGIN, PII_PLUGIN_ID, m_sId );
	cProfile->IOProfileData( PII_PLUGIN, PII_PLUGIN_NAME, m_sName );
	cProfile->IOProfileData( PII_PLUGIN, PII_PLUGIN_DESCRIPTION, m_sDescription );
	cProfile->IOProfileData( PII_PLUGIN, PII_PLUGIN_AUTHOR, m_sAuthor );
	cProfile->IOProfileData( PII_PLUGIN, PII_PLUGIN_VERSION, m_sVersion );
	cProfile->IOProfileData( PII_PLUGIN, PII_PLUGIN_URL, m_sUrl );
	if( cProfileMlang ){
		cProfileMlang->IOProfileData( PII_PLUGIN, PII_PLUGIN_NAME, m_sName );
		cProfileMlang->IOProfileData( PII_PLUGIN, PII_PLUGIN_DESCRIPTION, m_sDescription );
		cProfileMlang->IOProfileData( PII_PLUGIN, PII_PLUGIN_URL, m_sUrl );
	}

	DEBUG_TRACE(L"    Name:%ls\n", m_sName.c_str());
	DEBUG_TRACE(L"    Description:%ls\n", m_sDescription.c_str());
	DEBUG_TRACE(L"    Author:%ls\n", m_sAuthor.c_str());
	DEBUG_TRACE(L"    Version:%ls\n", m_sVersion.c_str());
	DEBUG_TRACE(L"    Url:%ls\n", m_sUrl.c_str());

	return true;
}

//プラグイン定義ファイルのPlugセクションを読み込む
// @date 2011.08.20 syat Plugセクションも複数定義可能とする
bool CPlugin::ReadPluginDefPlug( CDataProfile *cProfile, CDataProfile *cProfileMlang )
{
	unsigned int i;
	std::vector<JackDef> jacks = CJackManager::getInstance()->GetJackDef();
	wchar_t szIndex[8];

	for( i=0; i<jacks.size(); i++ ){
		const std::wstring sKey = jacks[i].szName;
		for( int nCount = 0; nCount < MAX_PLUG_CMD; nCount++ ){
			if( nCount == 0 ){
				szIndex[0] = L'\0';
			}else{
				_swprintf(szIndex, L"[%d]", nCount);
			}
			std::wstring sHandler;
			if( cProfile->IOProfileData( PII_PLUG, (sKey + szIndex).c_str(), sHandler ) ){
				//ラベルの取得
				std::wstring sKeyLabel = sKey + szIndex + L".Label";
				std::wstring sLabel;
				cProfile->IOProfileData( PII_PLUG, sKeyLabel.c_str(), sLabel );
				if( cProfileMlang ){
					cProfileMlang->IOProfileData( PII_PLUG, sKeyLabel.c_str(), sLabel );
				}
				if (sLabel.empty()) {
					sLabel = sHandler;		// Labelが無ければハンドラ名で代用
				}

				CPlug *newPlug = CreatePlug( *this, nCount, jacks[i].szName, sHandler, sLabel );
				m_plugs.push_back( newPlug );
			}else{
				break;		//定義がなければ読み込みを終了
			}
		}
	}

	return true;
}

//プラグイン定義ファイルのCommandセクションを読み込む
bool CPlugin::ReadPluginDefCommand( CDataProfile *cProfile, CDataProfile *cProfileMlang )
{
	std::wstring sHandler;
	WCHAR bufKey[64];

	for( int nCount = 1; nCount < MAX_PLUG_CMD; nCount++ ){	//添え字は１から始める
		_swprintf( bufKey, L"C[%d]", nCount );
		if( cProfile->IOProfileData( PII_COMMAND, bufKey, sHandler ) ){
			std::wstring sLabel;
			std::wstring sIcon;

			//ラベルの取得
			_swprintf( bufKey, L"C[%d].Label", nCount );
			cProfile->IOProfileData( PII_COMMAND, bufKey, sLabel );
			if( cProfileMlang ){
				cProfileMlang->IOProfileData( PII_COMMAND, bufKey, sLabel );
			}
			if (sLabel.empty()) {
				sLabel = sHandler;		// Labelが無ければハンドラ名で代用
			}
			//アイコンの取得
			_swprintf( bufKey, L"C[%d].Icon", nCount );
			cProfile->IOProfileData( PII_COMMAND, bufKey, sIcon );
			if( cProfileMlang ){
				cProfileMlang->IOProfileData( PII_COMMAND, bufKey, sIcon );
			}

			AddCommand( sHandler.c_str(), sLabel.c_str(), sIcon.c_str(), false );
		}else{
			break;		//定義がなければ読み込みを終了
		}
	}

	return true;
}

//プラグイン定義ファイルのOptionセクションを読み込む	// 2010/3/24 Uchi
bool CPlugin::ReadPluginDefOption( CDataProfile *cProfile, CDataProfile *cProfileMlang )
{
	std::wstring sLabel;
	std::wstring sSection;
	std::wstring sSection_wk;
	std::wstring sKey;
	std::wstring sType;
	std::wstring sSelect;
	std::wstring sDefaultVal;
	WCHAR bufKey[64];

	sSection.clear();
	for( int nCount = 1; nCount < MAX_PLUG_OPTION; nCount++ ){	//添え字は１から始める
		sKey.clear();
		sLabel.clear();
		sType.clear();
		sDefaultVal.clear();
		//Keyの取得
		_swprintf( bufKey, L"O[%d].Key", nCount );
		if( cProfile->IOProfileData( PII_OPTION, bufKey, sKey ) ){
			//Sectionの取得
			_swprintf( bufKey, L"O[%d].Section", nCount );
			cProfile->IOProfileData( PII_OPTION, bufKey, sSection_wk );
			if (!sSection_wk.empty()) {		// 指定が無ければ前を引き継ぐ
				sSection = sSection_wk;
			}
			//ラベルの取得
			_swprintf( bufKey, L"O[%d].Label", nCount );
			cProfile->IOProfileData( PII_OPTION, bufKey, sLabel );
			if( cProfileMlang ){
				cProfileMlang->IOProfileData( PII_OPTION, bufKey, sLabel );
			}
			//Typeの取得
			_swprintf( bufKey, L"O[%d].Type", nCount );
			cProfile->IOProfileData( PII_OPTION, bufKey, sType );
			// 項目選択候補
			_swprintf( bufKey, L"O[%d].Select", nCount );
			cProfile->IOProfileData( PII_OPTION, bufKey, sSelect );
			if( cProfileMlang ){
				cProfileMlang->IOProfileData( PII_OPTION, bufKey, sSelect );
			}
			// デフォルト値
			_swprintf( bufKey, L"O[%d].Default", nCount );
			cProfile->IOProfileData( PII_OPTION, bufKey, sDefaultVal );

			if (sSection.empty() || sKey.empty()) {
				// 設定が無かったら無視
				continue;
			}
			if (sLabel.empty()) {
				// Label指定が無ければ、Keyで代用
				sLabel = sKey;
			}

			m_options.push_back( new CPluginOption( this, sLabel, sSection, sKey, sType, sSelect, sDefaultVal, nCount ) );
		}
	}

	return true;
}

//プラグインフォルダー基準の相対パスをフルパスに変換
std::wstring CPlugin::GetFilePath( const std::wstring& sFileName ) const
{
	return m_sBaseDir + L"\\" + sFileName;
}

std::wstring CPlugin::GetFolderName() const
{
	return GetFileTitlePointer(m_sBaseDir.c_str());
}

//コマンドを追加する
int CPlugin::AddCommand( const WCHAR* handler, const WCHAR* label, const WCHAR* icon, bool doRegister )
{
	if( !handler ){ handler = L""; }
	if( !label ){ label = L""; }

	//コマンドプラグIDは1から振る
	m_nCommandCount++;
	CPlug *newPlug = CreatePlug( *this, m_nCommandCount, PP_COMMAND_STR, std::wstring(handler), std::wstring(label) );
	if( icon ){
		newPlug->m_sIcon = icon;
	}

	m_plugs.push_back( newPlug );

	if( doRegister ){
		CJackManager::getInstance()->RegisterPlug( PP_COMMAND_STR, newPlug );
	}
	return newPlug->GetFunctionCode();
}

// 文字列分割	2010/4/4 Uchi
//	独立させたほうがいいのだが
std::vector<std::wstring> wstring_split( std::wstring sTrg, wchar_t cSep )
{
    std::vector<std::wstring>	splitVec;
    int 	idx;

    while ((idx = sTrg.find( cSep )) != std::wstring::npos) {
        splitVec.push_back( sTrg.substr( 0, idx ) );
        sTrg = sTrg.substr( ++idx );
    }
	if (sTrg.length()) {
		splitVec.push_back( sTrg );
	}

    return splitVec;
}

/*!	プラグイン定義ファイルのStringセクションを読み込む
*/
bool CPlugin::ReadPluginDefString( CDataProfile *cProfile, CDataProfile *cProfileMlang )
{
	WCHAR bufKey[64];
	m_aStrings.clear();
	m_aStrings.emplace_back( std::wstring() ); // 0番目ダミー
	for( int nCount = 1; nCount < MAX_PLUG_STRING; nCount++ ){	//添え字は１から始める
		std::wstring sVal = L"";
		_swprintf( bufKey, L"S[%d]", nCount );
		if( cProfile->IOProfileData( PII_STRING, bufKey, sVal ) ){
			if( cProfileMlang ){
				cProfileMlang->IOProfileData( PII_STRING, bufKey, sVal );
			}
		}
		m_aStrings.push_back( sVal );
	}
	return true;
}
