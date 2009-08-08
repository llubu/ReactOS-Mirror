/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <math.h>
#include <limits.h>

#include "jscript.h"
#include "engine.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

#define LONGLONG_MAX (((LONGLONG)0x7fffffff<<32)|0xffffffff)

static const WCHAR NaNW[] = {'N','a','N',0};
static const WCHAR InfinityW[] = {'I','n','f','i','n','i','t','y',0};
static const WCHAR ArrayW[] = {'A','r','r','a','y',0};
static const WCHAR BooleanW[] = {'B','o','o','l','e','a','n',0};
static const WCHAR DateW[] = {'D','a','t','e',0};
static const WCHAR ErrorW[] = {'E','r','r','o','r',0};
static const WCHAR EvalErrorW[] = {'E','v','a','l','E','r','r','o','r',0};
static const WCHAR RangeErrorW[] = {'R','a','n','g','e','E','r','r','o','r',0};
static const WCHAR ReferenceErrorW[] = {'R','e','f','e','r','e','n','c','e','E','r','r','o','r',0};
static const WCHAR SyntaxErrorW[] = {'S','y','n','t','a','x','E','r','r','o','r',0};
static const WCHAR TypeErrorW[] = {'T','y','p','e','E','r','r','o','r',0};
static const WCHAR URIErrorW[] = {'U','R','I','E','r','r','o','r',0};
static const WCHAR FunctionW[] = {'F','u','n','c','t','i','o','n',0};
static const WCHAR NumberW[] = {'N','u','m','b','e','r',0};
static const WCHAR ObjectW[] = {'O','b','j','e','c','t',0};
static const WCHAR StringW[] = {'S','t','r','i','n','g',0};
static const WCHAR RegExpW[] = {'R','e','g','E','x','p',0};
static const WCHAR ActiveXObjectW[] = {'A','c','t','i','v','e','X','O','b','j','e','c','t',0};
static const WCHAR VBArrayW[] = {'V','B','A','r','r','a','y',0};
static const WCHAR EnumeratorW[] = {'E','n','u','m','e','r','a','t','o','r',0};
static const WCHAR escapeW[] = {'e','s','c','a','p','e',0};
static const WCHAR evalW[] = {'e','v','a','l',0};
static const WCHAR isNaNW[] = {'i','s','N','a','N',0};
static const WCHAR isFiniteW[] = {'i','s','F','i','n','i','t','e',0};
static const WCHAR parseIntW[] = {'p','a','r','s','e','I','n','t',0};
static const WCHAR parseFloatW[] = {'p','a','r','s','e','F','l','o','a','t',0};
static const WCHAR unescapeW[] = {'u','n','e','s','c','a','p','e',0};
static const WCHAR _GetObjectW[] = {'G','e','t','O','b','j','e','c','t',0};
static const WCHAR ScriptEngineW[] = {'S','c','r','i','p','t','E','n','g','i','n','e',0};
static const WCHAR ScriptEngineMajorVersionW[] =
    {'S','c','r','i','p','t','E','n','g','i','n','e','M','a','j','o','r','V','e','r','s','i','o','n',0};
static const WCHAR ScriptEngineMinorVersionW[] =
    {'S','c','r','i','p','t','E','n','g','i','n','e','M','i','n','o','r','V','e','r','s','i','o','n',0};
static const WCHAR ScriptEngineBuildVersionW[] =
    {'S','c','r','i','p','t','E','n','g','i','n','e','B','u','i','l','d','V','e','r','s','i','o','n',0};
static const WCHAR CollectGarbageW[] = {'C','o','l','l','e','c','t','G','a','r','b','a','g','e',0};
static const WCHAR MathW[] = {'M','a','t','h',0};
static const WCHAR encodeURIW[] = {'e','n','c','o','d','e','U','R','I',0};

static const WCHAR undefinedW[] = {'u','n','d','e','f','i','n','e','d',0};

static int uri_char_table[] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 00-0f */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 10-1f */
    0,2,0,0,1,0,1,2,2,2,2,1,1,2,2,1, /* 20-2f */
    2,2,2,2,2,2,2,2,2,2,1,1,0,1,0,1, /* 30-3f */
    1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, /* 40-4f */
    2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,2, /* 50-5f */
    0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, /* 60-6f */
    2,2,2,2,2,2,2,2,2,2,2,0,0,0,2,0, /* 70-7f */
};

/* 1 - reserved */
/* 2 - unescaped */

static inline BOOL is_uri_reserved(WCHAR c)
{
    return c < 128 && uri_char_table[c] == 1;
}

