BUILDNO_BASE = $(TOOLS_BASE_)buildno
BUILDNO_BASE_ = $(BUILDNO_BASE)$(SEP)
BUILDNO_INT = $(INTERMEDIATE_)$(BUILDNO_BASE)
BUILDNO_INT_ = $(BUILDNO_INT)$(SEP)
BUILDNO_OUT = $(OUTPUT_)$(BUILDNO_BASE)
BUILDNO_OUT_ = $(BUILDNO_OUT)$(SEP)

$(BUILDNO_INT): | $(TOOLS_INT)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(BUILDNO_OUT): | $(TOOLS_OUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif

BUILDNO_TARGET = \
	$(EXEPREFIX)$(BUILDNO_OUT_)buildno$(EXEPOSTFIX)

BUILDNO_SOURCES = $(addprefix $(BUILDNO_BASE_), \
	buildno.cpp \
	exception.cpp \
	ssprintf.cpp \
	xml.cpp \
	)

BUILDNO_OBJECTS = \
  $(addprefix $(INTERMEDIATE_), $(BUILDNO_SOURCES:.cpp=.o))

BUILDNO_HOST_CXXFLAGS = -Iinclude/reactos $(TOOLS_CPPFLAGS)

BUILDNO_HOST_LFLAGS = $(TOOLS_LFLAGS)

$(BUILDNO_TARGET): $(BUILDNO_OBJECTS) | $(BUILDNO_OUT)
	$(ECHO_LD)
	${host_gpp} $(BUILDNO_OBJECTS) $(BUILDNO_HOST_LFLAGS) -o $@

$(BUILDNO_INT_)buildno.o: $(BUILDNO_BASE_)buildno.cpp | $(BUILDNO_INT)
	$(ECHO_CC)
	${host_gpp} $(BUILDNO_HOST_CXXFLAGS) -c $< -o $@

$(BUILDNO_INT_)exception.o: $(BUILDNO_BASE_)exception.cpp | $(BUILDNO_INT)
	$(ECHO_CC)
	${host_gpp} $(BUILDNO_HOST_CXXFLAGS) -c $< -o $@

$(BUILDNO_INT_)ssprintf.o: $(BUILDNO_BASE_)ssprintf.cpp | $(BUILDNO_INT)
	$(ECHO_CC)
	${host_gpp} $(BUILDNO_HOST_CXXFLAGS) -c $< -o $@

$(BUILDNO_INT_)xml.o: $(BUILDNO_BASE_)xml.cpp | $(BUILDNO_INT)
	$(ECHO_CC)
	${host_gpp} $(BUILDNO_HOST_CXXFLAGS) -c $< -o $@

.PHONY: buildno_clean
buildno_clean:
	-@$(rm) $(BUILDNO_TARGET) $(BUILDNO_OBJECTS) 2>$(NUL)
clean: buildno_clean

$(BUILDNO_H): $(BUILDNO_TARGET)
	$(ECHO_BUILDNO)
	$(Q)$(BUILDNO_TARGET) $(BUILDNO_QUIET) $(BUILDNO_H)
