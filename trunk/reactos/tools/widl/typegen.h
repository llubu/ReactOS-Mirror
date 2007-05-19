/*
 * Format String Generator for IDL Compiler
 *
 * Copyright 2005-2006 Eric Kohl
 * Copyright 2005 Robert Shearman
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


enum pass
{
    PASS_IN,
    PASS_OUT,
    PASS_RETURN
};

enum remoting_phase
{
    PHASE_BUFFERSIZE,
    PHASE_MARSHAL,
    PHASE_UNMARSHAL,
    PHASE_FREE
};

void write_formatstringsdecl(FILE *f, int indent, ifref_list_t *ifaces, int for_objects);
void write_procformatstring(FILE *file, const ifref_list_t *ifaces, int for_objects);
void write_typeformatstring(FILE *file, const ifref_list_t *ifaces, int for_objects);
void print_phase_basetype(FILE *file, int indent, enum remoting_phase phase, enum pass pass, const var_t *var, const char *varname);
void write_remoting_arguments(FILE *file, int indent, const func_t *func, enum pass pass, enum remoting_phase phase);
size_t get_size_procformatstring_var(const var_t *var);
size_t get_size_procformatstring_func(const func_t *func);
size_t get_size_procformatstring(const ifref_list_t *ifaces, int for_objects);
size_t get_size_typeformatstring(const ifref_list_t *ifaces, int for_objects);
void assign_stub_out_args( FILE *file, int indent, const func_t *func );
void declare_stub_args( FILE *file, int indent, const func_t *func );
int write_expr_eval_routines(FILE *file, const char *iface);
void write_expr_eval_routine_list(FILE *file, const char *iface);
void write_endpoints( FILE *f, const char *prefix, const str_list_t *list );
