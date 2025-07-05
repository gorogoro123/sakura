﻿/*!	@file
	@brief DLLのロード、アンロード

	@author genta
	@date Jun. 10, 2001
*/
/*
	Copyright (C) 2001, genta
	Copyright (C) 2018-2022, Sakura Editor Organization

	SPDX-License-Identifier: Zlib
*/

#include "StdAfx.h"
#include "CDllHandler.h"
#include "util/module.h"

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                        生成と破棄                           //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

CDllImp::CDllImp()
	: m_hInstance( nullptr )
{
}

/*!
	オブジェクト消滅前にDLLが読み込まれた状態であればDLLの解放を行う．
*/
CDllImp::~CDllImp()
{
	if( IsAvailable() ){
		DeinitDll(true);
	}
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                         DLLロード                           //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

EDllResult CDllImp::InitDll(LPCWSTR pszSpecifiedDllName)
{
	if( IsAvailable() ){
		//	既に利用可能で有れば何もしない．
		return DLL_SUCCESS;
	}

	//名前候補を順次検証し、有効なものを採用する
	LPCWSTR pszLastName  = nullptr;
	bool bInitImpFailure = false;
	for(int i = -1; ;i++)
	{
		//名前候補
		LPCWSTR pszName = nullptr;
		if(i==-1){ //まずは引数で指定された名前から。
			pszName = pszSpecifiedDllName;
		}
		else{ //クラス定義のDLL名
			pszName = GetDllNameImp(i);
			//GetDllNameImpから取得した名前が無効ならループを抜ける
			if(!pszName || !pszName[0]){
				break;
			}
			//GetDllNameImpから取得した名前が前回候補と同じならループを抜ける
			if(pszLastName && _wcsicmp(pszLastName,pszName)==0){
				break;
			}
		}
		pszLastName = pszName;

		//名前が無効の場合は、次の名前候補を試す。
		if(!pszName || !pszName[0])continue;

		//DLLロード。ロードできなかったら次の名前候補を試す。
		m_hInstance = LoadLibraryExedir(pszName);
		if(!m_hInstance)continue;

		//初期処理
		bool ret = InitDllImp();

		//初期処理に失敗した場合はDLLを解放し、次の名前候補を試す。
		if(!ret){
			bInitImpFailure = true;
			::FreeLibrary( m_hInstance );
			m_hInstance = nullptr;
			continue;
		}

		//初期処理に成功した場合は、DLL名を保存し、ループを抜ける
		if(ret){
			m_strLoadedDllName = pszName;
			break;
		}
	}

	//ロードと初期処理に成功なら
	if(IsAvailable()){
		return DLL_SUCCESS;
	}
	//初期処理に失敗したことがあったら
	else if(bInitImpFailure){
		return DLL_INITFAILURE; //DLLロードはできたけど、その初期処理に失敗
	}
	//それ以外
	else{
		return DLL_LOADFAILURE; //DLLロード自体に失敗
	}
}

bool CDllImp::DeinitDll(bool force) noexcept
{
	if( m_hInstance == nullptr || (!IsAvailable()) ){
		//	DLLが読み込まれていなければ何もしない
		return true;
	}

	//終了処理
	bool ret = DeinitDllImp();
	
	//DLL解放
	if( ret || force ){
		//DLL名を解放
		m_strLoadedDllName.clear();

		//DLL解放
		::FreeLibrary( m_hInstance );
		m_hInstance = nullptr;

		return true;
	}
	else{
		return false;
	}
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                           属性                              //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

LPCWSTR CDllImp::GetLoadedDllName() const
{
	return m_strLoadedDllName.c_str();
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                  オーバーロード可能実装                     //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

/*!
	実装を省略できるようにするため、空の関数を用意しておく
*/
bool CDllImp::DeinitDllImp()
{
	return true;
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                         実装補助                            //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

/*!
	テーブルで与えられたエントリポインタアドレスを入れる場所に
	対応する文字列から調べたエントリポインタを設定する。
	
	@param table [in] 名前とアドレスの対応表。最後は{NULL,0}で終わること。
	@retval true 全てのアドレスが設定された。
	@retval false アドレスの取得に失敗した関数があった。
*/
bool CDllImp::RegisterEntries(const ImportTable table[])
{
	if(!IsAvailable())return false;

	for(int i = 0; table[i].proc!=nullptr; i++)
	{
		FARPROC proc;
		if ((proc = ::GetProcAddress(GetInstance(), table[i].name)) == nullptr) 
		{
			return false;
		}
		*((FARPROC*)table[i].proc) = proc;
	}
	return true;
}
