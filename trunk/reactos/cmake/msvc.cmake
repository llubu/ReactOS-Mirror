
if(${CMAKE_BUILD_TYPE} MATCHES Debug)
    # no optimitation
elseif(OPTIMIZE STREQUAL "1")
    add_definitions(/O1)
elseif(OPTIMIZE STREQUAL "2")
    add_definitions(/O2)
elseif(OPTIMIZE STREQUAL "3")
    add_definitions(/Ot /Ox /GS-)
elseif(OPTIMIZE STREQUAL "4")
    add_definitions(/Os /Ox /GS-)
elseif(OPTIMIZE STREQUAL "5")
    add_definitions(/GF /Gy /Ob2 /Os /Ox /GS-)
endif()

if(ARCH MATCHES i386)
    add_definitions(/DWIN32 /D_WINDOWS)
endif()

add_definitions(/Dinline=__inline /D__STDC__=1)

add_compiler_flags(/X /GR- /GS- /Zl /W3)

if(${_MACHINE_ARCH_FLAG} MATCHES X86)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO /NODEFAULTLIB")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO /NODEFAULTLIB")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO /NODEFAULTLIB")
endif()

if(${ARCH} MATCHES amd64)
    add_definitions(/D__x86_64)
    set(SPEC2DEF_ARCH x86_64)
else()
    set(SPEC2DEF_ARCH i386)
endif()

link_directories(${REACTOS_SOURCE_DIR}/importlibs ${REACTOS_BINARY_DIR}/importlibs ${REACTOS_BINARY_DIR}/lib/sdk/crt)

set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <DEFINES> /I${REACTOS_SOURCE_DIR}/include/psdk /I${REACTOS_BINARY_DIR}/include/psdk /I${REACTOS_SOURCE_DIR}/include /I${REACTOS_SOURCE_DIR}/include/reactos /I${REACTOS_BINARY_DIR}/include/reactos /I${REACTOS_SOURCE_DIR}/include/reactos/wine /I${REACTOS_SOURCE_DIR}/include/crt /I${REACTOS_SOURCE_DIR}/include/crt/mingw32 /fo <OBJECT> <SOURCE>")

if(MSVC_IDE)
    # Asm source files are not supported in VS generators yet. As a result, <DEFINES> isn't recognized.
    # We may temporarily use just the global defines, but this is not a solution as some modules (minihal for example) apply additional definitions to source files, so we get an incorrect build of such targets.
    get_directory_property(definitions DEFINITIONS)
    set(CMAKE_ASM_COMPILE_OBJECT
        "<CMAKE_C_COMPILER> /nologo /X /I${REACTOS_SOURCE_DIR}/include/asm /I${REACTOS_BINARY_DIR}/include/asm <FLAGS> ${definitions} /D__ASM__ /D_USE_ML /EP /c <SOURCE> > <OBJECT>.tmp"
        "<CMAKE_ASM_COMPILER> /nologo /Cp /Fo<OBJECT> /c /Ta <OBJECT>.tmp")
else()
    # NMake Makefiles
    set(CMAKE_ASM_COMPILE_OBJECT
        "<CMAKE_C_COMPILER> /nologo /X /I${REACTOS_SOURCE_DIR}/include/asm /I${REACTOS_BINARY_DIR}/include/asm <FLAGS> <DEFINES> /D__ASM__ /D_USE_ML /EP /c <SOURCE> > <OBJECT>.tmp"
        "<CMAKE_ASM_COMPILER> /nologo /Cp /Fo<OBJECT> /c /Ta <OBJECT>.tmp")
endif()

set(CMAKE_RC_CREATE_SHARED_LIBRARY ${CMAKE_C_CREATE_SHARED_LIBRARY})
set(CMAKE_ASM_CREATE_SHARED_LIBRARY ${CMAKE_C_CREATE_SHARED_LIBRARY})
set(CMAKE_ASM_CREATE_STATIC_LIBRARY ${CMAKE_C_CREATE_STATIC_LIBRARY})

