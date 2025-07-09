﻿/*!	@file
	@brief WSH Handler

	@author 鬼
	@date 2002年4月28日
*/
/*
	Copyright (C) 2002, 鬼, genta
	Copyright (C) 2003, FILE
	Copyright (C) 2004, genta
	Copyright (C) 2005, FILE, zenryaku
	Copyright (C) 2009, syat
	Copyright (C) 2018-2022, Sakura Editor Organization

	SPDX-License-Identifier: Zlib
*/

#include "StdAfx.h"
#include <process.h> // _beginthreadex
#include <ObjBase.h>
#include <InitGuid.h>
#include <ShlDisp.h>
#include "macro/CWSH.h"
#include "macro/CIfObj.h"
#include "window/CEditWnd.h"
#include "util/os.h"
#include "util/module.h"
#include "util/window.h"	// BlockingHook
#include "dlg/CDlgCancel.h"
#include "CSelectLang.h"
#include "sakura_rc.h"
#include "config/app_constants.h"
#include "String_define.h"

#ifdef USE_JSCRIPT9
const GUID CLSID_JSScript9 =
{
	0x16d51579, 0xa30b, 0x4c8b, { 0xa2, 0x76, 0x0f, 0xf4, 0xdc, 0x41, 0xe7, 0x55 } 
};
#endif

/* 2009.10.29 syat インターフェースオブジェクト部分をCWSHIfObj.hに分離
class CInterfaceObjectTypeInfo: public ImplementsIUnknown<ITypeInfo>
 */

//IActiveScriptSite, IActiveScriptSiteWindow
/*!
	@date Sep. 15, 2005 FILE IActiveScriptSiteWindow実装．
		マクロでMsgBoxを使用可能にする．
*/
class CWSHSite: public IActiveScriptSite, public IActiveScriptSiteWindow
{
private:
	CWSHClient *m_Client;
	ULONG m_RefCount;
public:
	CWSHSite(CWSHClient *AClient): m_Client(AClient), m_RefCount(0)
	{
	}

	ULONG STDMETHODCALLTYPE AddRef() override {
		return ++m_RefCount;
	}

