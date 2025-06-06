﻿/*! @file */
/*
	Copyright (C) 2018-2022, Sakura Editor Organization

	SPDX-License-Identifier: Zlib
*/
#include "StdAfx.h"
#include "input.h"
#include "apiwrap/StdApi.h"
#include "config/system_constants.h"

// novice 2004/10/10 マウスサイドボタン対応
/*!
	Shift,Ctrl,Altキー状態の取得

	@retval nIdx Shift,Ctrl,Altキー状態
	@date 2004.10.10 関数化
*/
int getCtrlKeyState()
{
	int nIdx = 0;

	/* Shiftキーが押されているなら */
	if(GetKeyState_Shift()){
		nIdx |= _SHIFT;
	}
	/* Ctrlキーが押されているなら */
	if( GetKeyState_Control() ){
		nIdx |= _CTRL;
	}
	/* Altキーが押されているなら */
	if( GetKeyState_Alt() ){
		nIdx |= _ALT;
	}

	return nIdx;
}
