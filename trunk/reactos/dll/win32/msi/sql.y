%{

/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002-2004 Mike McCormack for CodeWeavers
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "query.h"
#include "wine/list.h"
#include "wine/debug.h"

#define YYLEX_PARAM info
#define YYPARSE_PARAM info

extern int SQL_error(const char *str);

WINE_DEFAULT_DEBUG_CHANNEL(msi);

typedef struct tag_SQL_input
{
    MSIDATABASE *db;
    LPCWSTR command;
    DWORD n, len;
    MSIVIEW **view;  /* view structure for the resulting query */
    struct list *mem;
} SQL_input;

static LPWSTR SQL_getstring( void *info, struct sql_str *str );
static INT SQL_getint( void *info );
static int SQL_lex( void *SQL_lval, SQL_input *info );

static void *parser_alloc( void *info, unsigned int sz );
static column_info *parser_alloc_column( void *info, LPCWSTR table, LPCWSTR column );

static BOOL SQL_MarkPrimaryKeys( column_info *cols, column_info *keys);

static struct expr * EXPR_complex( void *info, struct expr *l, UINT op, struct expr *r );
static struct expr * EXPR_column( void *info, column_info *column );
static struct expr * EXPR_ival( void *info, int val );
static struct expr * EXPR_sval( void *info, struct sql_str * );
static struct expr * EXPR_wildcard( void *info );

%}

%pure-parser

%union
{
    struct sql_str str;
    LPWSTR string;
    column_info *column_list;
    MSIVIEW *query;
    struct expr *expr;
    USHORT column_type;
    int integer;
}

%token TK_ABORT TK_AFTER TK_AGG_FUNCTION TK_ALL TK_AND TK_AS TK_ASC
%token TK_BEFORE TK_BEGIN TK_BETWEEN TK_BITAND TK_BITNOT TK_BITOR TK_BY
%token TK_CASCADE TK_CASE TK_CHAR TK_CHECK TK_CLUSTER TK_COLLATE TK_COLUMN
%token TK_COMMA TK_COMMENT TK_COMMIT TK_CONCAT TK_CONFLICT 
%token TK_CONSTRAINT TK_COPY TK_CREATE
%token TK_DEFAULT TK_DEFERRABLE TK_DEFERRED TK_DELETE TK_DELIMITERS TK_DESC
%token TK_DISTINCT TK_DOT TK_DROP TK_EACH
%token TK_ELSE TK_END TK_END_OF_FILE TK_EQ TK_EXCEPT TK_EXPLAIN
%token TK_FAIL TK_FLOAT TK_FOR TK_FOREIGN TK_FROM TK_FUNCTION
%token TK_GE TK_GLOB TK_GROUP TK_GT
%token TK_HAVING TK_HOLD
%token TK_IGNORE TK_ILLEGAL TK_IMMEDIATE TK_IN TK_INDEX TK_INITIALLY
%token <str> TK_ID 
%token TK_INSERT TK_INSTEAD TK_INT 
%token <str> TK_INTEGER
%token TK_INTERSECT TK_INTO TK_IS
%token TK_ISNULL
%token TK_JOIN TK_JOIN_KW
%token TK_KEY
%token TK_LE TK_LIKE TK_LIMIT TK_LONG TK_LONGCHAR TK_LP TK_LSHIFT TK_LT
%token TK_LOCALIZABLE
%token TK_MATCH TK_MINUS
%token TK_NE TK_NOT TK_NOTNULL TK_NULL
%token TK_OBJECT TK_OF TK_OFFSET TK_ON TK_OR TK_ORACLE_OUTER_JOIN TK_ORDER
%token TK_PLUS TK_PRAGMA TK_PRIMARY
%token TK_RAISE TK_REFERENCES TK_REM TK_REPLACE TK_RESTRICT TK_ROLLBACK
%token TK_ROW TK_RP TK_RSHIFT
%token TK_SELECT TK_SEMI TK_SET TK_SHORT TK_SLASH TK_SPACE TK_STAR TK_STATEMENT 
%token <str> TK_STRING
%token TK_TABLE TK_TEMP TK_THEN TK_TRANSACTION TK_TRIGGER
%token TK_UMINUS TK_UNCLOSED_STRING TK_UNION TK_UNIQUE
%token TK_UPDATE TK_UPLUS TK_USING
%token TK_VACUUM TK_VALUES TK_VIEW
%token TK_WHEN TK_WHERE TK_WILDCARD

