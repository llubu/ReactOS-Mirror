/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse cond_parse
#define yylex   cond_lex
#define yyerror cond_error
#define yylval  cond_lval
#define yychar  cond_char
#define yydebug cond_debug
#define yynerrs cond_nerrs


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     COND_SPACE = 258,
     COND_EOF = 259,
     COND_OR = 260,
     COND_AND = 261,
     COND_NOT = 262,
     COND_XOR = 263,
     COND_IMP = 264,
     COND_EQV = 265,
     COND_LT = 266,
     COND_GT = 267,
     COND_EQ = 268,
     COND_NE = 269,
     COND_GE = 270,
     COND_LE = 271,
     COND_ILT = 272,
     COND_IGT = 273,
     COND_IEQ = 274,
     COND_INE = 275,
     COND_IGE = 276,
     COND_ILE = 277,
     COND_LPAR = 278,
     COND_RPAR = 279,
     COND_TILDA = 280,
     COND_SS = 281,
     COND_ISS = 282,
     COND_ILHS = 283,
     COND_IRHS = 284,
     COND_LHS = 285,
     COND_RHS = 286,
     COND_PERCENT = 287,
     COND_DOLLARS = 288,
     COND_QUESTION = 289,
     COND_AMPER = 290,
     COND_EXCLAM = 291,
     COND_IDENT = 292,
     COND_NUMBER = 293,
     COND_LITER = 294,
     COND_ERROR = 295
   };
#endif
/* Tokens.  */
#define COND_SPACE 258
#define COND_EOF 259
#define COND_OR 260
#define COND_AND 261
#define COND_NOT 262
#define COND_XOR 263
#define COND_IMP 264
#define COND_EQV 265
#define COND_LT 266
#define COND_GT 267
#define COND_EQ 268
#define COND_NE 269
#define COND_GE 270
#define COND_LE 271
#define COND_ILT 272
#define COND_IGT 273
#define COND_IEQ 274
#define COND_INE 275
#define COND_IGE 276
#define COND_ILE 277
#define COND_LPAR 278
#define COND_RPAR 279
#define COND_TILDA 280
#define COND_SS 281
#define COND_ISS 282
#define COND_ILHS 283
#define COND_IRHS 284
#define COND_LHS 285
#define COND_RHS 286
#define COND_PERCENT 287
#define COND_DOLLARS 288
#define COND_QUESTION 289
#define COND_AMPER 290
#define COND_EXCLAM 291
#define COND_IDENT 292
#define COND_NUMBER 293
#define COND_LITER 294
#define COND_ERROR 295




/* Copy the first part of user declarations.  */
#line 1 "cond.y"


/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2003 Mike McCormack for CodeWeavers
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

#define COBJMACROS

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "msi.h"
#include "msiquery.h"
#include "objbase.h"
#include "oleauto.h"

#include "msipriv.h"
#include "msiserver.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#define YYLEX_PARAM info
#define YYPARSE_PARAM info

static int cond_error(const char *str);

WINE_DEFAULT_DEBUG_CHANNEL(msi);

typedef struct tag_yyinput
{
    MSIPACKAGE *package;
    LPCWSTR str;
    INT    n;
    MSICONDITION result;
} COND_input;

struct cond_str {
    LPCWSTR data;
    INT len;
};

static LPWSTR COND_GetString( const struct cond_str *str );
static LPWSTR COND_GetLiteral( const struct cond_str *str );
static int cond_lex( void *COND_lval, COND_input *info);
static const WCHAR szEmpty[] = { 0 };

static INT compare_int( INT a, INT operator, INT b );
static INT compare_string( LPCWSTR a, INT operator, LPCWSTR b );

static INT compare_and_free_strings( LPWSTR a, INT op, LPWSTR b )
{
    INT r;

    r = compare_string( a, op, b );
    msi_free( a );
    msi_free( b );
    return r;
}