macro(add_pch _target_name _FILE)
endmacro()

function(set_entrypoint MODULE ENTRYPOINT)
    if(${ENTRYPOINT} STREQUAL "0")
        add_linkerflag(${MODULE} "/NOENTRY")
    else()
        add_linkerflag(${MODULE} "/ENTRY:${ENTRYPOINT}")
    endif()
endfunction()

function(set_subsystem MODULE SUBSYSTEM)
    add_linkerflag(${MODULE} "/subsystem:${SUBSYSTEM}")
endfunction()

function(set_image_base MODULE IMAGE_BASE)
    add_linkerflag(${MODULE} "/BASE:${IMAGE_BASE}")
endfunction()

function(set_module_type MODULE TYPE)
    add_dependencies(${MODULE} psdk)
    if(${TYPE} MATCHES nativecui)
        set_subsystem(${MODULE} native)
        set_entrypoint(${MODULE} NtProcessStartup@4)
    elseif (${TYPE} MATCHES win32gui)
        set_subsystem(${MODULE} windows)
        if(IS_UNICODE)
            set_entrypoint(${MODULE} wWinMainCRTStartup)
        else()
            set_entrypoint(${MODULE} WinMainCRTStartup)
        endif(IS_UNICODE)
    elseif (${TYPE} MATCHES win32cui)
        set_subsystem(${MODULE} console)
        if(IS_UNICODE)
            set_entrypoint(${MODULE} wmainCRTStartup)
        else()
            set_entrypoint(${MODULE} mainCRTStartup)
        endif(IS_UNICODE)
    elseif(${TYPE} MATCHES win32dll)
        # Need this only because mingw library is broken
        set_entrypoint(${MODULE} DllMainCRTStartup@12)
        if(DEFINED baseaddress_${MODULE})
            set_image_base(${MODULE} ${baseaddress_${MODULE}})
        else()
            message(STATUS "${MODULE} has no base address")
        endif()
        add_linkerflag(${MODULE} "/DLL")
    elseif(${TYPE} MATCHES win32ocx)
        set_entrypoint(${MODULE} DllMainCRTStartup@12)
        set_target_properties(${MODULE} PROPERTIES SUFFIX ".ocx")
        add_linkerflag(${MODULE} "/DLL")
    elseif(${TYPE} MATCHES cpl)
        set_entrypoint(${MODULE} DllMainCRTStartup@12)
        set_target_properties(${MODULE} PROPERTIES SUFFIX ".cpl")
        add_linkerflag(${MODULE} "/DLL")
    elseif(${TYPE} MATCHES kernelmodedriver)
        set_target_properties(${MODULE} PROPERTIES SUFFIX ".sys")
        set_entrypoint(${MODULE} DriverEntry@8)
        set_subsystem(${MODULE} native)
        set_image_base(${MODULE} 0x00010000)
        add_linkerflag(${MODULE} "/DRIVER")
        add_dependencies(${MODULE} bugcodes)
    elseif(${TYPE} MATCHES nativedll)
        set_subsystem(${MODULE} native)
    else()
        message(FATAL_ERROR "Unknown module type : ${TYPE}")
    endif()
endfunction()

function(set_rc_compiler)
# dummy, this workaround is only needed in mingw due to lack of RC support in cmake
endfunction()

