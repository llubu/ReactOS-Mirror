# Automatic dependancy tracking
# Define $DEP_OBJECTS before this file is included
# $DEP_OBJECTS contain a list of object files that are checked for dependancies

DEP_FILTERED := $(filter-out ex/napi.% nt/zw.%, $(DEP_OBJECTS:.o=.d))
DEP_FILES := $(join $(dir $(DEP_FILTERED)), $(addprefix ., $(notdir $(DEP_FILTERED))))


ifneq ($(MAKECMDGOALS),clean)
include $(DEP_FILES)
endif


ifeq ($(HOST),mingw32-linux)
DEPENDS_PATH := $(PATH_TO_TOP)/tools
endif

ifeq ($(HOST),mingw32-windows)
DEPENDS_PATH := $(subst /,\,$(PATH_TO_TOP))\tools
endif

.%.d: %.c $(PATH_TO_TOP)/tools/depends$(EXE_POSTFIX)
	$(CC) $(CFLAGS) -M $< | $(DEPENDS_PATH)$(SEP)depends$(EXE_POSTFIX) $(@D) $@

.%.d: %.cc $(PATH_TO_TOP)/tools/depends$(EXE_POSTFIX)
	$(CC) $(CFLAGS) -M $< | $(DEPENDS_PATH)$(SEP)depends$(EXE_POSTFIX) $(@D) $@

.%.d: %.cpp $(PATH_TO_TOP)/tools/depends$(EXE_POSTFIX)
	$(CC) $(CFLAGS) -M $< | $(DEPENDS_PATH)$(SEP)depends$(EXE_POSTFIX) $(@D) $@

.%.d: %.s  $(PATH_TO_TOP)/tools/depends$(EXE_POSTFIX)
	$(CC) $(CFLAGS) -M $< | $(DEPENDS_PATH)$(SEP)depends$(EXE_POSTFIX) $(@D) $@

.%.d: %.S  $(PATH_TO_TOP)/tools/depends$(EXE_POSTFIX)
	$(CC) $(CFLAGS) -M $< | $(DEPENDS_PATH)$(SEP)depends$(EXE_POSTFIX) $(@D) $@

.%.d: %.asm $(PATH_TO_TOP)/tools/depends$(EXE_POSTFIX)
	$(NASM_CMD) $< | $(DEPENDS_PATH)$(SEP)depends$(EXE_POSTFIX) $(@D) $@
