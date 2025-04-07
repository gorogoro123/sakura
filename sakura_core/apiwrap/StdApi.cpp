/*! @file */
/*
	Copyright (C) 2018-2022, Sakura Editor Organization

	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

		1. The origin of this software must not be misrepresented;
		   you must not claim that you wrote the original software.
		   If you use this software in a product, an acknowledgment
		   in the product documentation would be appreciated but is
		   not required.

		2. Altered source versions must be plainly marked as such,
		   and must not be misrepresented as being the original software.

		3. This notice may not be removed or altered from any source
		   distribution.
*/
#include "StdAfx.h"
#include <vector>
#include "StdApi.h"
#include "charset/charcode.h"

using namespace std;

namespace ApiWrap{

	// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
	//             その他W系API                                     //
	// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

	// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
	//                    描画API 不具合ラップ                     //
	// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
	/*
		VistaでSetPixelが動かないため、代替関数を用意。

		参考：http://forums.microsoft.com/MSDN-JA/ShowPost.aspx?PostID=3228018&SiteID=7
		> Vista で Aero を OFF にすると SetPixel がうまく動かないそうです。
		> しかも、SP1 でも修正されていないとか。

		一旦はvista以降向けの「不具合」対策をそのまま残します。
		vista前後でGDIの考え方が変わってるので、デバッグのやり方を考え直すべきと思います。
		by berryzplus 2018/10/13記す。
	*/
	void SetPixelSurely(HDC hdc,int x,int y,COLORREF c)
	{
		{
		//Vista以降：SetPixelエミュレート
			static HPEN hPen = NULL;
			static COLORREF clrPen = 0;
			if(hPen && c!=clrPen){
				DeleteObject(hPen);
				hPen = NULL;
			}
			//ペン生成
			if(!hPen){
				hPen = CreatePen(PS_SOLID,1,clrPen = c);
			}
			//描画
			HPEN hpnOld = (HPEN)SelectObject(hdc,hPen);
			::MoveToEx(hdc,x,y,NULL);
			::LineTo(hdc,x+1,y+1);
			SelectObject(hdc,hpnOld);
		}
	}
}