static BOOL num_from_prop( LPCWSTR p, INT *val )
{
    INT ret = 0, sign = 1;

    if (!p)
        return FALSE;
    if (*p == '-')
    {
        sign = -1;
        p++;
    }
    if (!*p)
        return FALSE;
    while (*p)
    {
        if( *p < '0' || *p > '9' )
            return FALSE;
        ret = ret*10 + (*p - '0');
        p++;
    }
    *val = ret*sign;
    return TRUE;
}



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 111 "cond.y"
typedef union YYSTYPE {
    struct cond_str str;
    LPWSTR    string;
    INT       value;
} YYSTYPE;
/* Line 196 of yacc.c.  */
#line 286 "cond.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */
#line 298 "cond.tab.c"

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T) && (defined (__STDC__) || defined (__cplusplus))
# include <stddef.h> /* INFRINGES ON USER NAME SPACE */
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if defined (__STDC__) || defined (__cplusplus)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     define YYINCLUDED_STDLIB_H
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2005 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM ((YYSIZE_T) -1)
#  endif
#  ifdef __cplusplus
extern "C" {
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if (! defined (malloc) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if (! defined (free) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifdef __cplusplus
}
#  endif
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short int yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (YYSTYPE))			\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short int yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  28
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   71

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  41
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  12
/* YYNRULES -- Number of rules. */
#define YYNRULES  53
/* YYNRULES -- Number of states. */
#define YYNSTATES  70

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   295

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     5,     6,     8,    12,    16,    20,    24,
      26,    30,    33,    35,    37,    41,    45,    49,    53,    57,
      61,    65,    69,    73,    77,    79,    81,    83,    85,    87,
      89,    91,    93,    95,    97,    99,   101,   103,   105,   107,
     109,   111,   113,   115,   117,   119,   121,   124,   127,   130,
     133,   135,   138,   140
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      42,     0,    -1,    43,    -1,    -1,    44,    -1,    43,     5,
      44,    -1,    43,     9,    44,    -1,    43,     8,    44,    -1,
      43,    10,    44,    -1,    45,    -1,    44,     6,    45,    -1,
       7,    45,    -1,    49,    -1,    47,    -1,    49,    46,    49,
      -1,    50,    46,    49,    -1,    49,    46,    50,    -1,    50,
      46,    50,    -1,    50,    46,    48,    -1,    48,    46,    50,
      -1,    48,    46,    48,    -1,    48,    46,    49,    -1,    49,
      46,    48,    -1,    23,    43,    24,    -1,    13,    -1,    14,
      -1,    11,    -1,    12,    -1,    16,    -1,    15,    -1,    26,
      -1,    19,    -1,    20,    -1,    17,    -1,    18,    -1,    22,
      -1,    21,    -1,    27,    -1,    30,    -1,    31,    -1,    28,
      -1,    29,    -1,    50,    -1,    48,    -1,    39,    -1,    52,
      -1,    33,    51,    -1,    34,    51,    -1,    35,    51,    -1,
      36,    51,    -1,    51,    -1,    32,    51,    -1,    37,    -1,
      38,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   135,   135,   141,   148,   152,   156,   160,   164,   171,
     175,   182,   186,   190,   195,   199,   208,   217,   221,   225,
     229,   233,   238,   243,   251,   252,   253,   254,   255,   256,
     257,   258,   259,   260,   261,   262,   263,   264,   265,   266,
     267,   268,   272,   276,   283,   292,   296,   305,   314,   323,
     335,   342,   356,   365
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "COND_SPACE", "COND_EOF", "COND_OR",
  "COND_AND", "COND_NOT", "COND_XOR", "COND_IMP", "COND_EQV", "COND_LT",
  "COND_GT", "COND_EQ", "COND_NE", "COND_GE", "COND_LE", "COND_ILT",
  "COND_IGT", "COND_IEQ", "COND_INE", "COND_IGE", "COND_ILE", "COND_LPAR",
  "COND_RPAR", "COND_TILDA", "COND_SS", "COND_ISS", "COND_ILHS",
  "COND_IRHS", "COND_LHS", "COND_RHS", "COND_PERCENT", "COND_DOLLARS",
  "COND_QUESTION", "COND_AMPER", "COND_EXCLAM", "COND_IDENT",
  "COND_NUMBER", "COND_LITER", "COND_ERROR", "$accept", "condition",
  "expression", "boolean_term", "boolean_factor", "operator", "value_s",
  "literal", "value_i", "symbol_s", "identifier", "integer", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short int yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    41,    42,    42,    43,    43,    43,    43,    43,    44,
      44,    45,    45,    45,    45,    45,    45,    45,    45,    45,
      45,    45,    45,    45,    46,    46,    46,    46,    46,    46,
      46,    46,    46,    46,    46,    46,    46,    46,    46,    46,
      46,    46,    47,    47,    48,    49,    49,    49,    49,    49,
      50,    50,    51,    52
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     0,     1,     3,     3,     3,     3,     1,
       3,     2,     1,     1,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     2,     2,     2,
       1,     2,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       3,     0,     0,     0,     0,     0,     0,     0,    52,    53,
      44,     0,     2,     4,     9,    13,    43,    12,    42,    50,
      45,    11,     0,    51,    46,    47,    48,    49,     1,     0,
       0,     0,     0,     0,    26,    27,    24,    25,    29,    28,
      33,    34,    31,    32,    36,    35,    30,    37,    40,    41,
      38,    39,     0,     0,     0,    23,     5,     7,     6,     8,
      10,    20,    21,    19,    22,    14,    16,    18,    15,    17
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,    11,    12,    13,    14,    52,    15,    16,    17,    18,
      19,    20
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -15
static const yysigned_char yypact[] =
{
      -7,    -7,    -7,   -14,   -14,   -14,   -14,   -14,   -15,   -15,
     -15,    24,    46,    30,   -15,   -15,    -9,    -9,    -9,   -15,
     -15,   -15,    29,   -15,   -15,   -15,   -15,   -15,   -15,    -7,
      -7,    -7,    -7,    -7,   -15,   -15,   -15,   -15,   -15,   -15,
     -15,   -15,   -15,   -15,   -15,   -15,   -15,   -15,   -15,   -15,
     -15,   -15,     8,     8,     8,   -15,    30,    30,    30,    30,
     -15,   -15,   -15,   -15,   -15,   -15,   -15,   -15,   -15,   -15
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -15,   -15,    50,    33,     0,    -3,   -15,    -4,    14,    17,
      54,   -15
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
       1,    21,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    53,    54,     2,    46,    47,    48,
      49,    50,    51,     8,    28,     3,     4,     5,     6,     7,
       8,     9,    10,    60,    29,     0,    33,    30,    31,    32,
       3,     4,     5,     6,     7,     8,     9,    10,    61,    64,
      67,    29,    22,    55,    30,    31,    32,    23,    24,    25,
      26,    27,    56,    57,    58,    59,    62,    65,    68,    63,
      66,    69
};

static const yysigned_char yycheck[] =
{
       7,     1,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    17,    18,    23,    26,    27,    28,
      29,    30,    31,    37,     0,    32,    33,    34,    35,    36,
      37,    38,    39,    33,     5,    -1,     6,     8,     9,    10,
      32,    33,    34,    35,    36,    37,    38,    39,    52,    53,
      54,     5,     2,    24,     8,     9,    10,     3,     4,     5,
       6,     7,    29,    30,    31,    32,    52,    53,    54,    52,
      53,    54
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     7,    23,    32,    33,    34,    35,    36,    37,    38,
      39,    42,    43,    44,    45,    47,    48,    49,    50,    51,
      52,    45,    43,    51,    51,    51,    51,    51,     0,     5,
       8,     9,    10,     6,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    26,    27,    28,    29,
      30,    31,    46,    46,    46,    24,    44,    44,    44,    44,
      45,    48,    49,    50,    48,    49,    50,    48,    49,    50
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (0)


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (N)								\
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (0)
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
              (Loc).first_line, (Loc).first_column,	\
              (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr,					\
                  Type, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short int *bottom, short int *top)
#else
static void
yy_stack_print (bottom, top)
    short int *bottom;
    short int *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname[yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      size_t yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

#endif /* YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);


# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */






/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()
    ;
#endif
#endif
{
  /* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;

  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short int yyssa[YYINITDEPTH];
  short int *yyss = yyssa;
  short int *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short int *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short int *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a look-ahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to look-ahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 136 "cond.y"
    {
            COND_input* cond = (COND_input*) info;
            cond->result = (yyvsp[0].value);
        ;}
    break;

  case 3:
#line 141 "cond.y"
    {
            COND_input* cond = (COND_input*) info;
            cond->result = MSICONDITION_NONE;
        ;}
    break;

  case 4:
#line 149 "cond.y"
    {
            (yyval.value) = (yyvsp[0].value);
        ;}
    break;

  case 5:
#line 153 "cond.y"
    {
            (yyval.value) = (yyvsp[-2].value) || (yyvsp[0].value);
        ;}
    break;

  case 6:
#line 157 "cond.y"
    {
            (yyval.value) = !(yyvsp[-2].value) || (yyvsp[0].value);
        ;}
    break;

  case 7:
#line 161 "cond.y"
    {
            (yyval.value) = ( (yyvsp[-2].value) || (yyvsp[0].value) ) && !( (yyvsp[-2].value) && (yyvsp[0].value) );
        ;}
    break;

  case 8:
#line 165 "cond.y"
    {
            (yyval.value) = ( (yyvsp[-2].value) && (yyvsp[0].value) ) || ( !(yyvsp[-2].value) && !(yyvsp[0].value) );
        ;}
    break;

  case 9:
#line 172 "cond.y"
    {
            (yyval.value) = (yyvsp[0].value);
        ;}
    break;

  case 10:
#line 176 "cond.y"
    {
            (yyval.value) = (yyvsp[-2].value) && (yyvsp[0].value);
        ;}
    break;

  case 11:
#line 183 "cond.y"
    {
            (yyval.value) = (yyvsp[0].value) ? 0 : 1;
        ;}
    break;

  case 12:
#line 187 "cond.y"
    {
            (yyval.value) = (yyvsp[0].value) ? 1 : 0;
        ;}
    break;

  case 13:
#line 191 "cond.y"
    {
            (yyval.value) = ((yyvsp[0].string) && (yyvsp[0].string)[0]) ? 1 : 0;
            msi_free((yyvsp[0].string));
        ;}
    break;

  case 14:
#line 196 "cond.y"
    {
            (yyval.value) = compare_int( (yyvsp[-2].value), (yyvsp[-1].value), (yyvsp[0].value) );
        ;}
    break;

  case 15:
#line 200 "cond.y"
    {
            int num;
            if (num_from_prop( (yyvsp[-2].string), &num ))
                (yyval.value) = compare_int( num, (yyvsp[-1].value), (yyvsp[0].value) );
            else 
                (yyval.value) = ((yyvsp[-1].value) == COND_NE || (yyvsp[-1].value) == COND_INE );
            msi_free((yyvsp[-2].string));
        ;}
    break;

  case 16:
#line 209 "cond.y"
    {
            int num;
            if (num_from_prop( (yyvsp[0].string), &num ))
                (yyval.value) = compare_int( (yyvsp[-2].value), (yyvsp[-1].value), num );
            else 
                (yyval.value) = ((yyvsp[-1].value) == COND_NE || (yyvsp[-1].value) == COND_INE );
            msi_free((yyvsp[0].string));
        ;}
    break;

  case 17:
#line 218 "cond.y"
    {
            (yyval.value) = compare_and_free_strings( (yyvsp[-2].string), (yyvsp[-1].value), (yyvsp[0].string) );
        ;}
    break;

  case 18:
#line 222 "cond.y"
    {
            (yyval.value) = compare_and_free_strings( (yyvsp[-2].string), (yyvsp[-1].value), (yyvsp[0].string) );
        ;}
    break;

  case 19:
#line 226 "cond.y"
    {
            (yyval.value) = compare_and_free_strings( (yyvsp[-2].string), (yyvsp[-1].value), (yyvsp[0].string) );
        ;}
    break;

  case 20:
#line 230 "cond.y"
    {
            (yyval.value) = compare_and_free_strings( (yyvsp[-2].string), (yyvsp[-1].value), (yyvsp[0].string) );
        ;}
    break;

  case 21:
#line 234 "cond.y"
    {
            (yyval.value) = 0;
            msi_free((yyvsp[-2].string));
        ;}
    break;

  case 22:
#line 239 "cond.y"
    {
            (yyval.value) = 0;
            msi_free((yyvsp[0].string));
        ;}
    break;

  case 23:
#line 244 "cond.y"
    {
            (yyval.value) = (yyvsp[-1].value);
        ;}
    break;

  case 24:
#line 251 "cond.y"
    { (yyval.value) = COND_EQ; ;}
    break;

  case 25:
#line 252 "cond.y"
    { (yyval.value) = COND_NE; ;}
    break;

  case 26:
#line 253 "cond.y"
    { (yyval.value) = COND_LT; ;}
    break;

  case 27:
#line 254 "cond.y"
    { (yyval.value) = COND_GT; ;}
    break;

  case 28:
#line 255 "cond.y"
    { (yyval.value) = COND_LE; ;}
    break;

  case 29:
#line 256 "cond.y"
    { (yyval.value) = COND_GE; ;}
    break;

  case 30:
#line 257 "cond.y"
    { (yyval.value) = COND_SS; ;}
    break;

  case 31:
#line 258 "cond.y"
    { (yyval.value) = COND_IEQ; ;}
    break;

  case 32:
#line 259 "cond.y"
    { (yyval.value) = COND_INE; ;}
    break;

  case 33:
#line 260 "cond.y"
    { (yyval.value) = COND_ILT; ;}
    break;

  case 34:
#line 261 "cond.y"
    { (yyval.value) = COND_IGT; ;}
    break;

  case 35:
#line 262 "cond.y"
    { (yyval.value) = COND_ILE; ;}
    break;

  case 36:
#line 263 "cond.y"
    { (yyval.value) = COND_IGE; ;}
    break;

  case 37:
#line 264 "cond.y"
    { (yyval.value) = COND_ISS; ;}
    break;

  case 38:
#line 265 "cond.y"
    { (yyval.value) = COND_LHS; ;}
    break;

  case 39:
#line 266 "cond.y"
    { (yyval.value) = COND_RHS; ;}
    break;

  case 40:
#line 267 "cond.y"
    { (yyval.value) = COND_ILHS; ;}
    break;

  case 41:
#line 268 "cond.y"
    { (yyval.value) = COND_IRHS; ;}
    break;

  case 42:
#line 273 "cond.y"
    {
        (yyval.string) = (yyvsp[0].string);
    ;}
    break;

  case 43:
#line 277 "cond.y"
    {
        (yyval.string) = (yyvsp[0].string);
    ;}
    break;

  case 44:
#line 284 "cond.y"
    {
            (yyval.string) = COND_GetLiteral(&(yyvsp[0].str));
            if( !(yyval.string) )
                YYABORT;
        ;}
    break;

  case 45:
#line 293 "cond.y"
    {
            (yyval.value) = (yyvsp[0].value);
        ;}
    break;

  case 46:
#line 297 "cond.y"
    {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;
      
            MSI_GetComponentStateW(cond->package, (yyvsp[0].string), &install, &action );
            (yyval.value) = action;
            msi_free( (yyvsp[0].string) );
        ;}
    break;

  case 47:
#line 306 "cond.y"
    {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;
      
            MSI_GetComponentStateW(cond->package, (yyvsp[0].string), &install, &action );
            (yyval.value) = install;
            msi_free( (yyvsp[0].string) );
        ;}
    break;

  case 48:
#line 315 "cond.y"
    {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;
      
            MSI_GetFeatureStateW(cond->package, (yyvsp[0].string), &install, &action );
            (yyval.value) = action;
            msi_free( (yyvsp[0].string) );
        ;}
    break;

  case 49:
#line 324 "cond.y"
    {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;
      
            MSI_GetFeatureStateW(cond->package, (yyvsp[0].string), &install, &action );
            (yyval.value) = install;
            msi_free( (yyvsp[0].string) );
        ;}
    break;

  case 50:
#line 336 "cond.y"
    {
            COND_input* cond = (COND_input*) info;

            (yyval.string) = msi_dup_property( cond->package, (yyvsp[0].string) );
            msi_free( (yyvsp[0].string) );
        ;}
    break;

  case 51:
#line 343 "cond.y"
    {
            UINT len = GetEnvironmentVariableW( (yyvsp[0].string), NULL, 0 );
            (yyval.string) = NULL;
            if (len++)
            {
                (yyval.string) = msi_alloc( len*sizeof (WCHAR) );
                GetEnvironmentVariableW( (yyvsp[0].string), (yyval.string), len );
            }
            msi_free( (yyvsp[0].string) );
        ;}
    break;

  case 52:
#line 357 "cond.y"
    {
            (yyval.string) = COND_GetString(&(yyvsp[0].str));
            if( !(yyval.string) )
                YYABORT;
        ;}
    break;

  case 53:
#line 366 "cond.y"
    {
            LPWSTR szNum = COND_GetString(&(yyvsp[0].str));
            if( !szNum )
                YYABORT;
            (yyval.value) = atoiW( szNum );
            msi_free( szNum );
        ;}
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
#line 1736 "cond.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  int yytype = YYTRANSLATE (yychar);
	  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
	  YYSIZE_T yysize = yysize0;
	  YYSIZE_T yysize1;
	  int yysize_overflow = 0;
	  char *yymsg = 0;
#	  define YYERROR_VERBOSE_ARGS_MAXIMUM 5
	  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
	  int yyx;

#if 0
	  /* This is so xgettext sees the translatable formats that are
	     constructed on the fly.  */
	  YY_("syntax error, unexpected %s");
	  YY_("syntax error, unexpected %s, expecting %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
#endif
	  char *yyfmt;
	  char const *yyf;
	  static char const yyunexpected[] = "syntax error, unexpected %s";
	  static char const yyexpecting[] = ", expecting %s";
	  static char const yyor[] = " or %s";
	  char yyformat[sizeof yyunexpected
			+ sizeof yyexpecting - 1
			+ ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
			   * (sizeof yyor - 1))];
	  char const *yyprefix = yyexpecting;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 1;

	  yyarg[0] = yytname[yytype];
	  yyfmt = yystpcpy (yyformat, yyunexpected);

	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
		  {
		    yycount = 1;
		    yysize = yysize0;
		    yyformat[sizeof yyunexpected - 1] = '\0';
		    break;
		  }
		yyarg[yycount++] = yytname[yyx];
		yysize1 = yysize + yytnamerr (0, yytname[yyx]);
		yysize_overflow |= yysize1 < yysize;
		yysize = yysize1;
		yyfmt = yystpcpy (yyfmt, yyprefix);
		yyprefix = yyor;
	      }

	  yyf = YY_(yyformat);
	  yysize1 = yysize + yystrlen (yyf);
	  yysize_overflow |= yysize1 < yysize;
	  yysize = yysize1;

	  if (!yysize_overflow && yysize <= YYSTACK_ALLOC_MAXIMUM)
	    yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg)
	    {
	      /* Avoid sprintf, as that infringes on the user's name space.
		 Don't have undefined behavior even if the translation
		 produced a string with the wrong number of "%s"s.  */
	      char *yyp = yymsg;
	      int yyi = 0;
	      while ((*yyp = *yyf))
		{
		  if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		    {
		      yyp += yytnamerr (yyp, yyarg[yyi++]);
		      yyf += 2;
		    }
		  else
		    {
		      yyp++;
		      yyf++;
		    }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    {
	      yyerror (YY_("syntax error"));
	      goto yyexhaustedlab;
	    }
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror (YY_("syntax error"));
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
        {
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
        }
      else
	{
	  yydestruct ("Error: discarding", yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (0)
     goto yyerrorlab;

yyvsp -= yylen;
  yyssp -= yylen;
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping", yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token. */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK;
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 375 "cond.y"



static int COND_IsAlpha( WCHAR x )
{
    return( ( ( x >= 'A' ) && ( x <= 'Z' ) ) ||
            ( ( x >= 'a' ) && ( x <= 'z' ) ) ||
            ( ( x == '_' ) ) );
}

static int COND_IsNumber( WCHAR x )
{
    return( (( x >= '0' ) && ( x <= '9' ))  || (x =='-') || (x =='.') );
}

static WCHAR *strstriW( const WCHAR *str, const WCHAR *sub )
{
    LPWSTR strlower, sublower, r;
    strlower = CharLowerW( strdupW( str ) );
    sublower = CharLowerW( strdupW( sub ) );
    r = strstrW( strlower, sublower );
    if (r)
        r = (LPWSTR)str + (r - strlower);
    msi_free( strlower );
    msi_free( sublower );
    return r;
}

static BOOL str_is_number( LPCWSTR str )
{
    int i;

    for (i = 0; i < lstrlenW( str ); i++)
        if (!isdigitW(str[i]))
            return FALSE;

    return TRUE;
}

static INT compare_substring( LPCWSTR a, INT operator, LPCWSTR b )
{
    int lhs, rhs;

    /* substring operators return 0 if LHS is missing */
    if (!a || !*a)
        return 0;

    /* substring operators return 1 if RHS is missing */
    if (!b || !*b)
        return 1;

    /* if both strings contain only numbers, use integer comparison */
    lhs = atoiW(a);
    rhs = atoiW(b);
    if (str_is_number(a) && str_is_number(b))
        return compare_int( lhs, operator, rhs );

    switch (operator)
    {
    case COND_SS:
        return strstrW( a, b ) ? 1 : 0;
    case COND_ISS:
        return strstriW( a, b ) ? 1 : 0;
    case COND_LHS:
    	return 0 == strncmpW( a, b, lstrlenW( b ) );
    case COND_RHS:
    	return 0 == lstrcmpW( a + (lstrlenW( a ) - lstrlenW( b )), b );
    case COND_ILHS:
    	return 0 == strncmpiW( a, b, lstrlenW( b ) );
    case COND_IRHS:
        return 0 == lstrcmpiW( a + (lstrlenW( a ) - lstrlenW( b )), b );
    default:
    	ERR("invalid substring operator\n");
        return 0;
    }
    return 0;
}

static INT compare_string( LPCWSTR a, INT operator, LPCWSTR b )
{
    if (operator >= COND_SS && operator <= COND_RHS)
        return compare_substring( a, operator, b );
	
    /* null and empty string are equivalent */
    if (!a) a = szEmpty;
    if (!b) b = szEmpty;

    /* a or b may be NULL */
    switch (operator)
    {
    case COND_LT:
        return -1 == lstrcmpW( a, b );
    case COND_GT:
        return  1 == lstrcmpW( a, b );
    case COND_EQ:
        return  0 == lstrcmpW( a, b );
    case COND_NE:
        return  0 != lstrcmpW( a, b );
    case COND_GE:
        return -1 != lstrcmpW( a, b );
    case COND_LE:
        return  1 != lstrcmpW( a, b );
    case COND_ILT:
        return -1 == lstrcmpiW( a, b );
    case COND_IGT:
        return  1 == lstrcmpiW( a, b );
    case COND_IEQ:
        return  0 == lstrcmpiW( a, b );
    case COND_INE:
        return  0 != lstrcmpiW( a, b );
    case COND_IGE:
        return -1 != lstrcmpiW( a, b );
    case COND_ILE:
        return  1 != lstrcmpiW( a, b );
    default:
        ERR("invalid string operator\n");
        return 0;
    }
    return 0;
}


static INT compare_int( INT a, INT operator, INT b )
{
    switch (operator)
    {
    case COND_LT:
    case COND_ILT:
        return a < b;
    case COND_GT:
    case COND_IGT:
        return a > b;
    case COND_EQ:
    case COND_IEQ:
        return a == b;
    case COND_NE:
    case COND_INE:
        return a != b;
    case COND_GE:
    case COND_IGE:
        return a >= b;
    case COND_LE:
    case COND_ILE:
        return a <= b;
    case COND_SS:
    case COND_ISS:
        return ( a & b ) ? 1 : 0;
    case COND_RHS:
        return ( ( a & 0xffff ) == b ) ? 1 : 0;
    case COND_LHS:
        return ( ( (a>>16) & 0xffff ) == b ) ? 1 : 0;
    default:
        ERR("invalid integer operator\n");
        return 0;
    }
    return 0;
}


static int COND_IsIdent( WCHAR x )
{
    return( COND_IsAlpha( x ) || COND_IsNumber( x ) || ( x == '_' ) 
            || ( x == '#' ) || (x == '.') );
}

static int COND_GetOperator( COND_input *cond )
{
    static const struct {
        const WCHAR str[4];
        int id;
    } table[] = {
        { {'~','=',0},     COND_IEQ },
        { {'~','<','=',0}, COND_ILE },
        { {'~','>','<',0}, COND_ISS },
        { {'~','>','>',0}, COND_IRHS },
        { {'~','<','>',0}, COND_INE },
        { {'~','<',0},     COND_ILT },
        { {'~','>','=',0}, COND_IGE },
        { {'~','<','<',0}, COND_ILHS },
        { {'~','>',0},     COND_IGT },
        { {'>','=',0},     COND_GE  },
        { {'>','<',0},     COND_SS  },
        { {'<','<',0},     COND_LHS },
        { {'<','>',0},     COND_NE  },
        { {'<','=',0},     COND_LE  },
        { {'>','>',0},     COND_RHS },
        { {'>',0},         COND_GT  },
        { {'<',0},         COND_LT  },
        { {0},             0        }
    };
    LPCWSTR p = &cond->str[cond->n];
    int i = 0, len;

    while ( 1 )
    {
        len = lstrlenW( table[i].str );
        if ( !len || 0 == strncmpW( table[i].str, p, len ) )
            break;
        i++;
    }
    cond->n += len;
    return table[i].id;
}

static int COND_GetOne( struct cond_str *str, COND_input *cond )
{
    int rc, len = 1;
    WCHAR ch;

    str->data = &cond->str[cond->n];

    ch = str->data[0];

    switch( ch )
    {
    case 0: return 0;
    case '(': rc = COND_LPAR; break;
    case ')': rc = COND_RPAR; break;
    case '&': rc = COND_AMPER; break;
    case '!': rc = COND_EXCLAM; break;
    case '$': rc = COND_DOLLARS; break;
    case '?': rc = COND_QUESTION; break;
    case '%': rc = COND_PERCENT; break;
    case ' ': rc = COND_SPACE; break;
    case '=': rc = COND_EQ; break;

    case '~':
    case '<':
    case '>':
        rc = COND_GetOperator( cond );
        if (!rc)
            rc = COND_ERROR;
        return rc;
    default:
        rc = 0;
    }

    if ( rc )
    {
        cond->n += len;
        return rc;
    }

    if (ch == '"' )
    {
        LPCWSTR p = strchrW( str->data + 1, '"' );
	if (!p)
            return COND_ERROR;
        len = p - str->data + 1;
        rc = COND_LITER;
    }
    else if( COND_IsAlpha( ch ) )
    {
        static const WCHAR szNot[] = {'N','O','T',0};
        static const WCHAR szAnd[] = {'A','N','D',0};
        static const WCHAR szXor[] = {'X','O','R',0};
        static const WCHAR szEqv[] = {'E','Q','V',0};
        static const WCHAR szImp[] = {'I','M','P',0};
        static const WCHAR szOr[] = {'O','R',0};

        while( COND_IsIdent( str->data[len] ) )
            len++;
        rc = COND_IDENT;

        if ( len == 3 )
        {
            if ( !strncmpiW( str->data, szNot, len ) )
                rc = COND_NOT;
            else if( !strncmpiW( str->data, szAnd, len ) )
                rc = COND_AND;
            else if( !strncmpiW( str->data, szXor, len ) )
                rc = COND_XOR;
            else if( !strncmpiW( str->data, szEqv, len ) )
                rc = COND_EQV;
            else if( !strncmpiW( str->data, szImp, len ) )
                rc = COND_IMP;
        }
        else if( (len == 2) && !strncmpiW( str->data, szOr, len ) )
            rc = COND_OR;
    }
    else if( COND_IsNumber( ch ) )
    {
        while( COND_IsNumber( str->data[len] ) )
            len++;
        rc = COND_NUMBER;
    }
    else
    {
        ERR("Got unknown character %c(%x)\n",ch,ch);
        return COND_ERROR;
    }

    cond->n += len;
    str->len = len;

    return rc;
}

static int cond_lex( void *COND_lval, COND_input *cond )
{
    int rc;
    struct cond_str *str = COND_lval;

    do {
        rc = COND_GetOne( str, cond );
    } while (rc == COND_SPACE);
    
    return rc;
}

static LPWSTR COND_GetString( const struct cond_str *str )
{
    LPWSTR ret;

    ret = msi_alloc( (str->len+1) * sizeof (WCHAR) );
    if( ret )
    {
        memcpy( ret, str->data, str->len * sizeof(WCHAR));
        ret[str->len]=0;
    }
    TRACE("Got identifier %s\n",debugstr_w(ret));
    return ret;
}

static LPWSTR COND_GetLiteral( const struct cond_str *str )
{
    LPWSTR ret;

    ret = msi_alloc( (str->len-1) * sizeof (WCHAR) );
    if( ret )
    {
        memcpy( ret, str->data+1, (str->len-2) * sizeof(WCHAR) );
        ret[str->len - 2]=0;
    }
    TRACE("Got literal %s\n",debugstr_w(ret));
    return ret;
}

static int cond_error(const char *str)
{
    TRACE("%s\n", str );
    return 0;
}

MSICONDITION MSI_EvaluateConditionW( MSIPACKAGE *package, LPCWSTR szCondition )
{
    COND_input cond;
    MSICONDITION r;

    TRACE("%s\n", debugstr_w( szCondition ) );

    if ( szCondition == NULL )
    	return MSICONDITION_NONE;

    cond.package = package;
    cond.str   = szCondition;
    cond.n     = 0;
    cond.result = MSICONDITION_ERROR;
    
    if ( !cond_parse( &cond ) )
        r = cond.result;
    else
        r = MSICONDITION_ERROR;

    TRACE("%i <- %s\n", r, debugstr_w(szCondition));
    return r;
}

MSICONDITION WINAPI MsiEvaluateConditionW( MSIHANDLE hInstall, LPCWSTR szCondition )
{
    MSIPACKAGE *package;
    UINT ret;

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE);
    if( !package )
    {
        HRESULT hr;
        BSTR condition;
        IWineMsiRemotePackage *remote_package;

        remote_package = (IWineMsiRemotePackage *)msi_get_remote( hInstall );
        if (!remote_package)
            return MSICONDITION_ERROR;

        condition = SysAllocString( szCondition );
        if (!condition)
        {
            IWineMsiRemotePackage_Release( remote_package );
            return ERROR_OUTOFMEMORY;
        }

        hr = IWineMsiRemotePackage_EvaluateCondition( remote_package, condition );

        SysFreeString( condition );
        IWineMsiRemotePackage_Release( remote_package );

        if (FAILED(hr))
        {
            if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
                return HRESULT_CODE(hr);

            return ERROR_FUNCTION_FAILED;
        }

        return ERROR_SUCCESS;
    }

    ret = MSI_EvaluateConditionW( package, szCondition );
    msiobj_release( &package->hdr );
    return ret;
}

MSICONDITION WINAPI MsiEvaluateConditionA( MSIHANDLE hInstall, LPCSTR szCondition )
{
    LPWSTR szwCond = NULL;
    MSICONDITION r;

    szwCond = strdupAtoW( szCondition );
    if( szCondition && !szwCond )
        return MSICONDITION_ERROR;

    r = MsiEvaluateConditionW( hInstall, szwCond );
    msi_free( szwCond );
    return r;
}

