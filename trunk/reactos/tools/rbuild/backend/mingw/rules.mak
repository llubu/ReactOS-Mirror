RBUILD_fullpath=${subst <<<,,${subst $(SEP)<<<,,${subst /,$(SEP),${subst \\,$(SEP),$(1)}}<<<}}

RBUILD_compress_prefix=${subst >>>,,${subst >>>$($(2)),$$($(2)),>>>$(1)}}
RBUILD_compress_prefixes=${call RBUILD_compress_prefix,${call RBUILD_compress_prefix,${call RBUILD_compress_prefix,${call RBUILD_compress_prefix,${call RBUILD_compress_prefix,$(1),INTERMEDIATE},OUTPUT},CDOUTPUT},TEMPORARY},INSTALL}

RBUILD_strip_prefix=${subst >>>,,${subst >>>$($(2)),,>>>$(1)}}
RBUILD_strip_prefixes=${subst >>>,,${subst >>>$(SEP),,>>>${call RBUILD_strip_prefix,${call RBUILD_strip_prefix,${call RBUILD_strip_prefix,${call RBUILD_strip_prefix,${call RBUILD_strip_prefix,$(1),INTERMEDIATE},OUTPUT},CDOUTPUT},TEMPORARY},INSTALL}}}

#(source)
RBUILD_intermediate_path_noext=${call RBUILD_intermediate_dir,$(1)}$(SEP)$(basename $(notdir $(1)))
RBUILD_output_path_noext=${call RBUILD_output_dir,$(1)}$(SEP)$(basename $(notdir $(1)))

#(module, source)
RBUILD_intermediate_path_unique=${call RBUILD_intermediate_path_noext,$(2)}_$(1)
RBUILD_output_dir=${call RBUILD_output_path_noext,$(2)}_$(1)

#(source)
RBUILD_intermediate_dir=${call RBUILD_fullpath,$(value INTERMEDIATE)$(SEP)$(dir ${call RBUILD_strip_prefixes,$(1)})}
RBUILD_output_dir=${call RBUILD_fullpath,$(value OUTPUT)$(SEP)$(dir ${call RBUILD_strip_prefixes,$(1)})}

#(source)
RBUILD_name=$(basename $(notdir $(1)))
RBUILD_noext=$(dir $(1))$(basename $(notdir $(1)))

#(source)
RBUILD_dir=${call RBUILD_fullpath,$(dir ${call RBUILD_compress_prefixes,$(1)})}

#(module, flags, includes, compiler, prefix)
RBUILD_compiler_flags=\
$$(BUILTIN_$(5)$(4)FLAGS) \
$$(PROJECT_$(5)$(4)FLAGS) \
$$(MODULETYPE$($(1)_TYPE)_$(4)FLAGS) \
$$($(1)_$(4)FLAGS) \
$(2) \
$$($(5)$(4)FLAGS)

#(module, flags, includes, compiler, prefix)
RBUILD_compiler_flags_with_cpp=\
$$($(5)$(4)INCLUDES) $$(CPPINCLUDES) \
$(3) \
$$($(1)_$(4)INCLUDES) $$($(1)_CPPINCLUDES) \
$$(MODULETYPE$($(1)_TYPE)_$(4)INCLUDES) $$(MODULETYPE$($(1)_TYPE)_CPPINCLUDES) \
$$(PROJECT_$(5)$(4)INCLUDES) $$(PROJECT_$(5)CPPINCLUDES) \
$$(BUILTIN_$(5)$(4)INCLUDES) $$(BUILTIN_$(5)CPPINCLUDES) \
$$(BUILTIN_$(5)CPPDEFINES) $$(BUILTIN_$(5)CPPFLAGS) $$(BUILTIN_$(5)$(4)DEFINES) $$(BUILTIN_$(5)$(4)FLAGS) \
$$(PROJECT_$(5)CPPDEFINES) $$(PROJECT_$(5)CPPFLAGS) $$(PROJECT_$(5)$(4)DEFINES) $$(PROJECT_$(5)$(4)FLAGS) \
$$(MODULETYPE$($(1)_TYPE)_CPPDEFINES) $$(MODULETYPE$($(1)_TYPE)_CPPFLAGS) $$(MODULETYPE$($(1)_TYPE)_$(4)DEFINES) $$(MODULETYPE$($(1)_TYPE)_$(4)FLAGS) \
$$($(1)_CPPDEFINES) $$($(1)_CPPFLAGS) $$($(1)_$(4)DEFINES) $$($(1)_$(4)FLAGS) \
$(2) \
$$(CPPDEFINES) $$(CPPFLAGS) $$($(5)$(4)DEFINES) $$($(5)$(4)FLAGS)

