﻿/*!	@file
	@brief 印刷関連

	@author Norio Nakatani
	@date 1998/06/09 新規作成
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2003, かろと
	Copyright (C) 2018-2022, Sakura Editor Organization

	SPDX-License-Identifier: Zlib
*/

#ifndef SAKURA_CPRINT_CB147282_3673_4A39_9B0A_C5C323C39C56_H_
#define SAKURA_CPRINT_CB147282_3673_4A39_9B0A_C5C323C39C56_H_
#pragma once

#include <WinSpool.h>
#include <CommDlg.h> // PRINTDLG
#include "basis/primitive.h"

struct	MYDEVMODE {
	BOOL	m_bPrinterNotFound;	/* プリンターがなかったフラグ */
	WCHAR	m_szPrinterDriverName[_MAX_PATH + 1];	// プリンタードライバー名
	WCHAR	m_szPrinterDeviceName[_MAX_PATH + 1];	// プリンターデバイス名
	WCHAR	m_szPrinterOutputName[_MAX_PATH + 1];	// プリンターポート名
	DWORD	dmFields;
	short	dmOrientation;
	short	dmPaperSize;
	short	dmPaperLength;
	short	dmPaperWidth;

	//! 等価比較演算子
	bool operator == (const MYDEVMODE& rhs) const noexcept {
		if (this == &rhs) return true;
		return m_bPrinterNotFound == rhs.m_bPrinterNotFound
			&& 0 == wcsncmp(m_szPrinterDriverName, rhs.m_szPrinterDriverName, _countof(m_szPrinterDriverName))
			&& 0 == wcsncmp(m_szPrinterDeviceName, rhs.m_szPrinterDeviceName, _countof(m_szPrinterDeviceName))
			&& 0 == wcsncmp(m_szPrinterOutputName, rhs.m_szPrinterOutputName, _countof(m_szPrinterOutputName))
			&& dmFields == rhs.dmFields
			&& dmOrientation == rhs.dmOrientation
			&& dmPaperSize == rhs.dmPaperSize
			&& dmPaperLength == rhs.dmPaperLength
			&& dmPaperWidth == rhs.dmPaperWidth
			;
	}
	//! 否定の等価比較演算子
	bool operator != (const MYDEVMODE& rhs) const noexcept {
		return !(*this == rhs);
	}
};

// 2006.08.14 Moca 用紙情報の統合 PAPER_INFO新設
//! 用紙情報
struct PAPER_INFO {
	int				m_nId;			//!< 用紙ID
	short			m_nAllWidth;	//!< 幅 (0.1mm単位)
	short			m_nAllHeight;	//!< 高さ (0.1mm単位)
	const WCHAR*	m_pszName;		//!< 用紙名称
};

struct PRINTSETTING;

//! 印刷設定
#define POS_LEFT	0
#define POS_CENTER	1
#define POS_RIGHT	2
#define HEADER_MAX	100
#define FOOTER_MAX	HEADER_MAX
struct PRINTSETTING {
	WCHAR			m_szPrintSettingName[32 + 1];		/*!< 印刷設定の名前 */
	WCHAR			m_szPrintFontFaceHan[LF_FACESIZE];	/*!< 印刷フォント */
	WCHAR			m_szPrintFontFaceZen[LF_FACESIZE];	/*!< 印刷フォント */
	int				m_nPrintFontWidth;					/*!< 印刷フォント幅(1/10mm単位単位) */
	int				m_nPrintFontHeight;					/*!< 印刷フォント高さ(1/10mm単位単位) */
	int				m_nPrintDansuu;						/*!< 段組の段数 */
	int				m_nPrintDanSpace;					/*!< 段と段の隙間(1/10mm単位) */
	int				m_nPrintLineSpacing;				/*!< 印刷フォント行間 文字の高さに対する割合(%) */
	int				m_nPrintMarginTY;					/*!< 印刷用紙マージン 上(mm単位) */
	int				m_nPrintMarginBY;					/*!< 印刷用紙マージン 下(mm単位) */
	int				m_nPrintMarginLX;					/*!< 印刷用紙マージン 左(mm単位) */
	int				m_nPrintMarginRX;					/*!< 印刷用紙マージン 右(mm単位) */
	short			m_nPrintPaperOrientation;			/*!< 用紙方向 DMORIENT_PORTRAIT (1) または DMORIENT_LANDSCAPE (2) */
	short			m_nPrintPaperSize;					/*!< 用紙サイズ */
	bool			m_bColorPrint;						//!< カラー印刷			// 2013/4/26 Uchi
	bool			m_bPrintWordWrap;					//!< 英文ワードラップする
	bool			m_bPrintKinsokuHead;				//!< 行頭禁則する		//@@@ 2002.04.09 MIK
	bool			m_bPrintKinsokuTail;				//!< 行末禁則する		//@@@ 2002.04.09 MIK
	bool			m_bPrintKinsokuRet;					//!< 改行文字のぶら下げ	//@@@ 2002.04.13 MIK
	bool			m_bPrintKinsokuKuto;				//!< 句読点のぶらさげ	//@@@ 2002.04.17 MIK
	bool			m_bPrintLineNumber;					/*!< 行番号を印刷する */