/*
 * These are extra tokens used by the lexer but never seen by the
 * parser.  We put them in a rule so that the parser generator will
 * add them to the parse.h output file.
 *
 */
%nonassoc END_OF_FILE ILLEGAL SPACE UNCLOSED_STRING COMMENT FUNCTION
          COLUMN AGG_FUNCTION.

%type <string> table id
%type <column_list> selcollist column column_and_type column_def table_def
%type <column_list> column_assignment update_assign_list constlist
%type <query> query multifrom from fromtable selectfrom unorderedsel
%type <query> oneupdate onedelete oneselect onequery onecreate oneinsert
%type <expr> expr val column_val const_val
%type <column_type> column_type data_type data_type_l data_count
%type <integer> number

/* Reference: http://mates.ms.mff.cuni.cz/oracle/doc/ora815nt/server.815/a67779/operator.htm */
%left TK_OR
%left TK_AND
%left TK_NOT
%left TK_EQ TK_NE TK_LT TK_GT TK_LE TK_GE TK_ISNULL TK_LIKE TK_BETWEEN TK_IN
%left TK_PLUS TK_MINUS TK_CONCAT
%right TK_NEGATION

%%

query:
    onequery
    {
        SQL_input* sql = (SQL_input*) info;
        *sql->view = $1;
    }
    ;

onequery:
    oneselect
  | onecreate
  | oneinsert
  | oneupdate
  | onedelete
    ;

oneinsert:
    TK_INSERT TK_INTO table TK_LP selcollist TK_RP TK_VALUES TK_LP constlist TK_RP
        {
            SQL_input *sql = (SQL_input*) info;
            MSIVIEW *insert = NULL; 
            UINT r;

            r = INSERT_CreateView( sql->db, &insert, $3, $5, $9, FALSE ); 
            if( !insert )
                YYABORT;
            $$ = insert;
        }
  | TK_INSERT TK_INTO table TK_LP selcollist TK_RP TK_VALUES TK_LP constlist TK_RP TK_TEMP
        {
            SQL_input *sql = (SQL_input*) info;
            MSIVIEW *insert = NULL; 

            INSERT_CreateView( sql->db, &insert, $3, $5, $9, TRUE ); 
            if( !insert )
                YYABORT;
            $$ = insert;
        }
    ;

onecreate:
    TK_CREATE TK_TABLE table TK_LP table_def TK_RP
        {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *create = NULL; 

            if( !$5 )
                YYABORT;
            CREATE_CreateView( sql->db, &create, $3, $5, FALSE );
            if( !create )
                YYABORT;
            $$ = create;
        }
  | TK_CREATE TK_TABLE table TK_LP table_def TK_RP TK_HOLD
        {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *create = NULL; 

            if( !$5 )
                YYABORT;
            CREATE_CreateView( sql->db, &create, $3, $5, TRUE );
            if( !create )
                YYABORT;
            $$ = create;
        }
    ;

oneupdate:
    TK_UPDATE table TK_SET update_assign_list TK_WHERE expr
        {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *update = NULL; 

            UPDATE_CreateView( sql->db, &update, $2, $4, $6 );
            if( !update )
                YYABORT;
            $$ = update;
        }
    ;

onedelete:
    TK_DELETE from
        {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *delete = NULL; 

            DELETE_CreateView( sql->db, &delete, $2 );
            if( !delete )
                YYABORT;
            $$ = delete;
        }
    ;

table_def:
    column_def TK_PRIMARY TK_KEY selcollist
        {
            if( SQL_MarkPrimaryKeys( $1, $4 ) )
                $$ = $1;
            else
                $$ = NULL;
        }
    ;

column_def:
    column_def TK_COMMA column_and_type
        {
            column_info *ci;

            for( ci = $1; ci->next; ci = ci->next )
                ;

            ci->next = $3;
            $$ = $1;
        }
  | column_and_type
        {
            $$ = $1;
        }
    ;