#(module, flags, includes, compiler, prefix)
RBUILD_compiler_flags_builtin_cpp=\
$$($(5)$(4)INCLUDES) $$(CPPINCLUDES) \
$(3) \
$$($(1)_$(4)INCLUDES) $$($(1)_CPPINCLUDES) \
$$(MODULETYPE$($(1)_TYPE)_$(4)INCLUDES) $$(MODULETYPE$($(1)_TYPE)_CPPINCLUDES) \
$$(PROJECT_$(5)$(4)INCLUDES) $$(PROJECT_$(5)CPPINCLUDES) \
$$(BUILTIN_$(5)$(4)INCLUDES) $$(BUILTIN_$(5)CPPINCLUDES) \
$$(BUILTIN_$(5)CPPDEFINES) $$(BUILTIN_$(5)$(4)DEFINES) $$(BUILTIN_$(5)$(4)FLAGS) \
$$(PROJECT_$(5)CPPDEFINES) $$(PROJECT_$(5)$(4)DEFINES) $$(PROJECT_$(5)$(4)FLAGS) \
$$(MODULETYPE$($(1)_TYPE)_CPPDEFINES) $$(MODULETYPE$($(1)_TYPE)_$(4)DEFINES) $$(MODULETYPE$($(1)_TYPE)_$(4)FLAGS) \
$$($(1)_CPPDEFINES) $$($(1)_$(4)DEFINES) $$($(1)_$(4)FLAGS) \
$(2) \
$$(CPPDEFINES) $$($(5)$(4)DEFINES) $$($(5)$(4)FLAGS)

#(module, flags, includes, compiler, prefix)
RBUILD_compiler_flags_with_includes=\
$$($(5)$(4)INCLUDES) \
$(3) \
$$($(1)_$(4)INCLUDES) $$($(1)_CPPINCLUDES) \
$$(MODULETYPE$($(1)_TYPE)_$(4)INCLUDES) \
$$(PROJECT_$(5)$(4)INCLUDES) \
$$(BUILTIN_$(5)$(4)INCLUDES) \
$$(BUILTIN_$(5)$(4)FLAGS) \
$$(PROJECT_$(5)$(4)FLAGS) \
$$(MODULETYPE$($(1)_TYPE)_$(4)FLAGS) \
$$($(1)_$(4)FLAGS) \
$(2) \
$$($(5)$(4)FLAGS)

#(module, flags, includes, compiler, prefix)
RBUILD_compiler_flags_cpp=\
$$($(5)$(4)INCLUDES) $$(CPPINCLUDES) \
$(3) \
$$($(1)_$(4)INCLUDES) $$($(1)_CPPINCLUDES) \
$$(MODULETYPE$($(1)_TYPE)_$(4)INCLUDES) $$(MODULETYPE$($(1)_TYPE)_CPPINCLUDES) \
$$(PROJECT_$(5)$(4)INCLUDES) $$(PROJECT_$(5)CPPINCLUDES) \
$$(BUILTIN_$(5)$(4)INCLUDES) $$(BUILTIN_$(5)CPPINCLUDES) \
$$(BUILTIN_$(5)CPPFLAGS) $$(BUILTIN_$(5)CPPDEFINES) $$(BUILTIN_$(5)$(4)DEFINES) \
$$(PROJECT_$(5)CPPFLAGS) $$(PROJECT_$(5)CPPDEFINES) $$(PROJECT_$(5)$(4)DEFINES) \
$$(MODULETYPE$($(1)_TYPE)_CPPFLAGS) $$(MODULETYPE$($(1)_TYPE)_CPPDEFINES) $$(MODULETYPE$($(1)_TYPE)_$(4)DEFINES) \
$$($(1)_CPPFLAGS) $$($(1)_CPPDEFINES) $$($(1)_$(4)DEFINES) \
$(2) \
$$(CPPFLAGS) $$(CPPDEFINES) $$($(5)$(4)DEFINES)

