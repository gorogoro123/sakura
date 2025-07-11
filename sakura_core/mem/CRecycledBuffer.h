﻿/*! @file */
//一時的なメモリブロックをローテーションして使いまわすためのモノ
//Getで取得したメモリブロックは、「ある程度の期間」上書きされないことが保障される。
//その「期間」とは、Getを呼んでから再度CHAIN_COUNT回、Getを呼び出すまでの間である。
//取得したメモリブロックはCRecycledBufferの管理下にあるため、解放してはいけない。
/*
	Copyright (C) 2008, kobake
	Copyright (C) 2018-2022, Sakura Editor Organization

	SPDX-License-Identifier: Zlib
*/
#ifndef SAKURA_CRECYCLEDBUFFER_874E819F_4E31_4431_B5A6_F4BA89FB963E_H_
#define SAKURA_CRECYCLEDBUFFER_874E819F_4E31_4431_B5A6_F4BA89FB963E_H_
#pragma once

class CRecycledBuffer{
//コンフィグ
private:
	static const int BLOCK_SIZE  = 1024; //ブロックサイズ。バイト単位。
	static const int CHAIN_COUNT = 64;   //再利用可能なブロック数。

//コンストラクタ・デストラクタ
public:
	CRecycledBuffer()
	{
		m_current=0;
	}

//インターフェース
public:
	//!一時的に確保されたメモリブロックを取得。このメモリブロックを解放してはいけない。
	template <class T>
	T* GetBuffer(
		size_t* nCount //!< [out] 領域の要素数を受け取る。T単位。
	)
	{
		if(nCount)*nCount=BLOCK_SIZE/sizeof(T);
		m_current = (m_current+1) % CHAIN_COUNT;
		return reinterpret_cast<T*>(m_buf[m_current]);
	}

	//!領域の要素数を取得。T単位
	template <class T>
	size_t GetMaxCount() const
	{
		return BLOCK_SIZE/sizeof(T);
	}

//メンバ変数
private:
	BYTE m_buf[CHAIN_COUNT][BLOCK_SIZE];
	int  m_current;
};

class CRecycledBufferDynamic{
	using Me = CRecycledBufferDynamic;

	static const int CHAIN_COUNT = 64;   //再利用可能なブロック数。

//コンストラクタ・デストラクタ
public:
	CRecycledBufferDynamic()
	{
		m_current=0;
		for(int i=0;i<_countof(m_buf);i++){
			m_buf[i]=nullptr;
		}
	}
	CRecycledBufferDynamic(const Me&) = delete;
	Me& operator = (const Me&) = delete;
	CRecycledBufferDynamic(Me&&) noexcept = delete;
	Me& operator = (Me&&) noexcept = delete;
	~CRecycledBufferDynamic()
	{
		for(int i=0;i<_countof(m_buf);i++){
			if(m_buf[i])delete[] m_buf[i];
		}
	}

//インターフェース
public:
	//!一時的に確保されたメモリブロックを取得。このメモリブロックを解放してはいけない。
	template <class T>
	T* GetBuffer(
		size_t nCount //!< [in] 確保する要素数。T単位。
	)
	{
		m_current = (m_current+1) % CHAIN_COUNT;

		//メモリ確保
		if(m_buf[m_current])delete[] m_buf[m_current];
		m_buf[m_current]=new BYTE[nCount*sizeof(T)];

		return reinterpret_cast<T*>(m_buf[m_current]);
	}

//メンバ変数
private:
	BYTE* m_buf[CHAIN_COUNT];
	int   m_current;
};
#endif /* SAKURA_CRECYCLEDBUFFER_874E819F_4E31_4431_B5A6_F4BA89FB963E_H_ */