column_and_type:
    column column_type
        {
            $$ = $1;
            $$->type = $2 | MSITYPE_VALID;
        }
    ;

column_type:
    data_type_l
        {
            $$ = $1;
        }
  | data_type_l TK_LOCALIZABLE
        {
            $$ = $1 | MSITYPE_LOCALIZABLE;
        }
    ;

data_type_l:
    data_type
        {
            $$ |= MSITYPE_NULLABLE;
        }
  | data_type TK_NOT TK_NULL
        {
            $$ = $1;
        }
    ;

data_type:
    TK_CHAR
        {
            $$ = MSITYPE_STRING | 1;
        }
  | TK_CHAR TK_LP data_count TK_RP
        {
            $$ = MSITYPE_STRING | 0x400 | $3;
        }
  | TK_LONGCHAR
        {
            $$ = 2;
        }
  | TK_SHORT
        {
            $$ = 2;
        }
  | TK_INT
        {
            $$ = 2;
        }
  | TK_LONG
        {
            $$ = 4;
        }
  | TK_OBJECT
        {
            $$ = MSITYPE_STRING | MSITYPE_VALID;
        }
    ;

data_count:
    number
        {
            if( ( $1 > 255 ) || ( $1 < 0 ) )
                YYABORT;
            $$ = $1;
        }
    ;

oneselect:
    unorderedsel TK_ORDER TK_BY selcollist
        {
            SQL_input* sql = (SQL_input*) info;

            $$ = NULL;
            if( $4 )
                ORDER_CreateView( sql->db, &$$, $1, $4 );
            else
                $$ = $1;
            if( !$$ )
                YYABORT;
        }
  | unorderedsel
    ;

unorderedsel:
    TK_SELECT selectfrom
        {
            $$ = $2;
        }
  | TK_SELECT TK_DISTINCT selectfrom
        {
            SQL_input* sql = (SQL_input*) info;
            UINT r;

            $$ = NULL;
            r = DISTINCT_CreateView( sql->db, &$$, $3 );
            if (r != ERROR_SUCCESS)
            {
                $3->ops->delete($3);
                YYABORT;
            }
        }
    ;

selectfrom:
    selcollist multifrom 
        {
            SQL_input* sql = (SQL_input*) info;
            UINT r;

            $$ = NULL;
            if( $1 )
            {
                r = SELECT_CreateView( sql->db, &$$, $2, $1 );
                if (r != ERROR_SUCCESS)
                {
                    $2->ops->delete($2);
                    YYABORT;
                }
            }
            else
                $$ = $2;
        }
    ;

selcollist:
    column 
  | column TK_COMMA selcollist
        { 
            $1->next = $3;
        }
  | TK_STAR
        {
            $$ = NULL;
        }
    ;

multifrom:
    from
  | TK_FROM table TK_COMMA table TK_WHERE expr
        {
            SQL_input* sql = (SQL_input*) info;
            UINT r;

            /* only support inner joins on two tables */
            r = JOIN_CreateView( sql->db, &$$, $2, $4, $6 );
            if( r != ERROR_SUCCESS )
                YYABORT;
        }
    ;

from:
    fromtable
  | fromtable TK_WHERE expr
        { 
            SQL_input* sql = (SQL_input*) info;
            UINT r;

            $$ = NULL;
            r = WHERE_CreateView( sql->db, &$$, $1, $3 );
            if( r != ERROR_SUCCESS )
            {
                $1->ops->delete( $1 );
                YYABORT;
            }
        }
    ;

fromtable:
    TK_FROM table
        {
            SQL_input* sql = (SQL_input*) info;
            UINT r;

            $$ = NULL;
            r = TABLE_CreateView( sql->db, $2, &$$ );
            if( r != ERROR_SUCCESS || !$$ )
                YYABORT;
        }
    ;