	ULONG STDMETHODCALLTYPE Release() override {
		if(--m_RefCount == 0)
		{
			delete this;
			return 0;
		}
		return m_RefCount;
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(
	    /* [in] */ REFIID iid,
	    /* [out] */ void ** ppvObject) override
	{
		*ppvObject = nullptr;

		if(iid == IID_IActiveScriptSiteWindow){
			*ppvObject = static_cast<IActiveScriptSiteWindow*>(this);
			++m_RefCount;
			return S_OK;
		}

		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE GetLCID(
	    /* [out] */ LCID *plcid) override
	{ 
#ifdef TEST
		cout << "GetLCID" << endl;
#endif
		return E_NOTIMPL; //システムデフォルトを使用
	}

	HRESULT STDMETHODCALLTYPE GetItemInfo(
	    /* [in] */ LPCOLESTR pstrName,
	    /* [in] */ DWORD dwReturnMask,
	    /* [out] */ IUnknown **ppiunkItem,
	    /* [out] */ ITypeInfo **ppti) override
	{
#ifdef TEST
		wcout << L"GetItemInfo:" << pstrName << endl;
#endif
		//指定された名前のインターフェースオブジェクトを検索
		const CWSHClient::List& objects = m_Client->GetInterfaceObjects();
		for( CWSHClient::ListIter it = objects.begin(); it != objects.end(); it++ )
		{
			//	Nov. 10, 2003 FILE Win9Xでは、[lstrcmpiW]が無効のため、[_wcsicmp]に修正
			if( _wcsicmp( pstrName, (*it)->m_sName.c_str() ) == 0 )
			{
				if(dwReturnMask & SCRIPTINFO_IUNKNOWN)
				{
					(*ppiunkItem) = *it;
					(*ppiunkItem)->AddRef();
				}
				if(dwReturnMask & SCRIPTINFO_ITYPEINFO)
				{
					(*it)->GetTypeInfo(0, 0, ppti);
				}
				return S_OK;
			}
		}
		return TYPE_E_ELEMENTNOTFOUND;
	}

	HRESULT STDMETHODCALLTYPE GetDocVersionString(
	    /* [out] */ BSTR *pbstrVersion) override
	{ 
#ifdef TEST
		cout << "GetDocVersionString" << endl;
#endif
		return E_NOTIMPL; 
	}

	HRESULT STDMETHODCALLTYPE OnScriptTerminate(
	    /* [in] */ const VARIANT *pvarResult,
	    /* [in] */ const EXCEPINFO *pexcepinfo) override
	{ 
#ifdef TEST
		cout << "OnScriptTerminate" << endl;
#endif
		return S_OK; 
	}

	HRESULT STDMETHODCALLTYPE OnStateChange(
	    /* [in] */ SCRIPTSTATE ssScriptState) override
	{ 
#ifdef TEST
		cout << "OnStateChange" << endl;
#endif
		return S_OK; 
	}

	//	Nov. 3, 2002 鬼
	//	エラー行番号表示対応
	HRESULT STDMETHODCALLTYPE OnScriptError(
	  /* [in] */ IActiveScriptError *pscripterror) override
	{ 
		EXCEPINFO Info;
		if(pscripterror->GetExceptionInfo(&Info) == S_OK)
		{
			DWORD Context;
			ULONG Line;
			LONG Pos;
			if(Info.bstrDescription == nullptr) {
				Info.bstrDescription = SysAllocString(LS(STR_ERR_CWSH09));
			}
			if(pscripterror->GetSourcePosition(&Context, &Line, &Pos) == S_OK)
			{
				wchar_t *Message = new wchar_t[SysStringLen(Info.bstrDescription) + 128];
				//	Nov. 10, 2003 FILE Win9Xでは、[wsprintfW]が無効のため、[auto_sprintf]に修正
				const wchar_t* szDesc=Info.bstrDescription;
				auto_sprintf(Message, L"[Line %d] %ls", Line + 1, szDesc);
				SysReAllocString(&Info.bstrDescription, Message);
				delete[] Message;
			}
			m_Client->Error(Info.bstrDescription, Info.bstrSource);
			SysFreeString(Info.bstrSource);
			SysFreeString(Info.bstrDescription);
			SysFreeString(Info.bstrHelpFile);
		}
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnEnterScript() override {
#ifdef TEST
		cout << "OnEnterScript" << endl;
#endif
		return S_OK; 
	}

	HRESULT STDMETHODCALLTYPE OnLeaveScript() override {
#ifdef TEST
		cout << "OnLeaveScript" << endl;
#endif
		return S_OK; 
	}

	//	Sep. 15, 2005 FILE IActiveScriptSiteWindow実装
	HRESULT STDMETHODCALLTYPE GetWindow(
	    /* [out] */ HWND *phwnd) override
	{
		*phwnd = CEditWnd::getInstance()->m_cSplitterWnd.GetHwnd();
		return S_OK;
	}

	//	Sep. 15, 2005 FILE IActiveScriptSiteWindow実装
	HRESULT STDMETHODCALLTYPE EnableModeless(
	    /* [in] */ BOOL fEnable) override
	{
		return S_OK;
	}
};

//implementation

CWSHClient::CWSHClient(const wchar_t *AEngine, ScriptErrorHandler AErrorHandler, void *AData): 
				m_OnError(AErrorHandler), m_Data(AData), m_Valid(false), m_Engine(nullptr)
{ 
	// 2010.08.28 DLL インジェクション対策としてEXEのフォルダーに移動する
	CCurrentDirectoryBackupPoint dirBack;
	ChangeCurrentDirectoryToExeDir();
	
	CLSID ClassID;
	if(CLSIDFromProgID(AEngine, &ClassID) != S_OK)
		Error(LS(STR_ERR_CWSH01));
	else
	{
#ifdef USE_JSCRIPT9
		if( 0 == wcscmp( AEngine, LTEXT("JScript") ) ){
			ClassID = CLSID_JSScript9;
		}
#endif
		if(CoCreateInstance(ClassID, nullptr, CLSCTX_INPROC_SERVER, IID_IActiveScript, reinterpret_cast<void **>(&m_Engine)) != S_OK)
			Error(LS(STR_ERR_CWSH02));
		else
		{
			IActiveScriptSite *Site = new CWSHSite(this);
			if(m_Engine->SetScriptSite(Site) != S_OK)
			{
				delete Site;
				Error(LS(STR_ERR_CWSH03));
			}
			else
			{
				m_Valid = true;
			}
		}
	}
}

CWSHClient::~CWSHClient()
{
	//インターフェースオブジェクトを解放
	for( ListIter it = m_IfObjArr.begin(); it != m_IfObjArr.end(); it++ ){
		(*it)->Release();
	}
	
	if(m_Engine != nullptr) 
		m_Engine->Release();
}

// AbortMacroProcのパラメータ構造体
typedef struct {
	HANDLE hEvent;
	IActiveScript *pEngine;				//ActiveScript
	int nCancelTimer;
	CEditView *view;
} SAbortMacroParam;

// WSHマクロ実行を中止するスレッド
static unsigned __stdcall AbortMacroProc( LPVOID lpParameter )
{
	SAbortMacroParam* pParam = (SAbortMacroParam*) lpParameter;

	//停止ダイアログ表示前に数秒待つ
	if(::WaitForSingleObject(pParam->hEvent, pParam->nCancelTimer * 1000) == WAIT_TIMEOUT){
		//停止ダイアログ表示
		DEBUG_TRACE(L"AbortMacro: Show Dialog\n");

		MSG msg;
		CDlgCancel cDlgCancel;
		HWND hwndDlg = cDlgCancel.DoModeless(G_AppInstance(), nullptr, IDD_MACRORUNNING);	// エディタビジーでも表示できるよう、親を指定しない
		// ダイアログタイトルとファイル名を設定
		::SendMessage(hwndDlg, WM_SETTEXT, 0, (LPARAM)GSTR_APPNAME);
		::SendMessage(GetDlgItem(hwndDlg, IDC_STATIC_CMD),
			WM_SETTEXT, 0, (LPARAM)pParam->view->GetDocument()->m_cDocFile.GetFilePath());
		
		bool bCanceled = false;
		for(;;){
			DWORD dwResult = MsgWaitForMultipleObjects( 1, &pParam->hEvent, FALSE, INFINITE, QS_ALLINPUT );
			if(dwResult == WAIT_OBJECT_0){
				::SendMessage( cDlgCancel.GetHwnd(), WM_CLOSE, 0, 0 );
			}else if(dwResult == WAIT_OBJECT_0+1){
				while(::PeekMessage(&msg , nullptr , 0 , 0, PM_REMOVE )){
					if(cDlgCancel.GetHwnd() != nullptr && ::IsDialogMessage(cDlgCancel.GetHwnd(), &msg)){
					}else{
						::TranslateMessage(&msg);
						::DispatchMessage(&msg);
					}
				}
			}else{
				//MsgWaitForMultipleObjectsに与えたハンドルのエラー
				break;
			}
			if(!bCanceled && cDlgCancel.IsCanceled()){
				DEBUG_TRACE(L"Canceld\n");
				bCanceled = true;
				cDlgCancel.CloseDialog( 0 );
			}
			if(cDlgCancel.GetHwnd() == nullptr){
				DEBUG_TRACE(L"Close\n");
				break;
			}
		}

		DEBUG_TRACE(L"AbortMacro: Try Interrupt\n");
		pParam->pEngine->InterruptScriptThread(SCRIPTTHREADID_BASE, nullptr, 0);
		DEBUG_TRACE(L"AbortMacro: Done\n");
	}

	DEBUG_TRACE(L"AbortMacro: Exit\n");
	return 0;
}

bool CWSHClient::Execute(const wchar_t *AScript)
{
	bool bRet = false;
	IActiveScriptParse *Parser;
	if(m_Engine->QueryInterface(IID_IActiveScriptParse, reinterpret_cast<void **>(&Parser)) != S_OK)
		Error(LS(STR_ERR_CWSH04));
	else 
	{
		if(Parser->InitNew() != S_OK)
			Error(LS(STR_ERR_CWSH05));
		else
		{
			bool bAddNamedItemError = false;

			for( ListIter it = m_IfObjArr.cbegin(); it != m_IfObjArr.cend(); it++ )
			{
				DWORD dwFlag = SCRIPTITEM_ISVISIBLE;

				if( (*it)->IsGlobal() ){ dwFlag |= SCRIPTITEM_GLOBALMEMBERS; }

				if(m_Engine->AddNamedItem( (*it)->Name(), dwFlag ) != S_OK)
				{
					bAddNamedItemError = true;
					Error(LS(STR_ERR_CWSH06));
					break;
				}
			}
			if( !bAddNamedItemError )
			{
				//マクロ停止スレッドの起動
				SAbortMacroParam sThreadParam;
				sThreadParam.pEngine = m_Engine;
				sThreadParam.nCancelTimer = GetDllShareData().m_Common.m_sMacro.m_nMacroCancelTimer;
				sThreadParam.view = (CEditView*)m_Data;

				HANDLE hThread = nullptr;
				unsigned int nThreadId = 0;
				if( 0 < sThreadParam.nCancelTimer ){
					sThreadParam.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
					hThread = (HANDLE)_beginthreadex( nullptr, 0, AbortMacroProc, (LPVOID)&sThreadParam, 0, &nThreadId );
					DEBUG_TRACE(L"Start AbortMacroProc 0x%08x\n", nThreadId);
				}

				//マクロ実行
				if(m_Engine->SetScriptState(SCRIPTSTATE_STARTED) != S_OK)
					Error(LS(STR_ERR_CWSH07));
				else
				{
					HRESULT hr = Parser->ParseScriptText(AScript, nullptr, nullptr, nullptr, 0, 0, SCRIPTTEXT_ISVISIBLE, nullptr, nullptr);
					if (hr == SCRIPT_E_REPORTED) {
					/*
						IActiveScriptSite->OnScriptErrorに通知済み。
						中断メッセージが既に表示されてるはず。
					*/
					} else if(hr != S_OK) {
						Error(LS(STR_ERR_CWSH08));
					} else {
						bRet = true;
					}
				}

				if( 0 < sThreadParam.nCancelTimer ){
					::SetEvent(sThreadParam.hEvent);

					//マクロ停止スレッドの終了待ち
					DEBUG_TRACE(L"Waiting for AbortMacroProc to finish\n");
					::WaitForSingleObject(hThread, INFINITE); 
					::CloseHandle(hThread);
					::CloseHandle(sThreadParam.hEvent);
				}
			}
		}
		Parser->Release();
	}
	m_Engine->Close();
	return bRet;
}

void CWSHClient::Error(BSTR Description, BSTR Source)
{
	if(m_OnError != nullptr)
		m_OnError(Description, Source, m_Data);
}

void CWSHClient::Error(const wchar_t* Description)
{
	BSTR S = SysAllocString(L"WSH");
	BSTR D = SysAllocString(Description);
	Error(D, S);
	SysFreeString(S);
	SysFreeString(D);
}

//インターフェースオブジェクトの追加
void CWSHClient::AddInterfaceObject( CIfObj* obj )
{
	if( !obj ) return;
	m_IfObjArr.push_back( obj );
	obj->m_Owner = this;
	obj->AddRef();
}

/////////////////////////////////////////////
/*!
	MacroCommand→CWSHIfObj.cppへ移動
	CWSHMacroManager →　CWSHManager.cppへ移動

*/
