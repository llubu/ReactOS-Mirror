/*
 * ReactOS ATL
 *
 * Copyright 2009 Andrew Hill <ash77@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include "atlcore.h"


#ifdef _MSC_VER
// It is common to use this in ATL constructors. They only store this for later use, so the usage is safe.
#pragma warning(disable:4355)
#endif

#ifndef _ATL_PACKING
#define _ATL_PACKING 8
#endif

#ifndef _ATL_FREE_THREADED
#ifndef _ATL_APARTMENT_THREADED
#ifndef _ATL_SINGLE_THREADED
#define _ATL_FREE_THREADED
#endif
#endif
#endif

#ifndef ATLTRY
#define ATLTRY(x) x;
#endif

#ifdef _ATL_DISABLE_NO_VTABLE
#define ATL_NO_VTABLE
#else
#define ATL_NO_VTABLE __declspec(novtable)
#endif

namespace ATL
{


template<class T>
class CComPtr
{
public:
    T *p;
public:
    CComPtr()
    {
        p = NULL;
    }

    CComPtr(T *lp)
    {
        p = lp;
        if (p != NULL)
            p->AddRef();
    }

    CComPtr(const CComPtr<T> &lp)
    {
        p = lp.p;
        if (p != NULL)
            p->AddRef();
    }

    ~CComPtr()
    {
        if (p != NULL)
            p->Release();
    }

    T *operator = (T *lp)
    {
        if (p != NULL)
            p->Release();
        p = lp;
        if (p != NULL)
            p->AddRef();
        return *this;
    }

    T *operator = (const CComPtr<T> &lp)
    {
        if (p != NULL)
            p->Release();
        p = lp.p;
        if (p != NULL)
            p->AddRef();
        return *this;
    }

    void Release()
    {
        if (p != NULL)
        {
            p->Release();
            p = NULL;
        }
    }

    void Attach(T *lp)
    {
        if (p != NULL)
            p->Release();
        p = lp;
    }

    T *Detach()
    {
        T *saveP;

        saveP = p;
        p = NULL;
        return saveP;
    }

    T **operator & ()
    {
        ATLASSERT(p == NULL);
        return &p;
    }

    operator T * ()
    {
        return p;
    }

    T *operator -> ()
    {
        ATLASSERT(p != NULL);
        return p;
    }
};


class CComBSTR
{
public:
    BSTR m_str;
public:
    CComBSTR() :
        m_str(NULL)
    {
    }

    CComBSTR(LPCOLESTR pSrc)
    {
        if (pSrc == NULL)
            m_str = NULL;
        else
            m_str = ::SysAllocString(pSrc);
    }

    CComBSTR(int length)
    {
        if (length == 0)
            m_str = NULL;
        else
            m_str = ::SysAllocStringLen(NULL, length);
    }

    CComBSTR(int length, LPCOLESTR pSrc)
    {
        if (length == 0)
            m_str = NULL;
        else
            m_str = ::SysAllocStringLen(pSrc, length);
    }

    CComBSTR(PCSTR pSrc)
    {
        if (pSrc)
        {
            int len = MultiByteToWideChar(CP_THREAD_ACP, 0, pSrc, -1, NULL, 0);
            m_str = ::SysAllocStringLen(NULL, len - 1);
            if (m_str)
            {
                int res = MultiByteToWideChar(CP_THREAD_ACP, 0, pSrc, -1, m_str, len);
                ATLASSERT(res == len);
                if (res != len)
                {
                    ::SysFreeString(m_str);
                    m_str = NULL;
                }
            }
        }
        else
        {
            m_str = NULL;
        }
    }

    CComBSTR(const CComBSTR &other)
    {
        m_str = other.Copy();
    }

    CComBSTR(REFGUID guid)
    {
        OLECHAR szGuid[40];
        ::StringFromGUID2(guid, szGuid, 40);
        m_str = ::SysAllocString(szGuid);
    }

    ~CComBSTR()
    {
        ::SysFreeString(m_str);
        m_str = NULL;
    }

    operator BSTR () const
    {
        return m_str;
    }

    BSTR *operator & ()
    {
        return &m_str;
    }

    CComBSTR &operator = (const CComBSTR &other)
    {
        ::SysFreeString(m_str);
        m_str = other.Copy();
        return *this;
    }

    BSTR Copy() const
    {
        if (!m_str)
            return NULL;
        return ::SysAllocStringLen(m_str, ::SysStringLen(m_str));
    }

    HRESULT CopyTo(BSTR *other) const
    {
        if (!other)
            return E_POINTER;
        *other = Copy();
        return S_OK;
    }

    bool LoadString(HMODULE module, DWORD uID)
    {
        ::SysFreeString(m_str);
        m_str = NULL;
        const wchar_t *ptr = NULL;
        int len = ::LoadStringW(module, uID, (PWSTR)&ptr, 0);
        if (len)
            m_str = ::SysAllocStringLen(ptr, len);
        return m_str != NULL;
    }

    unsigned int Length() const
    {
        return ::SysStringLen(m_str);
    }

    unsigned int ByteLength() const
    {
        return ::SysStringByteLen(m_str);
    }
};


class CComVariant : public tagVARIANT
{
public:
    CComVariant()
    {
        ::VariantInit(this);
    }

    ~CComVariant()
    {
        Clear();
    }

    HRESULT Clear()
    {
        return ::VariantClear(this);
    }
};



}; // namespace ATL

#ifndef _ATL_NO_AUTOMATIC_NAMESPACE
using namespace ATL;
#endif //!_ATL_NO_AUTOMATIC_NAMESPACE

