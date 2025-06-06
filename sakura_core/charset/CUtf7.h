﻿/*! @file */
/*
	Copyright (C) 2008, kobake
	Copyright (C) 2018-2022, Sakura Editor Organization

	SPDX-License-Identifier: Zlib
*/
#ifndef SAKURA_CUTF7_55498766_1C8A_416B_9F39_88D3D83B8B65_H_
#define SAKURA_CUTF7_55498766_1C8A_416B_9F39_88D3D83B8B65_H_
#pragma once

#include "CCodeBase.h"

class CUtf7 : public CCodeBase{
public:
	//CCodeBaseインターフェース
	EConvertResult CodeToUnicode(const CMemory& cSrc, CNativeW* pDst) override{ return UTF7ToUnicode(cSrc, pDst); }	//!< 特定コード → UNICODE    変換
	EConvertResult UnicodeToCode(const CNativeW& cSrc, CMemory* pDst) override{ return UnicodeToUTF7(cSrc, pDst); }	//!< UNICODE    → 特定コード 変換

public:
	//実装
	static EConvertResult UTF7ToUnicode(const CMemory& cSrc, CNativeW* pDstMem);		// UTF-7     → Unicodeコード変換 //2007.08.13 kobake 追加
	static EConvertResult UnicodeToUTF7(const CNativeW& cSrc, CMemory* pDstMem);		// Unicode   → UTF-7コード変換
//	static int MemBASE64_Encode( const char*, int, char**, int, int );/* Base64エンコード */  // convert/convert_util2.h へ移動

protected:

	// 2008.11.10 変換ロジックを書き直す
	static int _Utf7SetDToUni_block( const char* pSrc, const int nSrcLen, wchar_t* pDst );
	static int _Utf7SetBToUni_block( const char* pSrc, const int nSrcLen, wchar_t* pDst, bool* pbError );
	static int Utf7ToUni( const char* pSrc, const int nSrcLen, wchar_t* pDst, bool* pbError );

	static int _UniToUtf7SetD_block( const wchar_t* pSrc, const int nSrcLen, char* pDst );
	static int _UniToUtf7SetB_block( const wchar_t* pSrc, const int nSrcLen, char* pDst );
	static int UniToUtf7( const wchar_t* pSrc, const int nSrcLen, char* pDst, int nDstLen );
};
#endif /* SAKURA_CUTF7_55498766_1C8A_416B_9F39_88D3D83B8B65_H_ */