static inline BOOL is_uri_unescaped(WCHAR c)
{
    return c < 128 && uri_char_table[c] == 2;
}

static WCHAR int_to_char(int i)
{
    if(i < 10)
        return '0'+i;
    return 'A'+i-10;
}

static HRESULT constructor_call(DispatchEx *constr, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    if(flags != DISPATCH_PROPERTYGET)
        return jsdisp_call_value(constr, lcid, flags, dp, retv, ei, sp);

    V_VT(retv) = VT_DISPATCH;
    V_DISPATCH(retv) = (IDispatch*)_IDispatchEx_(constr);
    IDispatchEx_AddRef(_IDispatchEx_(constr));
    return S_OK;
}

static HRESULT JSGlobal_NaN(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    switch(flags) {
    case DISPATCH_PROPERTYGET:
        num_set_nan(retv);
        break;

    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT JSGlobal_Infinity(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    switch(flags) {
    case DISPATCH_PROPERTYGET:
        num_set_inf(retv, TRUE);
        break;

    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT JSGlobal_Array(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return constructor_call(dispex->ctx->array_constr, lcid, flags, dp, retv, ei, sp);
}

static HRESULT JSGlobal_Boolean(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return constructor_call(dispex->ctx->bool_constr, lcid, flags, dp, retv, ei, sp);
}

static HRESULT JSGlobal_Date(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return constructor_call(dispex->ctx->date_constr, lcid, flags, dp, retv, ei, sp);
}

static HRESULT JSGlobal_Error(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return constructor_call(dispex->ctx->error_constr, lcid, flags, dp, retv, ei, sp);
}

static HRESULT JSGlobal_EvalError(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return constructor_call(dispex->ctx->eval_error_constr, lcid, flags, dp, retv, ei, sp);
}

static HRESULT JSGlobal_RangeError(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return constructor_call(dispex->ctx->range_error_constr, lcid, flags, dp, retv, ei, sp);
}

static HRESULT JSGlobal_ReferenceError(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return constructor_call(dispex->ctx->reference_error_constr, lcid, flags, dp, retv, ei, sp);
}

static HRESULT JSGlobal_SyntaxError(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return constructor_call(dispex->ctx->syntax_error_constr, lcid, flags, dp, retv, ei, sp);
}

static HRESULT JSGlobal_TypeError(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return constructor_call(dispex->ctx->type_error_constr, lcid, flags, dp, retv, ei, sp);
}

static HRESULT JSGlobal_URIError(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return constructor_call(dispex->ctx->uri_error_constr, lcid, flags, dp, retv, ei, sp);
}

static HRESULT JSGlobal_Function(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return constructor_call(dispex->ctx->function_constr, lcid, flags, dp, retv, ei, sp);
}

static HRESULT JSGlobal_Number(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return constructor_call(dispex->ctx->number_constr, lcid, flags, dp, retv, ei, sp);
}

static HRESULT JSGlobal_Object(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return constructor_call(dispex->ctx->object_constr, lcid, flags, dp, retv, ei, sp);
}

static HRESULT JSGlobal_String(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return constructor_call(dispex->ctx->string_constr, lcid, flags, dp, retv, ei, sp);
}

static HRESULT JSGlobal_RegExp(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return constructor_call(dispex->ctx->regexp_constr, lcid, flags, dp, retv, ei, sp);
}

static HRESULT JSGlobal_ActiveXObject(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_VBArray(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_Enumerator(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_escape(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/* ECMA-262 3rd Edition    15.1.2.1 */
static HRESULT JSGlobal_eval(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    parser_ctx_t *parser_ctx;
    VARIANT *arg;
    HRESULT hres;

    TRACE("\n");

    if(!arg_cnt(dp)) {
        if(retv)
            V_VT(retv) = VT_EMPTY;
        return S_OK;
    }

    arg = get_arg(dp, 0);
    if(V_VT(arg) != VT_BSTR) {
        if(retv) {
            V_VT(retv) = VT_EMPTY;
            return VariantCopy(retv, arg);
        }
        return S_OK;
    }

    if(!dispex->ctx->exec_ctx) {
        FIXME("No active exec_ctx\n");
        return E_UNEXPECTED;
    }

    TRACE("parsing %s\n", debugstr_w(V_BSTR(arg)));
    hres = script_parse(dispex->ctx, V_BSTR(arg), NULL, &parser_ctx);
    if(FAILED(hres)) {
        WARN("parse (%s) failed: %08x\n", debugstr_w(V_BSTR(arg)), hres);
        return throw_syntax_error(dispex->ctx, ei, hres, NULL);
    }

    hres = exec_source(dispex->ctx->exec_ctx, parser_ctx, parser_ctx->source, ei, retv);
    parser_release(parser_ctx);

    return hres;
}

static HRESULT JSGlobal_isNaN(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    VARIANT_BOOL ret = VARIANT_FALSE;
    VARIANT num;
    HRESULT hres;

    TRACE("\n");

    if(arg_cnt(dp)) {
        hres = to_number(dispex->ctx, get_arg(dp,0), ei, &num);
        if(FAILED(hres))
            return hres;

        if(V_VT(&num) == VT_R8 && isnan(V_R8(&num)))
            ret = VARIANT_TRUE;
    }else {
        ret = VARIANT_TRUE;
    }

    if(retv) {
        V_VT(retv) = VT_BOOL;
        V_BOOL(retv) = ret;
    }
    return S_OK;
}

static HRESULT JSGlobal_isFinite(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    VARIANT_BOOL ret = VARIANT_FALSE;
    HRESULT hres;

    TRACE("\n");

    if(arg_cnt(dp)) {
        VARIANT num;

        hres = to_number(dispex->ctx, get_arg(dp,0), ei, &num);
        if(FAILED(hres))
            return hres;

        if(V_VT(&num) != VT_R8 || (!isinf(V_R8(&num)) && !isnan(V_R8(&num))))
            ret = VARIANT_TRUE;
    }

    if(retv) {
        V_VT(retv) = VT_BOOL;
        V_BOOL(retv) = ret;
    }
    return S_OK;
}

static INT char_to_int(WCHAR c)
{
    if('0' <= c && c <= '9')
        return c - '0';
    if('a' <= c && c <= 'z')
        return c - 'a' + 10;
    if('A' <= c && c <= 'Z')
        return c - 'A' + 10;
    return 100;
}

static HRESULT JSGlobal_parseInt(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    DOUBLE ret = 0.0;
    INT radix=10, i;
    WCHAR *ptr;
    BOOL neg = FALSE;
    BSTR str;
    HRESULT hres;

    if(!arg_cnt(dp)) {
        if(retv) num_set_nan(retv);
        return S_OK;
    }

    if(arg_cnt(dp) >= 2) {
        hres = to_int32(dispex->ctx, get_arg(dp, 1), ei, &radix);
        if(FAILED(hres))
            return hres;

        if(!radix) {
            radix = 10;
        }else if(radix < 2 || radix > 36) {
            WARN("radix %d out of range\n", radix);
            return E_FAIL;
        }
    }

    hres = to_string(dispex->ctx, get_arg(dp, 0), ei, &str);
    if(FAILED(hres))
        return hres;

    for(ptr = str; isspaceW(*ptr); ptr++);

    switch(*ptr) {
    case '+':
        ptr++;
        break;
    case '-':
        neg = TRUE;
        ptr++;
        break;
    case '0':
        ptr++;
        if(*ptr == 'x' || *ptr == 'X') {
            radix = 16;
            ptr++;
        }
    }

    while(*ptr) {
        i = char_to_int(*ptr++);
        if(i > radix)
            break;

        ret = ret*radix + i;
    }

    SysFreeString(str);

    if(neg)
        ret = -ret;

    if(retv)
        num_set_val(retv, ret);
    return S_OK;
}

static HRESULT JSGlobal_parseFloat(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    LONGLONG d = 0, hlp;
    int exp = 0, length;
    VARIANT *arg;
    WCHAR *str;
    BSTR val_str = NULL;
    BOOL ret_nan = TRUE, positive = TRUE;
    HRESULT hres;

    if(!arg_cnt(dp)) {
        if(retv)
            num_set_nan(retv);
        return S_OK;
    }

    arg = get_arg(dp, 0);
    hres = to_string(dispex->ctx, arg, ei, &val_str);
    if(FAILED(hres))
        return hres;

    str = val_str;
    length = SysStringLen(val_str);

    while(isspaceW(*str)) str++;

    if(*str == '+')
        str++;
    else if(*str == '-') {
        positive = FALSE;
        str++;
    }

    if(isdigitW(*str))
        ret_nan = FALSE;

    while(isdigitW(*str)) {
        hlp = d*10 + *(str++) - '0';
        if(d>LONGLONG_MAX/10 || hlp<0) {
            exp++;
            break;
        }
        else
            d = hlp;
    }
    while(isdigitW(*str)) {
        exp++;
        str++;
    }

    if(*str == '.') str++;

    if(isdigitW(*str))
        ret_nan = FALSE;

    while(isdigitW(*str)) {
        hlp = d*10 + *(str++) - '0';
        if(d>LONGLONG_MAX/10 || hlp<0)
            break;

        d = hlp;
        exp--;
    }
    while(isdigitW(*str))
        str++;

    if(*str && !ret_nan && (*str=='e' || *str=='E')) {
        int sign = 1, e = 0;

        str++;
        if(*str == '+')
            str++;
        else if(*str == '-') {
            sign = -1;
            str++;
        }

        while(isdigitW(*str)) {
            if(e>INT_MAX/10 || (e = e*10 + *str++ - '0')<0)
                e = INT_MAX;
        }
        e *= sign;

        if(exp<0 && e<0 && exp+e>0) exp = INT_MIN;
        else if(exp>0 && e>0 && exp+e<0) exp = INT_MAX;
        else exp += e;
    }

    SysFreeString(val_str);

    if(ret_nan) {
        if(retv)
            num_set_nan(retv);
        return S_OK;
    }

    V_VT(retv) = VT_R8;
    V_R8(retv) = (double)(positive?d:-d)*pow(10, exp);
    return S_OK;
}

static HRESULT JSGlobal_unescape(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_GetObject(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_ScriptEngine(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_ScriptEngineMajorVersion(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_ScriptEngineMinorVersion(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_ScriptEngineBuildVersion(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_CollectGarbage(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_encodeURI(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    const WCHAR *ptr;
    DWORD len = 0, i;
    char buf[4];
    BSTR str, ret;
    WCHAR *rptr;
    HRESULT hres;

    TRACE("\n");

    if(!arg_cnt(dp)) {
        if(retv) {
            ret = SysAllocString(undefinedW);
            if(!ret)
                return E_OUTOFMEMORY;

            V_VT(retv) = VT_BSTR;
            V_BSTR(retv) = ret;
        }

        return S_OK;
    }

    hres = to_string(dispex->ctx, get_arg(dp,0), ei, &str);
    if(FAILED(hres))
        return hres;

    for(ptr = str; *ptr; ptr++) {
        if(is_uri_unescaped(*ptr) || is_uri_reserved(*ptr) || *ptr == '#') {
            len++;
        }else {
            i = WideCharToMultiByte(CP_UTF8, 0, ptr, 1, NULL, 0, NULL, NULL)*3;
            if(!i) {
                FIXME("throw URIError\n");
                return E_FAIL;
            }

            len += i;
        }
    }

    rptr = ret = SysAllocStringLen(NULL, len);
    if(!ret)
        return E_OUTOFMEMORY;

    for(ptr = str; *ptr; ptr++) {
        if(is_uri_unescaped(*ptr) || is_uri_reserved(*ptr) || *ptr == '#') {
            *rptr++ = *ptr;
        }else {
            len = WideCharToMultiByte(CP_UTF8, 0, ptr, 1, buf, sizeof(buf), NULL, NULL);
            for(i=0; i<len; i++) {
                *rptr++ = '%';
                *rptr++ = int_to_char((BYTE)buf[i] >> 4);
                *rptr++ = int_to_char(buf[i] & 0x0f);
            }
        }
    }

    TRACE("%s -> %s\n", debugstr_w(str), debugstr_w(ret));
    if(retv) {
        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = ret;
    }else {
        SysFreeString(ret);
    }
    return S_OK;
}

static const builtin_prop_t JSGlobal_props[] = {
    {ActiveXObjectW,             JSGlobal_ActiveXObject,             PROPF_METHOD},
    {ArrayW,                     JSGlobal_Array,                     PROPF_CONSTR},
    {BooleanW,                   JSGlobal_Boolean,                   PROPF_CONSTR},
    {CollectGarbageW,            JSGlobal_CollectGarbage,            PROPF_METHOD},
    {DateW,                      JSGlobal_Date,                      PROPF_CONSTR},
    {EnumeratorW,                JSGlobal_Enumerator,                PROPF_METHOD},
    {ErrorW,                     JSGlobal_Error,                     PROPF_CONSTR},
    {EvalErrorW,                 JSGlobal_EvalError,                 PROPF_CONSTR},
    {FunctionW,                  JSGlobal_Function,                  PROPF_CONSTR},
    {_GetObjectW,                JSGlobal_GetObject,                 PROPF_METHOD},
    {InfinityW,                  JSGlobal_Infinity,                  0},
/*  {MathW,                      JSGlobal_Math,                      0},  */
    {NaNW,                       JSGlobal_NaN,                       0},
    {NumberW,                    JSGlobal_Number,                    PROPF_CONSTR},
    {ObjectW,                    JSGlobal_Object,                    PROPF_CONSTR},
    {RangeErrorW,                JSGlobal_RangeError,                PROPF_CONSTR},
    {ReferenceErrorW,            JSGlobal_ReferenceError,            PROPF_CONSTR},
    {RegExpW,                    JSGlobal_RegExp,                    PROPF_CONSTR},
    {ScriptEngineW,              JSGlobal_ScriptEngine,              PROPF_METHOD},
    {ScriptEngineBuildVersionW,  JSGlobal_ScriptEngineBuildVersion,  PROPF_METHOD},
    {ScriptEngineMajorVersionW,  JSGlobal_ScriptEngineMajorVersion,  PROPF_METHOD},
    {ScriptEngineMinorVersionW,  JSGlobal_ScriptEngineMinorVersion,  PROPF_METHOD},
    {StringW,                    JSGlobal_String,                    PROPF_CONSTR},
    {SyntaxErrorW,               JSGlobal_SyntaxError,               PROPF_CONSTR},
    {TypeErrorW,                 JSGlobal_TypeError,                 PROPF_CONSTR},
    {URIErrorW,                  JSGlobal_URIError,                  PROPF_CONSTR},
    {VBArrayW,                   JSGlobal_VBArray,                   PROPF_METHOD},
    {encodeURIW,                 JSGlobal_encodeURI,                 PROPF_METHOD},
    {escapeW,                    JSGlobal_escape,                    PROPF_METHOD},
    {evalW,                      JSGlobal_eval,                      PROPF_METHOD|1},
    {isFiniteW,                  JSGlobal_isFinite,                  PROPF_METHOD},
    {isNaNW,                     JSGlobal_isNaN,                     PROPF_METHOD},
    {parseFloatW,                JSGlobal_parseFloat,                PROPF_METHOD},
    {parseIntW,                  JSGlobal_parseInt,                  PROPF_METHOD|2},
    {unescapeW,                  JSGlobal_unescape,                  PROPF_METHOD}
};

static const builtin_info_t JSGlobal_info = {
    JSCLASS_GLOBAL,
    {NULL, NULL, 0},
    sizeof(JSGlobal_props)/sizeof(*JSGlobal_props),
    JSGlobal_props,
    NULL,
    NULL
};

static HRESULT init_constructors(script_ctx_t *ctx, DispatchEx *object_prototype)
{
    HRESULT hres;

    hres = init_function_constr(ctx, object_prototype);
    if(FAILED(hres))
        return hres;

    hres = create_object_constr(ctx, object_prototype, &ctx->object_constr);
    if(FAILED(hres))
        return hres;

    hres = create_array_constr(ctx, &ctx->array_constr);
    if(FAILED(hres))
        return hres;

    hres = create_bool_constr(ctx, &ctx->bool_constr);
    if(FAILED(hres))
        return hres;

    hres = create_date_constr(ctx, &ctx->date_constr);
    if(FAILED(hres))
        return hres;

    hres = init_error_constr(ctx);
    if(FAILED(hres))
        return hres;

    hres = create_number_constr(ctx, &ctx->number_constr);
    if(FAILED(hres))
        return hres;

    hres = create_regexp_constr(ctx, &ctx->regexp_constr);
    if(FAILED(hres))
        return hres;

    hres = create_string_constr(ctx, &ctx->string_constr);
    if(FAILED(hres))
        return hres;

    return S_OK;
}

HRESULT init_global(script_ctx_t *ctx)
{
    DispatchEx *math, *object_prototype;
    VARIANT var;
    HRESULT hres;

    if(ctx->global)
        return S_OK;

    hres = create_object_prototype(ctx, &object_prototype);
    if(FAILED(hres))
        return hres;

    hres = init_constructors(ctx, object_prototype);
    jsdisp_release(object_prototype);
    if(FAILED(hres))
        return hres;

    hres = create_dispex(ctx, &JSGlobal_info, NULL, &ctx->global);
    if(FAILED(hres))
        return hres;

    hres = create_math(ctx, &math);
    if(FAILED(hres))
        return hres;

    V_VT(&var) = VT_DISPATCH;
    V_DISPATCH(&var) = (IDispatch*)_IDispatchEx_(math);
    hres = jsdisp_propput_name(ctx->global, MathW, ctx->lcid, &var, NULL/*FIXME*/, NULL/*FIXME*/);
    jsdisp_release(math);

    return hres;
}
