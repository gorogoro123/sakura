﻿/*! @file */
/*
	Copyright (C) 2008, kobake
	Copyright (C) 2018-2022, Sakura Editor Organization

	SPDX-License-Identifier: Zlib
*/

#include "StdAfx.h"
#include "types/CType.h"
#include "doc/CEditDoc.h"
#include "doc/CDocOutline.h"
#include "doc/logic/CDocLine.h"
#include "outline/CFuncInfoArr.h"
#include "view/colors/EColorIndexType.h"

/* アセンブラ */
//	2004.05.01 MIK/genta
//Mar. 10, 2001 JEPRO	半角数値を色分け表示
void CType_Asm::InitTypeConfigImp(STypeConfig* pType)
{
	//名前と拡張子
	wcscpy( pType->m_szTypeName, L"アセンブラ" );
	wcscpy( pType->m_szTypeExts, L"asm" );

	//設定
	pType->m_cLineComment.CopyTo( 0, L";", -1 );			/* 行コメントデリミタ */
	pType->m_eDefaultOutline = OUTLINE_ASM;					/* アウトライン解析方法 */
	pType->m_ColorInfoArr[COLORIDX_DIGIT].m_bDisp = true;
}

/*! アセンブラ アウトライン解析

	@author MIK
	@date 2004.04.12 作り直し
*/
void CDocOutline::MakeTopicList_asm( CFuncInfoArr* pcFuncInfoArr )
{
	CLogicInt nTotalLine;

	nTotalLine = m_pcDocRef->m_cDocLineMgr.GetLineCount();

	for( CLogicInt nLineCount = CLogicInt(0); nLineCount < nTotalLine; nLineCount++ ){
		const WCHAR* pLine;
		CLogicInt nLineLen;
		WCHAR* pTmpLine;
		int length;
		int offset;
#define MAX_ASM_TOKEN 2
		WCHAR* token[MAX_ASM_TOKEN];
		int j;
		WCHAR* p;

		//1行取得する。
		pLine = m_pcDocRef->m_cDocLineMgr.GetLine(nLineCount)->GetDocLineStrWithEOL(&nLineLen);
		if( pLine == nullptr ) break;

		//作業用にコピーを作成する。バイナリがあったらその後ろは知らない。
		pTmpLine = _wcsdup( pLine );
		if( pTmpLine == nullptr ) break;
		if( wcslen( pTmpLine ) >= (unsigned int)nLineLen ){	//バイナリを含んでいたら短くなるので...
			pTmpLine[ nLineLen ] = L'\0';	//指定長で切り詰め
		}

		//行コメント削除
		p = wcschr( pTmpLine, L';' );
		if( p ) *p = L'\0';

		length = wcslen( pTmpLine );
		offset = 0;

		//トークンに分割
		for( j = 0; j < MAX_ASM_TOKEN; j++ ) token[ j ] = nullptr;
		for( j = 0; j < MAX_ASM_TOKEN; j++ ){
			token[ j ] = my_strtok<WCHAR>( pTmpLine, length, &offset, L" \t\r\n" );
			if( token[ j ] == nullptr ) break;
			//トークンに含まれるべき文字でないか？
			if( wcschr( token[ j ], L'\"') != nullptr
			 || wcschr( token[ j ], L'\\') != nullptr
			 || wcschr( token[ j ], L'\'' ) != nullptr ){
				token[ j ] = nullptr;
				break;
			}
		}

		if( token[ 0 ] != nullptr ){	//トークンが1個以上ある
			int nFuncId = -1;
			WCHAR* entry_token = nullptr;

			length = wcslen( token[ 0 ] );
			if( length >= 2
			 && token[ 0 ][ length - 1 ] == L':' ){	//ラベル
				token[ 0 ][ length - 1 ] = L'\0';
				nFuncId = 51;
				entry_token = token[ 0 ];
			}
			else if( token[ 1 ] != nullptr ){	//トークンが2個以上ある
				if( _wcsicmp( token[ 1 ], L"proc" ) == 0 ){	//関数
					nFuncId = 50;
					entry_token = token[ 0 ];
				}else
				if( _wcsicmp( token[ 1 ], L"endp" ) == 0 ){	//関数終了
					nFuncId = 52;
					entry_token = token[ 0 ];
				//}else
				//if( my_stricmp( token[ 1 ], L"macro" ) == 0 ){	//マクロ
				//	nFuncId = -1;
				//	entry_token = token[ 0 ];
				//}else
				//if( my_stricmp( token[ 1 ], L"struc" ) == 0 ){	//構造体
				//	nFuncId = -1;
				//	entry_token = token[ 0 ];
				}
			}

			if( nFuncId >= 0 ){
				/*
				  カーソル位置変換
				  物理位置(行頭からのバイト数、折り返し無し行位置)
				  →
				  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
				*/
				CLayoutPoint ptPos;
				m_pcDocRef->m_cLayoutMgr.LogicToLayout(
					CLogicPoint(0, nLineCount),
					&ptPos
				);
				pcFuncInfoArr->AppendData( nLineCount + CLogicInt(1), ptPos.GetY2() + CLayoutInt(1), entry_token, nFuncId );
			}
		}

		free( pTmpLine );
	}

	return;
}
