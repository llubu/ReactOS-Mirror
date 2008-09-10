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

typedef struct _source_elements_t source_elements_t;

typedef struct _parser_ctx_t {
    LONG ref;

    const WCHAR *ptr;
    const WCHAR *begin;
    const WCHAR *end;

    script_ctx_t *script;
    source_elements_t *source;
    BOOL nl;
    HRESULT hres;

    jsheap_t tmp_heap;
    jsheap_t heap;

    struct _parser_ctx_t *next;
} parser_ctx_t;

HRESULT script_parse(script_ctx_t*,const WCHAR*,parser_ctx_t**);
void parser_release(parser_ctx_t*);

int parser_lex(void*,parser_ctx_t*);

static inline void parser_addref(parser_ctx_t *ctx)
{
    ctx->ref++;
}

static inline void *parser_alloc(parser_ctx_t *ctx, DWORD size)
{
    return jsheap_alloc(&ctx->heap, size);
}

static inline void *parser_alloc_tmp(parser_ctx_t *ctx, DWORD size)
{
    return jsheap_alloc(&ctx->tmp_heap, size);
}

struct _exec_ctx_t {
    LONG ref;

    parser_ctx_t *parser;
};

static inline void exec_addref(exec_ctx_t *ctx)
{
    ctx->ref++;
}

void exec_release(exec_ctx_t*);
HRESULT create_exec_ctx(exec_ctx_t**);
HRESULT exec_source(exec_ctx_t*,parser_ctx_t*,source_elements_t*,jsexcept_t*,VARIANT*);

typedef struct _statement_t statement_t;
typedef struct _expression_t expression_t;
typedef struct _parameter_t parameter_t;

typedef struct {
    VARTYPE vt;
    union {
        LONG lval;
        double dval;
        const WCHAR *wstr;
        VARIANT_BOOL bval;
        IDispatch *disp;
    } u;
} literal_t;

typedef struct _variable_declaration_t {
    const WCHAR *identifier;
    expression_t *expr;

    struct _variable_declaration_t *next;
} variable_declaration_t;

typedef struct {
    enum{
        RT_NORMAL,
        RT_RETURN,
        RT_BREAK,
        RT_CONTINUE
    } type;
    jsexcept_t ei;
} return_type_t;

typedef HRESULT (*statement_eval_t)(exec_ctx_t*,statement_t*,return_type_t*,VARIANT*);

struct _statement_t {
    statement_eval_t eval;
    statement_t *next;
};

typedef struct {
    statement_t stat;
    statement_t *stat_list;
} block_statement_t;

typedef struct {
    statement_t stat;
    variable_declaration_t *variable_list;
} var_statement_t;

typedef struct {
    statement_t stat;
    expression_t *expr;
} expression_statement_t;

typedef struct {
    statement_t stat;
    expression_t *expr;
    statement_t *if_stat;
    statement_t *else_stat;
} if_statement_t;

typedef struct {
    statement_t stat;
    expression_t *expr;
    statement_t *statement;
} while_statement_t;

typedef struct {
    statement_t stat;
    variable_declaration_t *variable_list;
    expression_t *begin_expr;
    expression_t *expr;
    expression_t *end_expr;
    statement_t *statement;
} for_statement_t;

typedef struct {
    statement_t stat;
    variable_declaration_t *variable;
    expression_t *expr;
    expression_t *in_expr;
    statement_t *statement;
} forin_statement_t;

typedef struct {
    statement_t stat;
    const WCHAR *identifier;
} branch_statement_t;

typedef struct {
    statement_t stat;
    expression_t *expr;
    statement_t *statement;
} with_statement_t;

typedef struct {
    statement_t stat;
    const WCHAR *identifier;
    statement_t *statement;
} labelled_statement_t;

typedef struct _case_clausule_t {
    expression_t *expr;
    statement_t *stat;

    struct _case_clausule_t *next;
} case_clausule_t;

typedef struct {
    statement_t stat;
    expression_t *expr;
    case_clausule_t *case_list;
} switch_statement_t;

typedef struct {
    const WCHAR *identifier;
    statement_t *statement;
} catch_block_t;

typedef struct {
    statement_t stat;
    statement_t *try_statement;
    catch_block_t *catch_block;
    statement_t *finally_statement;
} try_statement_t;

HRESULT block_statement_eval(exec_ctx_t*,statement_t*,return_type_t*,VARIANT*);
HRESULT var_statement_eval(exec_ctx_t*,statement_t*,return_type_t*,VARIANT*);
HRESULT empty_statement_eval(exec_ctx_t*,statement_t*,return_type_t*,VARIANT*);
HRESULT expression_statement_eval(exec_ctx_t*,statement_t*,return_type_t*,VARIANT*);
HRESULT if_statement_eval(exec_ctx_t*,statement_t*,return_type_t*,VARIANT*);
HRESULT dowhile_statement_eval(exec_ctx_t*,statement_t*,return_type_t*,VARIANT*);
HRESULT while_statement_eval(exec_ctx_t*,statement_t*,return_type_t*,VARIANT*);
HRESULT for_statement_eval(exec_ctx_t*,statement_t*,return_type_t*,VARIANT*);
HRESULT forin_statement_eval(exec_ctx_t*,statement_t*,return_type_t*,VARIANT*);
HRESULT continue_statement_eval(exec_ctx_t*,statement_t*,return_type_t*,VARIANT*);
HRESULT break_statement_eval(exec_ctx_t*,statement_t*,return_type_t*,VARIANT*);
HRESULT return_statement_eval(exec_ctx_t*,statement_t*,return_type_t*,VARIANT*);
HRESULT with_statement_eval(exec_ctx_t*,statement_t*,return_type_t*,VARIANT*);
HRESULT labelled_statement_eval(exec_ctx_t*,statement_t*,return_type_t*,VARIANT*);
HRESULT switch_statement_eval(exec_ctx_t*,statement_t*,return_type_t*,VARIANT*);
HRESULT throw_statement_eval(exec_ctx_t*,statement_t*,return_type_t*,VARIANT*);
HRESULT try_statement_eval(exec_ctx_t*,statement_t*,return_type_t*,VARIANT*);

