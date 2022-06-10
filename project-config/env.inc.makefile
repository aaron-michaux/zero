
include project-config/toolchains/$(TOOLCHAIN_NAME).inc.makefile

# -------------------------------------------------------------------------------------------- Logic

# This is the "toolchain" file to be included
BUILDDIR?=/tmp/build-$(USER)/$(TOOLCHAIN_NAME)-$(TOOLCHAIN_CONFIG)/$(TARGET)
TARGETDIR?=build/$(TOOLCHAIN_NAME)-$(TOOLCHAIN_CONFIG)
GCMDIR:=$(BUILDDIR)/gcm.cache
CPP_SOURCES:=$(filter %.cpp, $(SOURCES))
C_SOURCES:=$(filter %.c, $(SOURCES))
OBJECTS:=$(addprefix $(BUILDDIR)/, $(patsubst %.cpp, %.o, $(CPP_SOURCES)) $(patsubst %.c, %.o, $(C_SOURCES)))
DEP_FILES:=$(addsuffix .d, $(OBJECTS))
COMPDBS:=$(addprefix $(BUILDDIR)/, $(patsubst %.cpp, %.comp-db.json, $(CPP_SOURCES)) $(patsubst %.c, %.comp-db.json, $(C_SOURCES)))
COMP_DATABASE:=$(TARGETDIR)/compilation-database.json

# Unity build
UNITY_O:=$(BUILDDIR)/unity-file.o
ifneq ("$(UNITY_BUILD)", "0")
  SOURCES:=$(C_SOURCES)
  OBJECTS:=$(addprefix $(BUILDDIR)/, $(patsubst %.c, %.o, $(C_SOURCES))) $(UNITY_O)
endif

# Static libcpp
ifeq ($(STATIC_LIBCPP), 1)
  LINK_LIBCPP:=$(LINK_LIBCPP_A)
else
  LINK_LIBCPP:=$(LINK_LIBCPP_SO)
endif

# Add asan|usan|tsan|debug|release
ifeq ($(TOOLCHAIN_CONFIG), asan)
  CFLAGS_1:=-O0 $(C_W_FLAGS) $(C_F_FLAGS) $(D_FLAGS) $(ASAN_FLAGS)
  CXXFLAGS_1:=-O0 $(W_FLAGS) $(F_FLAGS) $(D_FLAGS) $(ASAN_FLAGS)
  LDFLAGS_1:=$(LINK_LIBCPP) $(LDFLAGS) $(ASAN_LINK)
else ifeq ($(TOOLCHAIN_CONFIG), usan)
  CFLAGS_1:=-O0 $(C_W_FLAGS) $(C_F_FLAGS) $(S_FLAGS) $(USAN_FLAGS)
  CXXFLAGS_1:=-O0 $(W_FLAGS) $(F_FLAGS) $(S_FLAGS) $(USAN_FLAGS)
  LDFLAGS_1:=$(LINK_LIBCPP) $(LDFLAGS) $(USAN_LINK)
else ifeq ($(TOOLCHAIN_CONFIG), tsan)
  CFLAGS_1:=-O1 $(C_W_FLAGS) $(C_F_FLAGS) $(S_FLAGS) $(TSAN_FLAGS)
  CXXFLAGS_1:=-O1 $(W_FLAGS) $(F_FLAGS) $(S_FLAGS) $(TSAN_FLAGS)
  LDFLAGS_1:=$(LINK_LIBCPP) $(LDFLAGS) $(TSAN_LINK)
else ifeq ($(TOOLCHAIN_CONFIG), debug)
  CFLAGS_1:=-O0 $(C_W_FLAGS) $(C_F_FLAGS) $(D_FLAGS) $(GDB_FLAGS)
  CXXFLAGS_1:=-O0 $(W_FLAGS) $(F_FLAGS) $(D_FLAGS) $(GDB_FLAGS)
  LDFLAGS_1:=$(LINK_LIBCPP) $(LDFLAGS)
else ifeq ($(TOOLCHAIN_CONFIG), reldbg)
  CFLAGS_1:=-O3 $(C_W_FLAGS) $(C_F_FLAGS) $(R_FLAGS) -g
  CXXFLAGS_1:=-O3 $(W_FLAGS) $(F_FLAGS) $(R_FLAGS) -g
  LDFLAGS_1:=$(LINK_LIBCPP) $(LDFLAGS)