expr:
    TK_LP expr TK_RP
        {
            $$ = $2;
            if( !$$ )
                YYABORT;
        }
  | expr TK_AND expr
        {
            $$ = EXPR_complex( info, $1, OP_AND, $3 );
            if( !$$ )
                YYABORT;
        }
  | expr TK_OR expr
        {
            $$ = EXPR_complex( info, $1, OP_OR, $3 );
            if( !$$ )
                YYABORT;
        }
  | column_val TK_EQ val
        {
            $$ = EXPR_complex( info, $1, OP_EQ, $3 );
            if( !$$ )
                YYABORT;
        }
  | column_val TK_GT val
        {
            $$ = EXPR_complex( info, $1, OP_GT, $3 );
            if( !$$ )
                YYABORT;
        }
  | column_val TK_LT val
        {
            $$ = EXPR_complex( info, $1, OP_LT, $3 );
            if( !$$ )
                YYABORT;
        }
  | column_val TK_LE val
        {
            $$ = EXPR_complex( info, $1, OP_LE, $3 );
            if( !$$ )
                YYABORT;
        }
  | column_val TK_GE val
        {
            $$ = EXPR_complex( info, $1, OP_GE, $3 );
            if( !$$ )
                YYABORT;
        }
  | column_val TK_NE val
        {
            $$ = EXPR_complex( info, $1, OP_NE, $3 );
            if( !$$ )
                YYABORT;
        }
  | column_val TK_IS TK_NULL
        {
            $$ = EXPR_complex( info, $1, OP_ISNULL, NULL );
            if( !$$ )
                YYABORT;
        }
  | column_val TK_IS TK_NOT TK_NULL
        {
            $$ = EXPR_complex( info, $1, OP_NOTNULL, NULL );
            if( !$$ )
                YYABORT;
        }
    ;

val:
    column_val
  | const_val
    ;

constlist:
    const_val
        {
            $$ = parser_alloc_column( info, NULL, NULL );
            if( !$$ )
                YYABORT;
            $$->val = $1;
        }
  | const_val TK_COMMA constlist
        {
            $$ = parser_alloc_column( info, NULL, NULL );
            if( !$$ )
                YYABORT;
            $$->val = $1;
            $$->next = $3;
        }
    ;

update_assign_list:
    column_assignment
  | column_assignment TK_COMMA update_assign_list
        {
            $$ = $1;
            $$->next = $3;
        }
    ;

column_assignment:
    column TK_EQ const_val
        {
            $$ = $1;
            $$->val = $3;
        }
    ;

const_val:
    number
        {
            $$ = EXPR_ival( info, $1 );
            if( !$$ )
                YYABORT;
        }
  | TK_MINUS number %prec TK_NEGATION
        {
            $$ = EXPR_ival( info, -$2 );
            if( !$$ )
                YYABORT;
        }
  | TK_STRING
        {
            $$ = EXPR_sval( info, &$1 );
            if( !$$ )
                YYABORT;
        }
  | TK_WILDCARD
        {
            $$ = EXPR_wildcard( info );
            if( !$$ )
                YYABORT;
        }
    ;

column_val:
    column 
        {
            $$ = EXPR_column( info, $1 );
            if( !$$ )
                YYABORT;
        }
    ;

column:
    table TK_DOT id
        {
            $$ = parser_alloc_column( info, $1, $3 );
            if( !$$ )
                YYABORT;
        }
  | id
        {
            $$ = parser_alloc_column( info, NULL, $1 );
            if( !$$ )
                YYABORT;
        }
    ;

table:
    id
        {
            $$ = $1;
        }
    ;

id:
    TK_ID
        {
            $$ = SQL_getstring( info, &$1 );
            if( !$$ )
                YYABORT;
        }
    ;

number:
    TK_INTEGER
        {
            $$ = SQL_getint( info );
        }
    ;

%%

static void *parser_alloc( void *info, unsigned int sz )
{
    SQL_input* sql = (SQL_input*) info;
    struct list *mem;

    mem = msi_alloc( sizeof (struct list) + sz );
    list_add_tail( sql->mem, mem );
    return &mem[1];
}

static column_info *parser_alloc_column( void *info, LPCWSTR table, LPCWSTR column )
{
    column_info *col;

    col = parser_alloc( info, sizeof (*col) );
    if( col )
    {
        col->table = table;
        col->column = column;
        col->val = NULL;
        col->type = 0;
        col->next = NULL;
    }

    return col;
}