#(module, flags, includes)
RBUILD_cflags=${call RBUILD_compiler_flags_with_cpp,$(1),$(2),$(3),C}
RBUILD_cxxflags=${call RBUILD_compiler_flags_with_cpp,$(1),$(2),$(3),CXX}
#~ RBUILD_asflags=${call RBUILD_compiler_flags_with_cpp,$(1),$(2),$(3),AS}
RBUILD_asflags=${call RBUILD_compiler_flags_builtin_cpp,$(1),$(2),$(3),AS}
RBUILD_nasmflags=${call RBUILD_compiler_flags_builtin_cpp,$(1),$(2),$(3),NASM}
RBUILD_rc_pp_flags=${call RBUILD_compiler_flags_cpp,$(1),-DRC_INVOKED=1 -D__WIN32__=1 -D__FLAT__=1,$(3) -I.,RC}
RBUILD_rc_flags=${call RBUILD_compiler_flags_with_includes,$(1),$(2),$(3),RC}
RBUILD_spec_pp_flags=${call RBUILD_compiler_flags_cpp,$(1),,$(3),SPEC}
RBUILD_spec_flags=${call RBUILD_compiler_flags,$(1),$(2),,SPEC}
RBUILD_midlflags=${call RBUILD_compiler_flags_builtin_cpp,$(1),$(2),$(3),MIDL}
RBUILD_host_cflags=${call RBUILD_compiler_flags_with_cpp,$(1),$(2),$(3),C,HOST_}
RBUILD_host_cxxflags=${call RBUILD_compiler_flags_with_cpp,$(1),$(2),$(3),CXX,HOST_}
RBUILD_ldflags=${call RBUILD_compiler_flags,$(1),$(2),,LD}

RCFLAG_UNICODE:=-DUNICODE -D_UNICODE

BUILTIN_ASDEFINES+= -D__ASM__
BUILTIN_RCFLAGS+= --nostdinc
BUILTIN_RCDEFINES+= -DRC_INVOKED
BUILTIN_NASMFLAGS+= -f win32

#(module, source, dependencies, cflags, output)
define RBUILD_GAS

$(2): $${$(1)_precondition}

ifeq ($(ROS_BUILDDEPS),full)

$(5).d: $(2) | ${call RBUILD_dir,$(5)}
	$$(ECHO_DEPENDS)
	$${gas} -MF $$@ ${call RBUILD_asflags,$(1),$(4)} -M -MP -MT $$@ $$<

-include $(5).d

$(5): $(2) $(5).d $(3) | ${call RBUILD_dir,$(5)}
	$$(ECHO_AS)
	$${gas} -o $$@ ${call RBUILD_asflags,$(1),$(4)} -c $$<

else

$(5): $(2) $(3) | ${call RBUILD_dir,$(5)}
	$$(ECHO_AS)
	$${gas} -o $$@ ${call RBUILD_asflags,$(1),$(4)} -c $$<

endif

endef

#(module, source, dependencies, cflags)
RBUILD_GAS_RULE=${call RBUILD_GAS,$(1),$(2),$(3),$(4),${call RBUILD_intermediate_path_unique,$(1),$(2)}.o}

#(module, source, dependencies, cflags, output)
define RBUILD_NASM

$(2): $${$(1)_precondition}

$(5): $(2) $(3) | ${call RBUILD_dir,$(5)}
	$$(ECHO_NASM)
	$${nasm} -o $$@ ${call RBUILD_nasmflags,$(1),$(4)} $$<

endef

# TODO: module_dllname -> ${call RBUILD_module_dllname,$(1)}