else ifeq ($(TOOLCHAIN_CONFIG), release)
  CFLAGS_1:=-O3 $(C_W_FLAGS) $(C_F_FLAGS) $(R_FLAGS)
  CXXFLAGS_1:=-O3 $(W_FLAGS) $(F_FLAGS) $(R_FLAGS)
  LDFLAGS_1:=$(LINK_LIBCPP) $(LDFLAGS)
else
  $(error Unknown configuration: $(TOOLCHAIN_CONFIG))
endif

# Add LTO
ifeq ($(LTO), 1)
  CFLAGS_2:=$(CFLAGS_1) $(LTO_FLAGS)
  CXXFLAGS_2:=$(CXXFLAGS_1) $(LTO_FLAGS)
  LDFLAGS_2:=$(LDFLAGS_1) $(LTO_LINK)
else
  CFLAGS_2:=$(CFLAGS_1)
  CXXFLAGS_2:=$(CXXFLAGS_1)
  LDFLAGS_2:=$(LDFLAGS_1)
endif

# Final flags
CFLAGS_F:=$(CFLAGS_2) $(CFLAGS) $(CPPFLAGS)
CXXFLAGS_F:=$(CXXSTD) $(CXXFLAGS_2) $(CPP_INC) $(CXXFLAGS) $(CPPFLAGS)
LDFLAGS_F:=$(LDFLAGS_2)

# Visual feedback rules
ifeq ($(VERBOSE), 1)
  ISVERBOSE:=verbose
  BANNER:=$(shell printf "\# \e[1;37m-- ~ \e[1;37m\e[4m")
  BANEND:=$(shell printf "\e[0m\e[1;37m ~ --\e[0m")
  RECIPETAIL:=echo
else
  ISVERBOSE:=
  BANNER:=$(shell printf " \e[36m\e[1m⚡\e[0m ")
  BANEND:=
  RECIPETAIL:=
endif

# sed on OSX
UNAME:=$(shell uname -s)
ifeq ("$(UNAME)", "Darwin")
  SED:=gsed
  ifeq (, $(shell which $(SED)))
    $(error "ERROR: failed to find $(SED) on path, consider running 'brew install gnu-sed'")
  endif	
else
  SED:=sed
endif

# This must appear before SILENT
default: all

# Will be silent unless VERBOSE is set to 1
$(ISVERBOSE).SILENT:

# The build-unity rule
.PHONY: $(UNITY_O)
$(UNITY_O): $(CPP_SOURCES)
	@echo '$(BANNER)unity-build $@$(BANEND)'
	mkdir -p $(dir $@)
	echo $^ | tr ' ' '\n' | sort | grep -Ev '^\s*$$' | sed 's,^,#include ",' | sed 's,$$,",' | $(CXX) -x c++ $(CXXFLAGS_F) -c - -o $@

.PHONEY: unity_cpp
unity_cpp: $(CPP_SOURCES)
	echo $^ | tr ' ' '\n' | sort | grep -Ev '^\s*$$' | sed 's,^,#include ",' | sed 's,$$,",'

$(BUILDDIR)/%.o: %.cpp
	@echo "$(BANNER)c++ $<$(BANEND)"
	mkdir -p $(dir $@)
	$(CXX) -x c++ $(CXXFLAGS_F) -MMD -MF $@.d -c $< -o $@
	@$(RECIPETAIL)

comp-database: | $(COMP_DATABASE)

$(COMP_DATABASE): $(COMPDBS)
	@echo '$(BANNER)c++-system-header $<$(BANEND)'
	mkdir -p "$(dir $@)"
	echo "[" > $@
	cat $(COMPDBS) >> $@
	$(SED) -i '$$d' $@
	echo "]" >> $@

$(BUILDDIR)/%.comp-db.json: %.cpp
	@echo "$(BANNER)comp-db $<$(BANEND)"
	mkdir -p $(dir $@)
	printf "{ \"directory\": \"%s\",\n" "$$(echo "$(CURDIR)" | sed 's,\\,\\\\,g' | sed 's,",\\",g')" > $@
	printf "  \"file\":      \"%s\",\n" "$$(echo "$<" | sed 's,\\,\\\\,g' | sed 's,",\\",g')" >> $@
	printf "  \"command\":   \"%s\",\n" "$$(echo "$(CXX) -x c++ $(CXXFLAGS_F) -c $< -o $@" | sed 's,\\,\\\\,g' | sed 's,",\\",g')" >> $@
	printf "  \"output\":    \"%s\" }\n" "$$(echo "$@" | sed 's,\\,\\\\,g' | sed 's,",\\",g')" >> $@
	printf ",\n" >> $@
	@$(RECIPETAIL)