# Thanks MS for creating a stupid linker
function(add_importlib_target _exports_file)
    get_filename_component(_name ${_exports_file} NAME_WE)
    get_target_property(_suffix ${_name} SUFFIX)
    if(${_suffix} STREQUAL "_suffix-NOTFOUND")
        get_target_property(_type ${_name} TYPE)
        if(${_type} MATCHES EXECUTABLE)
            set(_suffix ".exe")
        else()
            set(_suffix ".dll")
        endif()
    endif()

    # Generate the asm stub file and the export def file
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_stubs.asm ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_exp.def
        COMMAND native-spec2def --ms --kill-at -a=${SPEC2DEF_ARCH} --implib -n=${_name}${_suffix} -d=${CMAKE_BINARY_DIR}/importlibs/lib${_name}_exp.def -l=${CMAKE_BINARY_DIR}/importlibs/lib${_name}_stubs.asm ${CMAKE_CURRENT_SOURCE_DIR}/${_exports_file}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_exports_file} native-spec2def)

    # Assemble the stub file
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_stubs.obj
        COMMAND ${CMAKE_ASM_COMPILER} /nologo /Cp /Fo${CMAKE_BINARY_DIR}/importlibs/lib${_name}_stubs.obj /c /Ta ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_stubs.asm
        DEPENDS "${CMAKE_BINARY_DIR}/importlibs/lib${_name}_stubs.asm")

    # Add neccessary importlibs for redirections
    set(_libraries "")
    set(_dependencies "")
    foreach(_lib ${ARGN})
        list(APPEND _libraries "${CMAKE_BINARY_DIR}/importlibs/${_lib}.lib")
        list(APPEND _dependencies ${_lib})
    endforeach()

    # Build the importlib
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/importlibs/lib${_name}.lib
        COMMAND LINK /LIB /NOLOGO /DEF:${CMAKE_BINARY_DIR}/importlibs/lib${_name}_exp.def /OUT:${CMAKE_BINARY_DIR}/importlibs/lib${_name}.lib ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_stubs.obj ${_libraries}
        DEPENDS ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_stubs.obj ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_exp.def)

    # Add the importlib target
    add_custom_target(
        lib${_name}
        DEPENDS ${CMAKE_BINARY_DIR}/importlibs/lib${_name}.lib)

    add_dependencies(lib${_name} asm ${_dependencies})
endfunction()

macro(add_delay_importlibs MODULE)
    # TODO. For now forward to normal import libs
    add_importlibs(${MODULE} ${ARGN})
endmacro()

function(spec2def _dllname _spec_file)
    get_filename_component(_file ${_spec_file} NAME_WE)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_file}.def ${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c
        COMMAND native-spec2def --ms --kill-at -a=${SPEC2DEF_ARCH} -n=${_dllname} -d=${CMAKE_CURRENT_BINARY_DIR}/${_file}.def -s=${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file} native-spec2def)
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${_file}.def ${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c
        PROPERTIES GENERATED TRUE)
endfunction()

macro(macro_mc FILE)
    set(COMMAND_MC mc -r ${REACTOS_BINARY_DIR}/include/reactos -h ${REACTOS_BINARY_DIR}/include/reactos ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.mc)
endmacro()

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/importlibs)

#pseh workaround
set(PSEH_LIB "pseh")

function(CreateBootSectorTarget2 _target_name _asm_file _binary_file _base_address)

    set(_object_file ${_binary_file}.obj)
    set(_temp_file ${_binary_file}.tmp)

    add_custom_command(
        OUTPUT ${_temp_file}
        COMMAND ${CMAKE_C_COMPILER} /nologo /X /I${REACTOS_SOURCE_DIR}/include/asm /I${REACTOS_BINARY_DIR}/include/asm /D__ASM__ /D_USE_ML /EP /c ${_asm_file} > ${_temp_file}
        DEPENDS ${_asm_file})

    add_custom_command(
        OUTPUT ${_object_file}
        COMMAND ml /nologo /Cp /Fo${_object_file} /c /Ta ${_temp_file}
        DEPENDS ${_temp_file})

    add_custom_command(
        OUTPUT ${_binary_file}
        COMMAND native-obj2bin ${_object_file} ${_binary_file} ${_base_address}
        DEPENDS ${_object_file} native-obj2bin)

    set_source_files_properties(${_object_file} ${_temp_file} ${_binary_file} PROPERTIES GENERATED TRUE)

    add_custom_target(${_target_name} ALL DEPENDS ${_binary_file})
endfunction()
