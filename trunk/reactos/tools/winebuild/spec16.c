/*
 * 16-bit spec files
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Martin von Loewis
 * Copyright 1995, 1996, 1997 Alexandre Julliard
 * Copyright 1997 Eric Youngdale
 * Copyright 1999 Ulrich Weigand
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <assert.h>
#include <ctype.h>

#include "wine/winbase16.h"

#include "build.h"

/* sequences of nops to fill a certain number of words */
static const char * const nop_sequence[4] =
{
    ".byte 0x89,0xf6",  /* mov %esi,%esi */
    ".byte 0x8d,0x74,0x26,0x00",  /* lea 0x00(%esi),%esi */
    ".byte 0x8d,0xb6,0x00,0x00,0x00,0x00",  /* lea 0x00000000(%esi),%esi */
    ".byte 0x8d,0x74,0x26,0x00,0x8d,0x74,0x26,0x00" /* lea 0x00(%esi),%esi; lea 0x00(%esi),%esi */
};

static inline int is_function( const ORDDEF *odp )
{
    return (odp->type == TYPE_CDECL ||
            odp->type == TYPE_PASCAL ||
            odp->type == TYPE_VARARGS ||
            odp->type == TYPE_STUB);
}

/*******************************************************************
 *         output_entries
 *
 * Output entries for individual symbols in the entry table.
 */
static void output_entries( FILE *outfile, DLLSPEC *spec, int first, int count )
{
    int i;

    for (i = 0; i < count; i++)
    {
        ORDDEF *odp = spec->ordinals[first + i];
        fprintf( outfile, "\t.byte 0x03\n" );  /* flags: exported & public data */
        switch (odp->type)
        {
        case TYPE_CDECL:
        case TYPE_PASCAL:
        case TYPE_VARARGS:
        case TYPE_STUB:
            fprintf( outfile, "\t%s .L__wine_%s_%u-.L__wine_spec_code_segment\n",
                     get_asm_short_keyword(),
                     make_c_identifier(spec->dll_name), first + i );
            break;
        case TYPE_VARIABLE:
            fprintf( outfile, "\t%s .L__wine_%s_%u-.L__wine_spec_data_segment\n",
                     get_asm_short_keyword(),
                     make_c_identifier(spec->dll_name), first + i );
            break;
        case TYPE_ABS:
            fprintf( outfile, "\t%s 0x%04x  /* %s */\n",
                     get_asm_short_keyword(), odp->u.abs.value, odp->name );
            break;
        default:
            assert(0);
        }
    }
}


/*******************************************************************
 *         output_entry_table
 */
static void output_entry_table( FILE *outfile, DLLSPEC *spec )
{
    int i, prev = 0, prev_sel = -1, bundle_count = 0;

    for (i = 1; i <= spec->limit; i++)
    {
        int selector = 0;
        ORDDEF *odp = spec->ordinals[i];
        if (!odp) continue;

        switch (odp->type)
        {
        case TYPE_CDECL:
        case TYPE_PASCAL:
        case TYPE_VARARGS:
        case TYPE_STUB:
            selector = 1;  /* Code selector */
            break;
        case TYPE_VARIABLE:
            selector = 2;  /* Data selector */
            break;
        case TYPE_ABS:
            selector = 0xfe;  /* Constant selector */
            break;
        default:
            continue;
        }

        if (prev + 1 != i || prev_sel != selector || bundle_count == 255)
        {
            /* need to start a new bundle */

            /* flush previous bundle */
            if (bundle_count)
            {
                fprintf( outfile, "\t/* %s.%d - %s.%d */\n",
                         spec->dll_name, prev - bundle_count + 1, spec->dll_name, prev );
                fprintf( outfile, "\t.byte 0x%02x,0x%02x\n", bundle_count, prev_sel );
                output_entries( outfile, spec, prev - bundle_count + 1, bundle_count );
            }

            if (prev + 1 != i)
            {
                int skip = i - (prev + 1);
                while (skip > 255)
                {
                    fprintf( outfile, "\t.byte 0xff,0x00\n" );
                    skip -= 255;
                }
                fprintf( outfile, "\t.byte 0x%02x,0x00\n", skip );
            }

            bundle_count = 0;
            prev_sel = selector;
        }
        bundle_count++;
        prev = i;
    }

    /* flush last bundle */
    if (bundle_count)
    {
        fprintf( outfile, "\t.byte 0x%02x,0x%02x\n", bundle_count, prev_sel );
        output_entries( outfile, spec, prev - bundle_count + 1, bundle_count );
    }
    fprintf( outfile, "\t.byte 0x00\n" );
}