#(module, source, dependencies, cflags, module_dllname, output)
define RBUILD_WINEBUILD_DEF

$(6): $(2) $$(winebuild_TARGET) | ${call RBUILD_intermediate_dir,$(6)}
	$$(ECHO_WINEBLD)
	$$(Q)$$(winebuild_TARGET) $$(WINEBUILD_FLAGS) -o $$@ --def -E $$< --filename $(5) ${call RBUILD_spec_flags,$(1),$(4)}

endef

#(module, source, dependencies, cflags, module_dllname, output)
define RBUILD_WINEBUILD_STUBS

$(6): $(2) $$(winebuild_TARGET) | ${call RBUILD_intermediate_dir,$(6)}
	$$(ECHO_WINEBLD)
	$$(Q)$$(winebuild_TARGET) $$(WINEBUILD_FLAGS) -o $$@ --pedll $$< --filename $(5) ${call RBUILD_spec_flags,$(1),$(4)}

endef

#(module, source, dependencies, cflags, module_dllname)
define RBUILD_WINEBUILD_WITH_CPP_RULE

ifeq ($$(ROS_BUILDDEPS),full)

${call RBUILD_DEPENDS,$(1),$(2),,${call RBUILD_spec_pp_flags,$(1),$(4)},${call RBUILD_intermediate_path_unique,$(1),$(2)}.spec.d}
-include ${call RBUILD_intermediate_path_unique,$(1),$(2)}.spec.d

${call RBUILD_CPP,$(1),$(2),${call RBUILD_intermediate_path_unique,$(1),$(2)}.spec.d $(3),${call RBUILD_spec_pp_flags,$(1),$(4)},${call RBUILD_intermediate_path_unique,$(1),$(2)}.spec}

else

${call RBUILD_CPP,$(1),$(2),$(3),${call RBUILD_spec_pp_flags,$(1),$(4)},${call RBUILD_intermediate_path_unique,$(1),$(2)}.spec}

endif

${call RBUILD_WINEBUILD_DEF,$(1),${call RBUILD_intermediate_path_unique,$(1),$(2)}.spec,,$(4),$(5),${call RBUILD_intermediate_path_unique,$(1),$(2)}.auto.def}
${call RBUILD_WINEBUILD_STUBS,$(1),${call RBUILD_intermediate_path_unique,$(1),$(2)}.spec,,$(4),$(5),${call RBUILD_intermediate_path_unique,$(1),$(2)}.stubs.c}
${call RBUILD_CC,$(1),${call RBUILD_intermediate_path_unique,$(1),$(2)}.stubs.c,,,${call RBUILD_intermediate_path_unique,$(1),$(2)}.stubs.o}

endef

#(module, source, dependencies, cflags, module_dllname)
define RBUILD_WINEBUILD_RULE

${call RBUILD_WINEBUILD_DEF,$(1),$(2),$(3),$(4),$(5),${call RBUILD_intermediate_path_unique,$(1),$(2)}.auto.def}
${call RBUILD_WINEBUILD_STUBS,$(1),$(2),$(3),$(4),$(5),${call RBUILD_intermediate_path_unique,$(1),$(2)}.stubs.c}
${call RBUILD_CC,$(1),${call RBUILD_intermediate_path_unique,$(1),$(2)}.stubs.c,,,${call RBUILD_intermediate_path_unique,$(1),$(2)}.stubs.o}

endef

# FIXME: wrc butchers localized strings and doesn't implement -M, so we have to use an external preprocessor
#(module, source, dependencies, cflags)
define RBUILD_WRC_RULE

$(2): $${$(1)_precondition}

ifeq ($$(ROS_BUILDDEPS),full)

${call RBUILD_DEPENDS,$(1),$(2),,${call RBUILD_rc_pp_flags,$(1),$(4)},${call RBUILD_intermediate_path_unique,$(1),$(2)}.res.d}
-include ${call RBUILD_intermediate_path_unique,$(1),$(2)}.coff.d