	MYDEVMODE		m_mdmDevMode;						/*!< プリンター設定 DEVMODE用 */
	BOOL			m_bHeaderUse[3];					/* ヘッダーが使われているか？	*/
	EDIT_CHAR		m_szHeaderForm[3][HEADER_MAX];		/* 0:左寄せヘッダー。1:中央寄せヘッダー。2:右寄せヘッダー。*/
	BOOL			m_bFooterUse[3];					/* フッターが使われているか？	*/
	EDIT_CHAR		m_szFooterForm[3][FOOTER_MAX];		/* 0:左寄せフッター。1:中央寄せフッター。2:右寄せフッター。*/

	// ヘッダー/フッターのフォント(lfFaceNameが設定されていなければ半角/全角フォントを使用)
	LOGFONT			m_lfHeader;							// ヘッダーフォント用LOGFONT構造体
	int 			m_nHeaderPointSize;					// ヘッダーフォントポイントサイズ
	LOGFONT			m_lfFooter;							// フッターフォント用LOGFONT構造体
	int 			m_nFooterPointSize;					// フッターフォントポイントサイズ
};

/*-----------------------------------------------------------------------
クラスの宣言
-----------------------------------------------------------------------*/
/*!
	@brief 印刷関連機能

	オブジェクト指向でないクラス
*/
class CPrint
{
	using Me = CPrint;

public:
	static const PAPER_INFO m_paperInfoArr[];	//!< 用紙情報一覧
	static const int m_nPaperInfoArrNum; //!< 用紙情報一覧の要素数

	/*
	||	static関数群
	*/
	static void SettingInitialize( PRINTSETTING&, const WCHAR* settingName );

	static WCHAR* GetPaperName( int , WCHAR* );	/* 用紙の名前を取得 */
	/* 用紙の幅、高さ */
	static BOOL GetPaperSize(
		short*		pnPaperAllWidth,
		short*		pnPaperAllHeight,
		MYDEVMODE*	pDEVMODE
	);
	/* 印字可能桁・行の計算 */
	static int CalculatePrintableColumns( PRINTSETTING*, int width, int nLineNumberColumns );
	static int CalculatePrintableLines( PRINTSETTING*, int height );

	/* ヘッダー・フッターの高さ計算 */
	static int CalcHeaderHeight( PRINTSETTING* );
	static int CalcFooterHeight( PRINTSETTING* );
public:
	/*
	||  Constructors
	*/
	CPrint();
	CPrint(const Me&) = delete;
	Me& operator = (const Me&) = delete;
	CPrint(Me&&) noexcept = delete;
	Me& operator = (Me&&) noexcept = delete;
	~CPrint();

	/*
	||  Attributes & Operations
	*/
	BOOL GetDefaultPrinter( MYDEVMODE *pMYDEVMODE );		/* デフォルトのプリンター情報を取得 */
	BOOL PrintDlg( PRINTDLG *pd, MYDEVMODE *pMYDEVMODE );				/* プリンター情報を取得 */
	/* 印刷/プレビューに必要な情報を取得 */
	BOOL GetPrintMetrics(
		MYDEVMODE*	pMYDEVMODE,
		short*		pnPaperAllWidth,	/* 用紙幅 */
		short*		pnPaperAllHeight,	/* 用紙高さ */
		short*		pnPaperWidth,		/* 用紙印刷可能幅 */
		short*		pnPaperHeight,		/* 用紙印刷可能高さ */
		short*		pnPaperOffsetLeft,	/* 用紙余白左端 */
		short*		pnPaperOffsetTop,	/* 用紙余白上端 */
		WCHAR*		pszErrMsg			/* エラーメッセージ格納場所 */
	);

	/* 印刷 ジョブ開始 */
	BOOL PrintOpen(
		WCHAR*		pszJobName,
		MYDEVMODE*	pMYDEVMODE,
		HDC*		phdc,
		WCHAR*		pszErrMsg		/* エラーメッセージ格納場所 */
	);
	void PrintStartPage(HDC hdc);	/* 印刷 ページ開始 */
	void PrintEndPage(HDC hdc);	/* 印刷 ページ終了 */
	void PrintClose(HDC hdc);		/* 印刷 ジョブ終了 */ // 2003.05.02 かろと 不要なhPrinter削除

protected:
	/*
	||  実装ヘルパ関数
	*/
	// DC作成する(処理をまとめた) 2003.05.02 かろと
	HDC CreateDC( MYDEVMODE *pMYDEVMODE, WCHAR *pszErrMsg);
	
	static const PAPER_INFO* FindPaperInfo( int id );
private:
	/*
	||  メンバ変数
	*/
	HGLOBAL	m_hDevMode;							//!< 現在プリンターのDEVMODEへのメモリハンドル
	HGLOBAL	m_hDevNames;						//!< 現在プリンターのDEVNAMESへのメモリハンドル
};
#endif /* SAKURA_CPRINT_CB147282_3673_4A39_9B0A_C5C323C39C56_H_ */