/*******************************************************************
 *         output_resident_name
 */
static void output_resident_name( FILE *outfile, const char *string, int ordinal )
{
    unsigned int i, len = strlen(string);

    fprintf( outfile, "\t.byte 0x%02x", len );
    for (i = 0; i < len; i++) fprintf( outfile, ",0x%02x", (unsigned char)toupper(string[i]) );
    fprintf( outfile, " /* %s */\n", string );
    fprintf( outfile, "\t%s %u\n", get_asm_short_keyword(), ordinal );
}


/*******************************************************************
 *         get_callfrom16_name
 */
static const char *get_callfrom16_name( const ORDDEF *odp )
{
    static char buffer[80];

    sprintf( buffer, "%s_%s_%s",
             (odp->type == TYPE_PASCAL) ? "p" :
             (odp->type == TYPE_VARARGS) ? "v" : "c",
             (odp->flags & FLAG_REGISTER) ? "regs" :
             (odp->flags & FLAG_RET16) ? "word" : "long",
             odp->u.func.arg_types );
    return buffer;
}


/*******************************************************************
 *         get_relay_name
 */
static const char *get_relay_name( const ORDDEF *odp )
{
    static char buffer[80];
    char *p;

    switch(odp->type)
    {
    case TYPE_PASCAL:
        strcpy( buffer, "p_" );
        break;
    case TYPE_VARARGS:
        strcpy( buffer, "v_" );
        break;
    case TYPE_CDECL:
    case TYPE_STUB:
        strcpy( buffer, "c_" );
        break;
    default:
        assert(0);
    }
    strcat( buffer, odp->u.func.arg_types );
    for (p = buffer + 2; *p; p++)
    {
        /* map string types to the corresponding plain pointer type */
        if (*p == 't') *p = 'p';
        else if (*p == 'T') *p = 'l';
    }
    if (odp->flags & FLAG_REGISTER) strcat( buffer, "_regs" );
    return buffer;
}


/*******************************************************************
 *         get_function_argsize
 */
static int get_function_argsize( const ORDDEF *odp )
{
    const char *args;
    int argsize = 0;

    for (args = odp->u.func.arg_types; *args; args++)
    {
        switch (*args)
        {
        case 'w':  /* word */
        case 's':  /* s_word */
            argsize += 2;
            break;
        case 'l':  /* long or segmented pointer */
        case 'T':  /* segmented pointer to null-terminated string */
        case 'p':  /* linear pointer */
        case 't':  /* linear pointer to null-terminated string */
            argsize += 4;
            break;
        default:
            assert(0);
        }
    }
    return argsize;
}


/*******************************************************************
 *         output_call16_function
 *
 * Build a 16-bit-to-Wine callback glue function.
 *
 * The generated routines are intended to be used as argument conversion
 * routines to be called by the CallFrom16... core. Thus, the prototypes of
 * the generated routines are (see also CallFrom16):
 *
 *  extern WORD WINAPI __wine_spec_call16_C_xxx( FARPROC func, LPBYTE args );
 *  extern LONG WINAPI __wine_spec_call16_C_xxx( FARPROC func, LPBYTE args );
 *  extern void WINAPI __wine_spec_call16_C_xxx_regs( FARPROC func, LPBYTE args, CONTEXT86 *context );
 *
 * where 'C' is the calling convention ('p' for pascal or 'c' for cdecl),
 * and each 'x' is an argument  ('w'=word, 's'=signed word, 'l'=long,
 * 'p'=linear pointer, 't'=linear pointer to null-terminated string,
 * 'T'=segmented pointer to null-terminated string).
 *
 * The generated routines fetch the arguments from the 16-bit stack (pointed
 * to by 'args'); the offsets of the single argument values are computed
 * according to the calling convention and the argument types.  Then, the
 * 32-bit entry point is called with these arguments.
 *
 * For register functions, the arguments (if present) are converted just
 * the same as for normal functions, but in addition the CONTEXT86 pointer
 * filled with the current register values is passed to the 32-bit routine.
 */
