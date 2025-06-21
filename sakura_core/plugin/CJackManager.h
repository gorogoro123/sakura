﻿/*!	@file
	@brief ジャック管理クラス

*/
/*
	Copyright (C) 2009, syat
	Copyright (C) 2018-2022, Sakura Editor Organization

	SPDX-License-Identifier: Zlib
*/
#ifndef SAKURA_CJACKMANAGER_99C6FE17_62C7_45E8_82F2_C36441FF809C_H_
#define SAKURA_CJACKMANAGER_99C6FE17_62C7_45E8_82F2_C36441FF809C_H_
#pragma once

#include "plugin/CPlugin.h"
#include <list>
#include "util/design_template.h"

#define PP_COMMAND_STR	L"Command"

// ジャック（＝プラグイン可能箇所）
enum EJack {
	PP_NONE			= -1,
	PP_COMMAND		= 0,
//	PP_INSTALL,
//	PP_UNINSTALL,
//	PP_APP_START,	// 現状エディタごとにプラグイン管理しているため
//	PP_APP_END,		// アプリレベルのイベントは扱いにくい
	PP_EDITOR_START,
	PP_EDITOR_END,
	PP_DOCUMENT_OPEN,
	PP_DOCUMENT_CLOSE,
	PP_DOCUMENT_BEFORE_SAVE,
	PP_DOCUMENT_AFTER_SAVE,
	PP_OUTLINE,
	PP_SMARTINDENT,
	PP_COMPLEMENT,
	PP_COMPLEMENTGLOBAL,

	//↑ジャックを追加するときはこの行の上に。
	PP_BUILTIN_JACK_COUNT	//組み込みジャック数
};

// ジャック定義構造体
typedef struct tagJackDef {
	EJack			ppId;
	const WCHAR*	szName;
	CPlug::Array		plugs;	//ジャックに関連付けられたプラグ
} JackDef;

// プラグ登録結果
enum ERegisterPlugResult {
	PPMGR_REG_OK,				//プラグイン登録成功
	PPMGR_INVALID_NAME,			//ジャック名が不正
	PPMGR_CONFLICT				//指定したジャックは別のプラグインが接続している
};

//ジャック管理クラス
class CJackManager final : public TSingleton<CJackManager>{
	friend class TSingleton<CJackManager>;
	CJackManager();

	//操作
public:
	ERegisterPlugResult RegisterPlug( std::wstring pszJack, CPlug* plug );	//プラグをジャックに関連付ける
	bool UnRegisterPlug( std::wstring pszJack, CPlug* plug );	//プラグの関連付けを解除する
	bool GetUsablePlug( EJack jack, PlugId plugId, CPlug::Array* plugs );	//利用可能なプラグを検索する
	void InvokePlugins( EJack jack, CEditView* view );		//プラグインを列挙して呼び出し
private:
	EJack GetJackFromName( std::wstring sName );	//ジャック名をジャック番号に変換する

	//属性
public:
	std::vector<JackDef> GetJackDef() const;	//ジャック定義一覧を返す
	EFunctionCode GetCommandCode( int index ) const;		//プラグインコマンドの機能コードを返す
	int GetCommandName( int funccode, WCHAR* buf, int size ) const;	//プラグインコマンドの名前を返す
	int GetCommandCount() const;	//プラグインコマンドの数を返す
	CPlug* GetCommandById( int id ) const;	//IDに合致するコマンドプラグを返す
	const CPlug::Array& GetPlugs( EJack jack ) const;	//プラグを返す
	//TODO: 作りが一貫してないので整理する syat

	//メンバ変数
private:
	DLLSHAREDATA* m_pShareData;
	std::vector<JackDef> m_Jacks;	//ジャック定義の一覧
};
#endif /* SAKURA_CJACKMANAGER_99C6FE17_62C7_45E8_82F2_C36441FF809C_H_ */