${call RBUILD_intermediate_path_unique,$(1),$(2)}.res: $(2) ${call RBUILD_intermediate_path_unique,$(1),$(2)}.res.d $(3) $$(wrc_TARGET) | ${call RBUILD_intermediate_dir,$(2)}
	$$(ECHO_RC)
	${call RBUILD_PIPE_CPP,$$<,${call RBUILD_rc_pp_flags,$(1),$(4)}} | $$(wrc_TARGET) -o $$@ ${call RBUILD_rc_flags,$(1),$(4),-I${call RBUILD_dir,$(2)}}

else

${call RBUILD_intermediate_path_unique,$(1),$(2)}.res: $(2) $(3) $$(wrc_TARGET) | ${call RBUILD_intermediate_dir,$(2)}
	$$(ECHO_RC)
	${call RBUILD_PIPE_CPP,$$<,${call RBUILD_rc_pp_flags,$(1),$(4)}} | $$(wrc_TARGET) -o $$@ ${call RBUILD_rc_flags,$(1),$(4),-I${call RBUILD_dir,$(2)}}

endif

${call RBUILD_intermediate_path_unique,$(1),$(2)}.coff: ${call RBUILD_intermediate_path_unique,$(1),$(2)}.res | ${call RBUILD_intermediate_dir,$(2)}
	$$(ECHO_CVTRES)
	$${windres} -i $$< -o $$@ -J res -O coff

endef

define RBUILD_WIDL

endef

define RBUILD_WIDL_HEADER_RULE

$(2): $${$(1)_precondition}

${call RBUILD_intermediate_path_noext,$(2)}.h: $(2) $(3) $$(widl_TARGET) | ${call RBUILD_intermediate_dir,$(2)}
	$$(ECHO_WIDL)
	$$(Q)$$(widl_TARGET) ${call RBUILD_midlflags,$(1),$(4),-I${call RBUILD_dir,$(2)}} -h -H $$@ $$<

endef

#(module, source, dependencies, cflags)
define RBUILD_WIDL_CLIENT_RULE

$(2): $${$(1)_precondition}

${call RBUILD_intermediate_path_noext,$(2)}_c.c ${call RBUILD_intermediate_path_noext,$(2)}_c.h: ${call RBUILD_intermediate_path_noext,$(2)}_c

${call RBUILD_intermediate_path_noext,$(2)}_c: $(2) $(3) $$(widl_TARGET) | ${call RBUILD_intermediate_dir,$(2)}
	$$(ECHO_WIDL)
	$$(Q)$$(widl_TARGET) ${call RBUILD_midlflags,$(1),$(4),-I${call RBUILD_dir,$(2)}} -h -H ${call RBUILD_intermediate_path_noext,$(2)}_c.h -c -C ${call RBUILD_intermediate_path_noext,$(2)}_c.c $(2)
	$${checkpoint} $$@>$(NUL)

${call RBUILD_CC,$(1),${call RBUILD_intermediate_path_noext,$(2)}_c.c,,-fno-unit-at-a-time,${call RBUILD_intermediate_path_noext,$(2)}_c.o}

endef

#(module, source, dependencies, cflags)
define RBUILD_WIDL_SERVER_RULE

$(2): $${$(1)_precondition}

${call RBUILD_intermediate_path_noext,$(2)}_s.c ${call RBUILD_intermediate_path_noext,$(2)}_s.h: ${call RBUILD_intermediate_path_noext,$(2)}_s

${call RBUILD_intermediate_path_noext,$(2)}_s: $(2) $(3) $$(widl_TARGET) | ${call RBUILD_intermediate_dir,$(2)}
	$$(ECHO_WIDL)
	$$(Q)$$(widl_TARGET) ${call RBUILD_midlflags,$(1),$(4),-I${call RBUILD_dir,$(2)}} -h -H ${call RBUILD_intermediate_path_noext,$(2)}_s.h -s -S ${call RBUILD_intermediate_path_noext,$(2)}_s.c $(2)
	$${checkpoint} $$@>$(NUL)

${call RBUILD_CC,$(1),${call RBUILD_intermediate_path_noext,$(2)}_s.c,,-fno-unit-at-a-time,${call RBUILD_intermediate_path_noext,$(2)}_s.o}

