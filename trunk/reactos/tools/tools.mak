TOOLS_BASE = tools
TOOLS_BASE_ = $(TOOLS_BASE)$(SEP)
TOOLS_INT = $(INTERMEDIATE_)$(TOOLS_BASE)
TOOLS_INT_ = $(TOOLS_INT)$(SEP)
TOOLS_OUT = $(OUTPUT_)$(TOOLS_BASE)
TOOLS_OUT_ = $(TOOLS_OUT)$(SEP)

$(TOOLS_INT): | $(INTERMEDIATE)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(TOOLS_OUT): | $(OUTPUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif


include tools/rsym.mak
include tools/bin2res/bin2res.mak
include tools/buildno/buildno.mak
include tools/cabman/cabman.mak
include tools/cdmake/cdmake.mak
include tools/gendib/gendib.mak
include tools/mkhive/mkhive.mak
include tools/nci/nci.mak
include tools/rbuild/rbuild.mak
include tools/unicode/unicode.mak
include tools/widl/widl.mak
include tools/winebuild/winebuild.mak
include tools/wmc/wmc.mak
include tools/wpp/wpp.mak
include tools/wrc/wrc.mak