int SQL_lex( void *SQL_lval, SQL_input *sql )
{
    int token;
    struct sql_str * str = SQL_lval;

    do
    {
        sql->n += sql->len;
        if( ! sql->command[sql->n] )
            return 0;  /* end of input */

        /* TRACE("string : %s\n", debugstr_w(&sql->command[sql->n])); */
        sql->len = sqliteGetToken( &sql->command[sql->n], &token );
        if( sql->len==0 )
            break;
        str->data = &sql->command[sql->n];
        str->len = sql->len;
    }
    while( token == TK_SPACE );

    /* TRACE("token : %d (%s)\n", token, debugstr_wn(&sql->command[sql->n], sql->len)); */
    
    return token;
}

LPWSTR SQL_getstring( void *info, struct sql_str *strdata )
{
    LPCWSTR p = strdata->data;
    UINT len = strdata->len;
    LPWSTR str;

    /* if there's quotes, remove them */
    if( ( (p[0]=='`') && (p[len-1]=='`') ) || 
        ( (p[0]=='\'') && (p[len-1]=='\'') ) )
    {
        p++;
        len -= 2;
    }
    str = parser_alloc( info, (len + 1)*sizeof(WCHAR) );
    if( !str )
        return str;
    memcpy( str, p, len*sizeof(WCHAR) );
    str[len]=0;

    return str;
}

INT SQL_getint( void *info )
{
    SQL_input* sql = (SQL_input*) info;
    LPCWSTR p = &sql->command[sql->n];
    INT i, r = 0;

    for( i=0; i<sql->len; i++ )
    {
        if( '0' > p[i] || '9' < p[i] )
        {
            ERR("should only be numbers here!\n");
            break;
        }
        r = (p[i]-'0') + r*10;
    }

    return r;
}

int SQL_error( const char *str )
{
    return 0;
}

static struct expr * EXPR_wildcard( void *info )
{
    struct expr *e = parser_alloc( info, sizeof *e );
    if( e )
    {
        e->type = EXPR_WILDCARD;
    }
    return e;
}

static struct expr * EXPR_complex( void *info, struct expr *l, UINT op, struct expr *r )
{
    struct expr *e = parser_alloc( info, sizeof *e );
    if( e )
    {
        e->type = EXPR_COMPLEX;
        e->u.expr.left = l;
        e->u.expr.op = op;
        e->u.expr.right = r;
    }
    return e;
}

static struct expr * EXPR_column( void *info, column_info *column )
{
    struct expr *e = parser_alloc( info, sizeof *e );
    if( e )
    {
        e->type = EXPR_COLUMN;
        e->u.sval = column->column;
    }
    return e;
}

static struct expr * EXPR_ival( void *info, int val )
{
    struct expr *e = parser_alloc( info, sizeof *e );
    if( e )
    {
        e->type = EXPR_IVAL;
        e->u.ival = val;
    }
    return e;
}

static struct expr * EXPR_sval( void *info, struct sql_str *str )
{
    struct expr *e = parser_alloc( info, sizeof *e );
    if( e )
    {
        e->type = EXPR_SVAL;
        e->u.sval = SQL_getstring( info, str );
    }
    return e;
}

static BOOL SQL_MarkPrimaryKeys( column_info *cols,
                                 column_info *keys )
{
    column_info *k;
    BOOL found = TRUE;

    for( k = keys; k && found; k = k->next )
    {
        column_info *c;

        found = FALSE;
        for( c = cols; c && !found; c = c->next )
        {
             if( lstrcmpW( k->column, c->column ) )
                 continue;
             c->type |= MSITYPE_KEY;
             found = TRUE;
        }
    }

    return found;
}

UINT MSI_ParseSQL( MSIDATABASE *db, LPCWSTR command, MSIVIEW **phview,
                   struct list *mem )
{
    SQL_input sql;
    int r;

    *phview = NULL;

    sql.db = db;
    sql.command = command;
    sql.n = 0;
    sql.len = 0;
    sql.view = phview;
    sql.mem = mem;

    r = SQL_parse(&sql);

    TRACE("Parse returned %d\n", r);
    if( r )
    {
        *sql.view = NULL;
        return ERROR_BAD_QUERY_SYNTAX;
    }

    return ERROR_SUCCESS;
}