static void output_call16_function( FILE *outfile, ORDDEF *odp )
{
    char name[256];
    int i, pos;
    const char *args = odp->u.func.arg_types;
    int argsize = get_function_argsize( odp );
    int needs_ldt = strchr( args, 'p' ) || strchr( args, 't' );

    sprintf( name, ".L__wine_spec_call16_%s", get_relay_name(odp) );

    fprintf( outfile, "\t.align %d\n", get_alignment(4) );
    fprintf( outfile, "\t%s\n", func_declaration(name) );
    fprintf( outfile, "%s:\n", name );
    fprintf( outfile, "\tpushl %%ebp\n" );
    fprintf( outfile, "\tmovl %%esp,%%ebp\n" );
    if (needs_ldt)
    {
        fprintf( outfile, "\tpushl %%esi\n" );
        if (UsePIC)
        {
            fprintf( outfile, "\tcall 1f\n" );
            fprintf( outfile, "1:\tpopl %%eax\n" );
            fprintf( outfile, "\tmovl wine_ldt_copy_ptr-1b(%%eax),%%esi\n" );
        }
        else
            fprintf( outfile, "\tmovl $%s,%%esi\n", asm_name("wine_ldt_copy") );
    }

    if (args[0] || odp->type == TYPE_VARARGS)
        fprintf( outfile, "\tmovl 12(%%ebp),%%ecx\n" );  /* args */

    if (odp->flags & FLAG_REGISTER)
    {
        fprintf( outfile, "\tpushl 16(%%ebp)\n" );  /* context */
    }
    else if (odp->type == TYPE_VARARGS)
    {
        fprintf( outfile, "\tleal %d(%%ecx),%%eax\n", argsize );
        fprintf( outfile, "\tpushl %%eax\n" );  /* va_list16 */
    }

    pos = (odp->type == TYPE_PASCAL) ? 0 : argsize;
    for (i = strlen(args) - 1; i >= 0; i--)
    {
        switch (args[i])
        {
        case 'w':  /* word */
            if (odp->type != TYPE_PASCAL) pos -= 2;
            fprintf( outfile, "\tmovzwl %d(%%ecx),%%eax\n", pos );
            fprintf( outfile, "\tpushl %%eax\n" );
            if (odp->type == TYPE_PASCAL) pos += 2;
            break;

        case 's':  /* s_word */
            if (odp->type != TYPE_PASCAL) pos -= 2;
            fprintf( outfile, "\tmovswl %d(%%ecx),%%eax\n", pos );
            fprintf( outfile, "\tpushl %%eax\n" );
            if (odp->type == TYPE_PASCAL) pos += 2;
            break;

        case 'l':  /* long or segmented pointer */
        case 'T':  /* segmented pointer to null-terminated string */
            if (odp->type != TYPE_PASCAL) pos -= 4;
            fprintf( outfile, "\tpushl %d(%%ecx)\n", pos );
            if (odp->type == TYPE_PASCAL) pos += 4;
            break;

        case 'p':  /* linear pointer */
        case 't':  /* linear pointer to null-terminated string */
            if (odp->type != TYPE_PASCAL) pos -= 4;
            fprintf( outfile, "\tmovzwl %d(%%ecx),%%edx\n", pos + 2 ); /* sel */
            fprintf( outfile, "\tshr $3,%%edx\n" );
            fprintf( outfile, "\tmovzwl %d(%%ecx),%%eax\n", pos ); /* offset */
            fprintf( outfile, "\taddl (%%esi,%%edx,4),%%eax\n" );
            fprintf( outfile, "\tpushl %%eax\n" );
            if (odp->type == TYPE_PASCAL) pos += 4;
            break;

        default:
            assert(0);
        }
    }

    fprintf( outfile, "\tcall *8(%%ebp)\n" );

    if (needs_ldt) fprintf( outfile, "\tmovl -4(%%ebp),%%esi\n" );
    if (odp->flags & FLAG_RET16) fprintf( outfile, "\tmovzwl %%ax,%%eax\n" );

    fprintf( outfile, "\tleave\n" );
    fprintf( outfile, "\tret\n" );
    output_function_size( outfile, name );
}


