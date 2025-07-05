﻿/*! @file */
/*
	Copyright (C) 2008, kobake
	Copyright (C) 2018-2022, Sakura Editor Organization

	SPDX-License-Identifier: Zlib
*/
#ifndef SAKURA_REGKEY_6B5694D7_BDD3_4835_8B34_356D3FC110C7_H_
#define SAKURA_REGKEY_6B5694D7_BDD3_4835_8B34_356D3FC110C7_H_
#pragma once

class CRegKey
{
	using Me = CRegKey;

protected:
	HKEY _root;
	HKEY _key;
public:
	CRegKey()
	{
		_root = nullptr;
		_key = nullptr;
	}

	CRegKey(const Me&) = delete;
	Me& operator = (const Me&) = delete;
	CRegKey(Me&&) noexcept = delete;
	Me& operator = (Me&&) noexcept = delete;

	virtual ~CRegKey()
	{
		Close();
	}

	static bool ExistsKey(HKEY root, const WCHAR* path, unsigned int access = KEY_READ)
	{
		CRegKey test;
		return (test.Open(root, path, access) == 0);
	}
	bool IsOpend() const
	{
		return (_key != nullptr);
	}
	int Create(HKEY root, const WCHAR* path, unsigned int access = (KEY_READ | KEY_WRITE))
	{
		LONG error = RegCreateKeyEx(root, path, 0, nullptr, 0, access, nullptr, &_key, nullptr);
		if(error != ERROR_SUCCESS)
		{
			return error;
		}
		_root = root;
		return ERROR_SUCCESS;
	}
	int Open(HKEY root, const WCHAR* path, unsigned int access = KEY_READ)
	{
		LONG error = RegOpenKeyEx(root, path, 0, access, &_key);
		if(error != ERROR_SUCCESS)
		{
			return error;
		}
		_root = root;
		return ERROR_SUCCESS;
	}
	void Close()
	{
		if(_key != nullptr)
		{
			RegCloseKey(_key);
			_key = nullptr;
			_root = nullptr;
		}
	}
	int GetValue(const WCHAR* valueName, WCHAR* buffer, unsigned int nMaxChar, int* pGetChars = nullptr) const
	{
		DWORD dwType = REG_SZ;
		DWORD nError = 0;
		DWORD getChars = nMaxChar;
		if ((nError = RegQueryValueEx(_key, valueName, nullptr, &dwType, (LPBYTE)buffer, &getChars)) != 0)
		{
			return nError;
		}
		if(pGetChars)
		{
			*pGetChars = getChars;
		}
		return ERROR_SUCCESS;
	}
	int GetValueBINARY(const WCHAR* valueName, BYTE* buffer, unsigned int nMaxChar, int* pGetChars = nullptr) const
	{
		DWORD dwType = REG_BINARY;
		DWORD nError = 0;
		DWORD getChars = nMaxChar;
		if ((nError = RegQueryValueEx(_key, valueName, nullptr, &dwType, (LPBYTE)buffer, &getChars)) != 0)
		{
			return nError;
		}
		if(pGetChars)
		{
			*pGetChars = getChars;
		}
		return ERROR_SUCCESS;
	}
	int SetValue(const WCHAR* valueName, const WCHAR* buffer, int nMaxChar = -1)
	{
		if(nMaxChar == -1)
		{
			nMaxChar = (DWORD)wcslen(buffer) * sizeof(WCHAR);
		}
		DWORD nError = 0;
		if ((nError = RegSetValueEx(_key, valueName, 0, REG_SZ, (LPBYTE)buffer, nMaxChar)) != 0)
		{
			return nError;
		}
		return ERROR_SUCCESS;
	}
	int SetValue(const WCHAR* valueName, const BYTE* buffer, int nMaxChar, DWORD dwType)
	{
		DWORD nError = 0;
		if ((nError = RegSetValueEx(_key, valueName, 0, dwType, (LPBYTE)buffer, nMaxChar)) != 0)
		{
			return nError;
		}
		return ERROR_SUCCESS;
	}

	int DeleteValue(const WCHAR* valueName)
	{
		return RegDeleteValue(_key, valueName);
	}
	int DeleteSubKey(const WCHAR* path)
	{
		return RegDeleteKey(_key, path);
	}

	int EnumKey(int &index, WCHAR* pNameBuffer, int nMaxChar, int* pGetChar = nullptr) const
	{
		if(index < 0)
		{
			return ERROR_NO_MORE_ITEMS;
		}
		DWORD nSize = nMaxChar;
		int nError = RegEnumKeyEx(_key, (DWORD)index, pNameBuffer, &nSize, nullptr, nullptr, nullptr, nullptr);
		if(nError != ERROR_SUCCESS)
		{
			index = -1;
			return nError;
		}
		if(pGetChar)
		{
			*pGetChar = nMaxChar;
		}
		return nError;
	}

	int EnumValue(int &index, WCHAR* pNameBuffer, int nMaxChar, DWORD *lpType, BYTE *lpData, int nMaxData, DWORD* pDataLen) const
	{
		if(index < 0)
		{
			return ERROR_NO_MORE_ITEMS;
		}
		DWORD nValueSize = nMaxChar;
		DWORD nDataSize = nMaxChar;
		int nError = RegEnumValue(_key, (DWORD)index, pNameBuffer, &nValueSize, nullptr, lpType, lpData, &nDataSize);
		if(nError != ERROR_SUCCESS)
		{
			index = -1;
			return nError;
		}
		if( pDataLen )
		{
			*pDataLen = nDataSize;
		}
		return nError;
	}

	static int DeleteKey(HKEY root, const WCHAR* path)
	{
		return RegDeleteKey(root, path);
	}
};
#endif /* SAKURA_REGKEY_6B5694D7_BDD3_4835_8B34_356D3FC110C7_H_ */
