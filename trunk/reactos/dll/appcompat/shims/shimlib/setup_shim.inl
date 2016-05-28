/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Shim library
 * FILE:            dll/appcompat/shims/shimlib/setup_shim.inl
 * PURPOSE:         Shimlib helper file, used for setting up the macro's used when registering a shim.
 * PROGRAMMER:      Mark Jansen
 */


#ifndef SHIM_NS
#error "A namespace should be provided in SHIM_NS before including this file!"
#endif

#ifndef SHIM_OBJ_NAME
#define SHIM_OBJ_NAME3(ns, name) ns ## _ ## name
#define SHIM_OBJ_NAME2(ns, name) SHIM_OBJ_NAME3(ns, name)
#define SHIM_OBJ_NAME(name) SHIM_OBJ_NAME2(SHIM_NS, name)

#define SHIM_STRINGIFY2(X_) # X_
#define SHIM_STRINGIFY(X_) SHIM_STRINGIFY2(X_)

#define SHIM_HOOK(num, dll, function, target) \
    SHIM_OBJ_NAME(g_pAPIHooks)[num].LibraryName = dll; \
    SHIM_OBJ_NAME(g_pAPIHooks)[num].FunctionName = function; \
    SHIM_OBJ_NAME(g_pAPIHooks)[num].ReplacementFunction = target; \
    SHIM_OBJ_NAME(g_pAPIHooks)[num].OriginalFunction = NULL; \
    SHIM_OBJ_NAME(g_pAPIHooks)[num].Unk1 = NULL; \
    SHIM_OBJ_NAME(g_pAPIHooks)[num].Unk2 = NULL;

#define CALL_SHIM(SHIM_NUM, SHIM_CALLCONV) \
    ((SHIM_CALLCONV)(SHIM_OBJ_NAME(g_pAPIHooks)[SHIM_NUM].OriginalFunction))

#endif


PCSTR SHIM_OBJ_NAME(g_szModuleName) = SHIM_STRINGIFY(SHIM_NS);
PCSTR SHIM_OBJ_NAME(g_szCommandLine);
PHOOKAPI SHIM_OBJ_NAME(g_pAPIHooks);