/*******************************************************************
 *         callfrom16_type_compare
 *
 * Compare two callfrom16 sequences.
 */
static int callfrom16_type_compare( const void *e1, const void *e2 )
{
    const ORDDEF *odp1 = *(const ORDDEF * const *)e1;
    const ORDDEF *odp2 = *(const ORDDEF * const *)e2;
    int retval;
    int type1 = odp1->type;
    int type2 = odp2->type;

    if (type1 == TYPE_STUB) type1 = TYPE_CDECL;
    if (type2 == TYPE_STUB) type2 = TYPE_CDECL;

    if ((retval = type1 - type2) != 0) return retval;

    type1 = odp1->flags & (FLAG_RET16|FLAG_REGISTER);
    type2 = odp2->flags & (FLAG_RET16|FLAG_REGISTER);

    if ((retval = type1 - type2) != 0) return retval;

    return strcmp( odp1->u.func.arg_types, odp2->u.func.arg_types );
}


/*******************************************************************
 *         relay_type_compare
 *
 * Same as callfrom16_type_compare but ignores differences that don't affect the resulting relay function.
 */
static int relay_type_compare( const void *e1, const void *e2 )
{
    const ORDDEF *odp1 = *(const ORDDEF * const *)e1;
    const ORDDEF *odp2 = *(const ORDDEF * const *)e2;
    char name1[80];

    strcpy( name1, get_relay_name(odp1) );
    return strcmp( name1, get_relay_name(odp2) );
}


/*******************************************************************
 *         sort_func_list
 *
 * Sort a list of functions, removing duplicates.
 */
static int sort_func_list( ORDDEF **list, int count,
                           int (*compare)(const void *, const void *) )
{
    int i, j;

    qsort( list, count, sizeof(*list), compare );

    for (i = j = 0; i < count; i++)
    {
        if (compare( &list[j], &list[i] )) list[++j] = list[i];
    }
    return j + 1;
}


/*******************************************************************
 *         output_init_code
 *
 * Output the dll initialization code.
 */
