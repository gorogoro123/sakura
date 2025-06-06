﻿/*! @file */
/*
	Copyright (C) 2008, kobake
	Copyright (C) 2018-2022, Sakura Editor Organization

	SPDX-License-Identifier: Zlib
*/

#include "StdAfx.h"
#include "types/CType.h"

const wchar_t* g_ppszKeywordsCORBA_IDL[] = {
	L"any",
	L"attribute",
	L"boolean",
	L"case",
	L"char",
	L"const",
	L"context",
	L"default",
	L"double",
	L"enum",
	L"exception",
	L"FALSE",
	L"fixed",
	L"float",
	L"in",
	L"inout",
	L"interface",
	L"long",
	L"module",
	L"Object",
	L"octet",
	L"oneway",
	L"out",
	L"raises",
	L"readonly",
	L"sequence",
	L"short",
	L"string",
	L"struct",
	L"switch",
	L"TRUE",
	L"typedef",
	L"unsigned",
	L"union",
	L"void",
	L"wchar_t",
	L"wstring"
};
int g_nKeywordsCORBA_IDL = _countof(g_ppszKeywordsCORBA_IDL);