endef

#(module, source, dependencies, cflags)
define RBUILD_WIDL_PROXY_RULE

$(2): $${$(1)_precondition}

${call RBUILD_intermediate_path_noext,$(2)}_p.c ${call RBUILD_intermediate_path_noext,$(2)}_p.h: ${call RBUILD_intermediate_path_noext,$(2)}_p

${call RBUILD_intermediate_path_noext,$(2)}_p: $(2) $(3) $$(widl_TARGET) | ${call RBUILD_intermediate_dir,$(2)}
	$$(ECHO_WIDL)
	$$(Q)$$(widl_TARGET) ${call RBUILD_midlflags,$(1),$(4),-I${call RBUILD_dir,$(2)}} -h -H ${call RBUILD_intermediate_path_noext,$(2)}_p.h -p -P ${call RBUILD_intermediate_path_noext,$(2)}_p.c $(2)
	$${checkpoint} $$@>$(NUL)

${call RBUILD_CC,$(1),${call RBUILD_intermediate_path_noext,$(2)}_p.c,,-fno-unit-at-a-time,${call RBUILD_intermediate_path_noext,$(2)}_p.o}

endef

#(module, source, dependencies, cflags)
define RBUILD_WIDL_INTERFACE_RULE

$(2): $${$(1)_precondition}

${call RBUILD_intermediate_path_noext,$(2)}_i.c: $(2) $(3) $$(widl_TARGET) | ${call RBUILD_intermediate_dir,$(2)}
	$$(ECHO_WIDL)
	$$(Q)$$(widl_TARGET) ${call RBUILD_midlflags,$(1),$(4),-I${call RBUILD_dir,$(2)}} -u -U $$@ $$<

${call RBUILD_CC,$(1),${call RBUILD_intermediate_path_noext,$(2)}_i.c,,-fno-unit-at-a-time,${call RBUILD_intermediate_path_noext,$(2)}_i.o}

endef

# FIXME: this rule sucks
#(module, source, dependencies, cflags, bare_dependencies)
define RBUILD_WIDL_DLLDATA_RULE

$(2): $(3) ${$(1)_precondition} $$(widl_TARGET) | ${call RBUILD_intermediate_dir,$(2)}
	$$(ECHO_WIDL)
	$$(Q)$$(widl_TARGET) ${call RBUILD_midlflags,$(1),$(4)} --dlldata-only --dlldata=$(2) $(5)

${call RBUILD_CC,$(1),$(2),,,${call RBUILD_intermediate_path_noext,$(2)}.o}

endef

#(module, source, dependencies, cflags)
define RBUILD_WIDL_TLB_RULE

$(2): $${$(1)_precondition}

${call RBUILD_intermediate_dir,$(2)}$$(SEP)$(1).tlb: $(2) $(3) $$(widl_TARGET) | ${call RBUILD_intermediate_dir,$(2)}
	$$(ECHO_WIDL)
	$$(Q)$$(widl_TARGET) ${call RBUILD_midlflags,$(1),$(4),-I${call RBUILD_dir,$(2)}} -t -T $$@ $$<

endef

#(module, source, dependencies, cflags)
define RBUILD_HOST_GCC_RULE

$(2): $${$(1)_precondition}

${call RBUILD_intermediate_path_unique,$(1),$(2)}.o: $(2) $(3) | ${call RBUILD_intermediate_dir,$(2)}
	$$(ECHO_HOSTCC)
	$${host_gcc} -o $$@ ${call RBUILD_host_cflags,$(1),$(4)} -c $$<

endef

#(module, source, dependencies, cflags)
define RBUILD_HOST_GPP_RULE

$(2): $${$(1)_precondition}

${call RBUILD_intermediate_path_unique,$(1),$(2)}.o: $(2) $(3) | ${call RBUILD_intermediate_dir,$(2)}
	$$(ECHO_HOSTCC)
	$${host_gpp} -o $$@ ${call RBUILD_host_cxxflags,$(1),$(4)} -c $$<

endef

# EOF