static void output_init_code( FILE *outfile, const DLLSPEC *spec, const char *header_name )
{
    char name[80];

    sprintf( name, ".L__wine_spec_%s_init", make_c_identifier(spec->dll_name) );

    fprintf( outfile, "\n/* dll initialization code */\n\n" );
    fprintf( outfile, "\t.text\n" );
    fprintf( outfile, "\t.align 4\n" );
    fprintf( outfile, "\t%s\n", func_declaration(name) );
    fprintf( outfile, "%s:\n", name );
    if (UsePIC)
    {
        fprintf( outfile, "\tcall %s\n", asm_name("__wine_spec_get_pc_thunk_eax") );
        fprintf( outfile, "1:\tleal .L__wine_spec_file_name-1b(%%eax),%%ecx\n" );
        fprintf( outfile, "\tpushl %%ecx\n" );
        fprintf( outfile, "\tleal %s-1b(%%eax),%%ecx\n", header_name );
        fprintf( outfile, "\tpushl %%ecx\n" );
    }
    else
    {
        fprintf( outfile, "\tpushl $.L__wine_spec_file_name\n" );
        fprintf( outfile, "\tpushl $%s\n", header_name );
    }
    fprintf( outfile, "\tcall %s\n", asm_name("__wine_dll_register_16") );
    fprintf( outfile, "\taddl $8,%%esp\n" );
    fprintf( outfile, "\tret\n" );
    output_function_size( outfile, name );

    sprintf( name, ".L__wine_spec_%s_fini", make_c_identifier(spec->dll_name) );

    fprintf( outfile, "\t.align 4\n" );
    fprintf( outfile, "\t%s\n", func_declaration(name) );
    fprintf( outfile, "%s:\n", name );
    if (UsePIC)
    {
        fprintf( outfile, "\tcall %s\n", asm_name("__wine_spec_get_pc_thunk_eax") );
        fprintf( outfile, "1:\tleal %s-1b(%%eax),%%ecx\n", header_name );
        fprintf( outfile, "\tpushl %%ecx\n" );
    }
    else
    {
        fprintf( outfile, "\tpushl $%s\n", header_name );
    }
    fprintf( outfile, "\tcall %s\n", asm_name("__wine_dll_unregister_16") );
    fprintf( outfile, "\taddl $4,%%esp\n" );
    fprintf( outfile, "\tret\n" );
    output_function_size( outfile, name );

    if (target_platform == PLATFORM_APPLE)
    {
        fprintf( outfile, "\t.mod_init_func\n" );
        fprintf( outfile, "\t.align %d\n", get_alignment(4) );
        fprintf( outfile, "\t.long .L__wine_spec_%s_init\n", make_c_identifier(spec->dll_name) );
        fprintf( outfile, "\t.mod_term_func\n" );
        fprintf( outfile, "\t.align %d\n", get_alignment(4) );
        fprintf( outfile, "\t.long .L__wine_spec_%s_fini\n", make_c_identifier(spec->dll_name) );
    }
    else
    {
        fprintf( outfile, "\t.section \".init\",\"ax\"\n" );
        fprintf( outfile, "\tcall .L__wine_spec_%s_init\n", make_c_identifier(spec->dll_name) );
        fprintf( outfile, "\t.section \".fini\",\"ax\"\n" );
        fprintf( outfile, "\tcall .L__wine_spec_%s_fini\n", make_c_identifier(spec->dll_name) );
    }
}


/*******************************************************************
 *         BuildSpec16File
 *
 * Build a Win16 assembly file from a spec file.
 */