typedef struct exprval_t exprval_t;

typedef HRESULT (*expression_eval_t)(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);

struct _expression_t {
    expression_eval_t eval;
};

struct _parameter_t {
    const WCHAR *identifier;

    struct _parameter_t *next;
};

typedef struct _function_declaration_t {
    const WCHAR *identifier;
    parameter_t *parameter_list;
    source_elements_t *source_elements;

    struct _function_declaration_t *next;
} function_declaration_t;

struct _source_elements_t {
    statement_t *statement;
    statement_t *statement_tail;
    function_declaration_t *functions;
    function_declaration_t *functions_tail;
};

typedef struct {
    expression_t expr;
    const WCHAR *identifier;
    parameter_t *parameter_list;
    source_elements_t *source_elements;
} function_expression_t;

typedef struct {
    expression_t expr;
    expression_t *expression1;
    expression_t *expression2;
} binary_expression_t;

typedef struct {
    expression_t expr;
    expression_t *expression;
} unary_expression_t;

typedef struct {
    expression_t expr;
    expression_t *expression;
    expression_t *true_expression;
    expression_t *false_expression;
} conditional_expression_t;

typedef struct {
    expression_t expr;
    expression_t *member_expr;
    expression_t *expression;
} array_expression_t;

typedef struct {
    expression_t expr;
    expression_t *expression;
    const WCHAR *identifier;
} member_expression_t;

typedef struct _argument_t {
    expression_t *expr;

    struct _argument_t *next;
} argument_t;

typedef struct {
    expression_t expr;
    expression_t *expression;
    argument_t *argument_list;
} call_expression_t;

typedef struct {
    expression_t expr;
    const WCHAR *identifier;
} identifier_expression_t;

typedef struct {
    expression_t expr;
    literal_t *literal;
} literal_expression_t;

typedef struct _array_element_t {
    int elision;
    expression_t *expr;

    struct _array_element_t *next;
} array_element_t;

typedef struct {
    expression_t expr;
    array_element_t *element_list;
    int length;
} array_literal_expression_t;

typedef struct _prop_val_t {
    literal_t *name;
    expression_t *value;

    struct _prop_val_t *next;
} prop_val_t;

typedef struct {
    expression_t expr;
    prop_val_t *property_list;
} property_value_expression_t;

typedef enum {
     EXPR_COMMA,
     EXPR_OR,
     EXPR_AND,
     EXPR_BOR,
     EXPR_BXOR,
     EXPR_BAND,
     EXPR_INSTANCEOF,
     EXPR_IN,
     EXPR_ADD,
     EXPR_SUB,
     EXPR_MUL,
     EXPR_DIV,
     EXPR_MOD,
     EXPR_DELETE,
     EXPR_VOID,
     EXPR_TYPEOF,
     EXPR_MINUS,
     EXPR_PLUS,
     EXPR_POSTINC,
     EXPR_POSTDEC,
     EXPR_PREINC,
     EXPR_PREDEC,
     EXPR_NEW,
     EXPR_EQ,
     EXPR_EQEQ,
     EXPR_NOTEQ,
     EXPR_NOTEQEQ,
     EXPR_LESS,
     EXPR_LESSEQ,
     EXPR_GREATER,
     EXPR_GREATEREQ,
     EXPR_BITNEG,
     EXPR_LOGNEG,
     EXPR_LSHIFT,
     EXPR_RSHIFT,
     EXPR_RRSHIFT,
     EXPR_ASSIGN,
     EXPR_ASSIGNLSHIFT,
     EXPR_ASSIGNRSHIFT,
     EXPR_ASSIGNRRSHIFT,
     EXPR_ASSIGNADD,
     EXPR_ASSIGNSUB,
     EXPR_ASSIGNMUL,
     EXPR_ASSIGNDIV,
     EXPR_ASSIGNMOD,
     EXPR_ASSIGNAND,
     EXPR_ASSIGNOR,
     EXPR_ASSIGNXOR
} expression_type_t;

HRESULT function_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT conditional_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT array_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT member_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT member_new_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT call_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT this_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT identifier_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT literal_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT array_literal_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT property_value_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);

HRESULT comma_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT logical_or_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT logical_and_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT binary_or_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT binary_xor_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT binary_and_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT instanceof_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT in_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT add_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT sub_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT mul_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT div_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT mod_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT delete_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT void_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT typeof_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT minus_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT plus_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT post_increment_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT post_decrement_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT pre_increment_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT pre_decrement_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT new_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT equal_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT equal2_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT not_equal_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT not_equal2_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT less_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT lesseq_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT greater_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT greatereq_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT binary_negation_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT logical_negation_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT left_shift_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT right_shift_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT right2_shift_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT assign_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT assign_lshift_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT assign_rshift_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT assign_rrshift_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT assign_add_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT assign_sub_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT assign_mul_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT assign_div_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT assign_mod_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT assign_and_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT assign_or_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
HRESULT assign_xor_expression_eval(exec_ctx_t*,expression_t*,DWORD,jsexcept_t*,exprval_t*);