void BuildSpec16File( FILE *outfile, DLLSPEC *spec )
{
    ORDDEF **typelist;
    int i, j, nb_funcs;
    char header_name[256];

    /* File header */

    output_standard_file_header( outfile );

    if (!spec->dll_name)  /* set default name from file name */
    {
        char *p;
        spec->dll_name = xstrdup( spec->file_name );
        if ((p = strrchr( spec->dll_name, '.' ))) *p = 0;
    }

    /* Build sorted list of all argument types, without duplicates */

    typelist = xmalloc( (spec->limit + 1) * sizeof(*typelist) );

    for (i = nb_funcs = 0; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];
        if (!odp) continue;
        if (is_function( odp )) typelist[nb_funcs++] = odp;
    }

    nb_funcs = sort_func_list( typelist, nb_funcs, callfrom16_type_compare );

    /* Output the module structure */

    sprintf( header_name, "__wine_spec_%s_dos_header", make_c_identifier(spec->dll_name) );
    fprintf( outfile, "\n/* module data */\n\n" );
    fprintf( outfile, "\t.data\n" );
    fprintf( outfile, "\t.align %d\n", get_alignment(4) );
    fprintf( outfile, "%s:\n", header_name );
    fprintf( outfile, "\t%s 0x%04x\n", get_asm_short_keyword(),                      /* e_magic */
             IMAGE_DOS_SIGNATURE );
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );                         /* e_cblp */
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );                         /* e_cp */
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );                         /* e_crlc */
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );                         /* e_cparhdr */
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );                         /* e_minalloc */
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );                         /* e_maxalloc */
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );                         /* e_ss */
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );                         /* e_sp */
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );                         /* e_csum */
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );                         /* e_ip */
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );                         /* e_cs */
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );                         /* e_lfarlc */
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );                         /* e_ovno */
    fprintf( outfile, "\t%s 0,0,0,0\n", get_asm_short_keyword() );                   /* e_res */
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );                         /* e_oemid */
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );                         /* e_oeminfo */
    fprintf( outfile, "\t%s 0,0,0,0,0,0,0,0,0,0\n", get_asm_short_keyword() );       /* e_res2 */
    fprintf( outfile, "\t.long .L__wine_spec_ne_header-%s\n", header_name );         /* e_lfanew */

    fprintf( outfile, ".L__wine_spec_ne_header:\n" );
    fprintf( outfile, "\t%s 0x%04x\n", get_asm_short_keyword(),                      /* ne_magic */
             IMAGE_OS2_SIGNATURE );
    fprintf( outfile, "\t.byte 0\n" );                                               /* ne_ver */
    fprintf( outfile, "\t.byte 0\n" );                                               /* ne_rev */
    fprintf( outfile, "\t%s .L__wine_spec_ne_enttab-.L__wine_spec_ne_header\n",      /* ne_enttab */
             get_asm_short_keyword() );
    fprintf( outfile, "\t%s .L__wine_spec_ne_enttab_end-.L__wine_spec_ne_enttab\n",  /* ne_cbenttab */
             get_asm_short_keyword() );
    fprintf( outfile, "\t.long 0\n" );                                               /* ne_crc */
    fprintf( outfile, "\t%s 0x%04x\n", get_asm_short_keyword(),                      /* ne_flags */
             NE_FFLAGS_SINGLEDATA | NE_FFLAGS_LIBMODULE );
    fprintf( outfile, "\t%s 2\n", get_asm_short_keyword() );                         /* ne_autodata */
    fprintf( outfile, "\t%s %u\n", get_asm_short_keyword(), spec->heap_size );       /* ne_heap */
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );                         /* ne_stack */
    fprintf( outfile, "\t.long 0\n" );                                               /* ne_csip */
    fprintf( outfile, "\t.long 0\n" );                                               /* ne_sssp */
    fprintf( outfile, "\t%s 2\n", get_asm_short_keyword() );                         /* ne_cseg */
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );                         /* ne_cmod */
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );                         /* ne_cbnrestab */
    fprintf( outfile, "\t%s .L__wine_spec_ne_segtab-.L__wine_spec_ne_header\n",      /* ne_segtab */
             get_asm_short_keyword() );
    fprintf( outfile, "\t%s .L__wine_spec_ne_rsrctab-.L__wine_spec_ne_header\n",     /* ne_rsrctab */
             get_asm_short_keyword() );
    fprintf( outfile, "\t%s .L__wine_spec_ne_restab-.L__wine_spec_ne_header\n",      /* ne_restab */
             get_asm_short_keyword() );
    fprintf( outfile, "\t%s .L__wine_spec_ne_modtab-.L__wine_spec_ne_header\n",      /* ne_modtab */
             get_asm_short_keyword() );
    fprintf( outfile, "\t%s .L__wine_spec_ne_imptab-.L__wine_spec_ne_header\n",      /* ne_imptab */
             get_asm_short_keyword() );
    fprintf( outfile, "\t.long 0\n" );                                   /* ne_nrestab */
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );             /* ne_cmovent */
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );             /* ne_align */
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );             /* ne_cres */
    fprintf( outfile, "\t.byte 0x%02x\n", NE_OSFLAGS_WINDOWS );          /* ne_exetyp */
    fprintf( outfile, "\t.byte 0x%02x\n", NE_AFLAGS_FASTLOAD );          /* ne_flagsothers */
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );             /* ne_pretthunks */
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );             /* ne_psegrefbytes */
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );             /* ne_swaparea */
    fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );             /* ne_expver */

    /* segment table */

    fprintf( outfile, "\n.L__wine_spec_ne_segtab:\n" );

    /* code segment entry */

    fprintf( outfile, "\t%s .L__wine_spec_code_segment-%s\n",  /* filepos */
             get_asm_short_keyword(), header_name );
    fprintf( outfile, "\t%s .L__wine_spec_code_segment_end-.L__wine_spec_code_segment\n", /* size */
             get_asm_short_keyword() );
    fprintf( outfile, "\t%s 0x%04x\n", get_asm_short_keyword(), NE_SEGFLAGS_32BIT );      /* flags */
    fprintf( outfile, "\t%s .L__wine_spec_code_segment_end-.L__wine_spec_code_segment\n", /* minsize */
             get_asm_short_keyword() );

    /* data segment entry */

    fprintf( outfile, "\t%s .L__wine_spec_data_segment-%s\n",  /* filepos */
             get_asm_short_keyword(), header_name );
    fprintf( outfile, "\t%s .L__wine_spec_data_segment_end-.L__wine_spec_data_segment\n", /* size */
             get_asm_short_keyword() );
    fprintf( outfile, "\t%s 0x%04x\n", get_asm_short_keyword(), NE_SEGFLAGS_DATA );      /* flags */
    fprintf( outfile, "\t%s .L__wine_spec_data_segment_end-.L__wine_spec_data_segment\n", /* minsize */
             get_asm_short_keyword() );

    /* resource directory */

    output_res16_directory( outfile, spec, header_name );

    /* resident names table */

    fprintf( outfile, "\n\t.align %d\n", get_alignment(2) );
    fprintf( outfile, ".L__wine_spec_ne_restab:\n" );
    output_resident_name( outfile, spec->dll_name, 0 );
    for (i = 1; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];
        if (!odp || !odp->name[0]) continue;
        output_resident_name( outfile, odp->name, i );
    }
    fprintf( outfile, "\t.byte 0\n" );

    /* imported names table */

    fprintf( outfile, "\n\t.align %d\n", get_alignment(2) );
    fprintf( outfile, ".L__wine_spec_ne_modtab:\n" );
    fprintf( outfile, ".L__wine_spec_ne_imptab:\n" );
    fprintf( outfile, "\t.byte 0,0\n" );

    /* entry table */

    fprintf( outfile, "\n.L__wine_spec_ne_enttab:\n" );
    output_entry_table( outfile, spec );
    fprintf( outfile, ".L__wine_spec_ne_enttab_end:\n" );

    /* code segment */

    fprintf( outfile, "\n\t.align %d\n", get_alignment(2) );
    fprintf( outfile, ".L__wine_spec_code_segment:\n" );

    for ( i = 0; i < nb_funcs; i++ )
    {
        unsigned int arg_types[2];
        int j, nop_words, argsize = 0;

        if ( typelist[i]->type == TYPE_PASCAL )
            argsize = get_function_argsize( typelist[i] );

        /* build the arg types bit fields */
        arg_types[0] = arg_types[1] = 0;
        for (j = 0; typelist[i]->u.func.arg_types[j]; j++)
        {
            int type = 0;
            switch(typelist[i]->u.func.arg_types[j])
            {
            case 'w': type = ARG_WORD; break;
            case 's': type = ARG_SWORD; break;
            case 'l': type = ARG_LONG; break;
            case 'p': type = ARG_PTR; break;
            case 't': type = ARG_STR; break;
            case 'T': type = ARG_SEGSTR; break;
            }
            arg_types[j / 10] |= type << (3 * (j % 10));
        }
        if (typelist[i]->type == TYPE_VARARGS) arg_types[j / 10] |= ARG_VARARG << (3 * (j % 10));

        fprintf( outfile, ".L__wine_spec_callfrom16_%s:\n", get_callfrom16_name(typelist[i]) );
        fprintf( outfile, "\tpushl $.L__wine_spec_call16_%s\n", get_relay_name(typelist[i]) );
        fprintf( outfile, "\tlcall $0,$0\n" );

        if (typelist[i]->flags & FLAG_REGISTER)
        {
            nop_words = 4;
        }
        else if (typelist[i]->flags & FLAG_RET16)
        {
            fprintf( outfile, "\torw %%ax,%%ax\n" );
            fprintf( outfile, "\tnop\n" );  /* so that the lretw is aligned */
            nop_words = 2;
        }
        else
        {
            fprintf( outfile, "shld $16,%%eax,%%edx\n" );
            fprintf( outfile, "orl %%eax,%%eax\n" );
            nop_words = 1;
        }

        if (argsize)
        {
            fprintf( outfile, "lretw $%u\n", argsize );
            nop_words--;
        }
        else fprintf( outfile, "lretw\n" );

        if (nop_words) fprintf( outfile, "\t%s\n", nop_sequence[nop_words-1] );

        /* the movl is here so that the code contains only valid instructions, */
        /* it's never actually executed, we only care about the arg_types[] values */
        fprintf( outfile, "\t%s 0x86c7\n", get_asm_short_keyword() );
        fprintf( outfile, "\t.long 0x%08x,0x%08x\n", arg_types[0], arg_types[1] );
    }

    for (i = 0; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];
        if (!odp || !is_function( odp )) continue;
        fprintf( outfile, ".L__wine_%s_%u:\n", make_c_identifier(spec->dll_name), i );
        fprintf( outfile, "\tpushw %%bp\n" );
        fprintf( outfile, "\tpushl $%s\n",
                 asm_name( odp->type == TYPE_STUB ? get_stub_name( odp, spec ) : odp->link_name ));
        fprintf( outfile, "\tcallw .L__wine_spec_callfrom16_%s\n", get_callfrom16_name( odp ) );
    }
    fprintf( outfile, ".L__wine_spec_code_segment_end:\n" );

    /* data segment */

    fprintf( outfile, "\n.L__wine_spec_data_segment:\n" );
    fprintf( outfile, "\t.byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0\n" );  /* instance data */
    for (i = 0; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];
        if (!odp || odp->type != TYPE_VARIABLE) continue;
        fprintf( outfile, ".L__wine_%s_%u:\n", make_c_identifier(spec->dll_name), i );
        fprintf( outfile, "\t.long " );
        for (j = 0; j < odp->u.var.n_values-1; j++)
            fprintf( outfile, "0x%08x,", odp->u.var.values[j] );
        fprintf( outfile, "0x%08x\n", odp->u.var.values[j] );
    }
    fprintf( outfile, ".L__wine_spec_data_segment_end:\n" );

    /* resource data */

    if (spec->nb_resources)
    {
        fprintf( outfile, "\n.L__wine_spec_resource_data:\n" );
        output_res16_data( outfile, spec );
    }

    fprintf( outfile, "\t.byte 0\n" );  /* make sure the last symbol points to something */

    /* relay functions */

    nb_funcs = sort_func_list( typelist, nb_funcs, relay_type_compare );
    if (nb_funcs)
    {
        fprintf( outfile, "\n/* relay functions */\n\n" );
        fprintf( outfile, "\t.text\n" );
        for ( i = 0; i < nb_funcs; i++ ) output_call16_function( outfile, typelist[i] );
        fprintf( outfile, "\t.data\n" );
        fprintf( outfile, "wine_ldt_copy_ptr:\n" );
        fprintf( outfile, "\t.long %s\n", asm_name("wine_ldt_copy") );
    }

    fprintf( outfile, "\n\t%s\n", get_asm_string_section() );
    fprintf( outfile, ".L__wine_spec_file_name:\n" );
    fprintf( outfile, "\t%s \"%s\"\n", get_asm_string_keyword(), spec->file_name );

    output_stubs( outfile, spec );
    output_get_pc_thunk( outfile );
    output_init_code( outfile, spec, header_name );

    free( typelist );
}
