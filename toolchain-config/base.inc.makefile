
# -------------------------------------------------------------------------------- Setup Environment

# Get the build directory
TOOLCHAIN:=clang
BUILD_CONFIG:=asan

VALID_TOOLCHAINS:="gcc clang"
ifneq ($(filter $(TOOLCHAIN),$(VALID_TOOLCHAINS)),)
   $(error "Toolchain '$(TOOLCHAIN)' not [$(VALID_TOOLCHAINS)]")
endif
VALID_BUILD_CONFIGS:=debug release asan usan tsan reldbg
ifneq ($(filter $(BUILD_CONFIG),$(VALID_BUILD_CONFIGS)), $(BUILD_CONFIG))
   $(error "Build config '$(BUILD_CONFIG)' not [$(VALID_BUILD_CONFIGS)]")
endif

MAKE_ENV_INC_FILE:=$(shell $(CURDIR)/../../installation/bin/toolchain-env.sh --print --write-make-env-inc --target=$(TARGET) --toolchain=$(TOOLCHAIN) --stdlib=$(STDLIB) --build-config=$(BUILD_CONFIG) --lto=$(LTO) --coverage=$(COVERAGE) --unity=$(UNITY_BUILD) --build-tests=$(BUILD_TESTS) --build-examples=$(BUILD_EXAMPLES) --benchmark=$(BENCHMARK) 2>&1 | grep MAKE_ENV_INC_FILE | awk -F= '{ print $$2 }' || true)

ifeq (, $(shell test -f "$(MAKE_ENV_INC_FILE)" && echo "found"))
   $(error "Error: failed to generate make-inc file: $(MAKE_ENV_INC_FILE)")
endif

include $(MAKE_ENV_INC_FILE)

# -------------------------------------------------------------------------------------------- Logic

# This is the "toolchain" file to be included
TARGET_DIR?=build/$(UNIQUE_DIR)
GCMDIR:=$(BUILDDIR)/gcm.cache
CPP_SOURCES:=$(filter %.cpp, $(SOURCES))
C_SOURCES:=$(filter %.c, $(SOURCES))
OBJECTS:=$(addprefix $(BUILDDIR)/, $(patsubst %.cpp, %.o, $(CPP_SOURCES)) $(patsubst %.c, %.o, $(C_SOURCES)))
DEP_FILES:=$(addsuffix .d, $(OBJECTS))
COMPDBS:=$(addprefix $(BUILDDIR)/, $(patsubst %.cpp, %.comp-db.json, $(CPP_SOURCES)) $(patsubst %.c, %.comp-db.json, $(C_SOURCES)))
COMP_DATABASE:=$(TARGET_DIR)/compilation-database.json

# Unity build
UNITY_O:=$(BUILDDIR)/unity-file.o
ifeq ("$(UNITY_BUILD)", "True")
  SOURCES:=$(C_SOURCES)
  OBJECTS:=$(addprefix $(BUILDDIR)/, $(patsubst %.c, %.o, $(C_SOURCES))) $(UNITY_O)
endif

# Static libcpp
CFLAGS_0:=
CXXFLAGS_0:=$(CXXLIB_FLAGS)
LDFLAGS_0:=$(CXXLIB_LDFLAGS) $(CXXLIB_LIBS)

# Add asan|usan|tsan|debug|release|reldbg
ifeq ($(BUILD_CONFIG), asan)
  CFLAGS_1:=-O0 $(C_W_FLAGS) $(C_F_FLAGS) $(D_FLAGS) $(ASAN_FLAGS) $(CFLAGS_0)
  CXXFLAGS_1:=-O0 $(W_FLAGS) $(F_FLAGS) $(D_FLAGS) $(ASAN_FLAGS) $(CXXFLAGS_0)
  LDFLAGS_1:=$(LDFLAGS) $(ASAN_LINK) $(LDFLAGS_0)
else ifeq ($(BUILD_CONFIG), usan)
  CFLAGS_1:=-O0 $(C_W_FLAGS) $(C_F_FLAGS) $(S_FLAGS) $(USAN_FLAGS) $(CFLAGS_0)
  CXXFLAGS_1:=-O0 $(W_FLAGS) $(F_FLAGS) $(S_FLAGS) $(USAN_FLAGS) $(CXXFLAGS_0)
  LDFLAGS_1:=$(LDFLAGS) $(USAN_LINK) $(LDFLAGS_0)
else ifeq ($(BUILD_CONFIG), tsan)
  CFLAGS_1:=-O1 $(C_W_FLAGS) $(C_F_FLAGS) $(S_FLAGS) $(TSAN_FLAGS) $(CFLAGS_0)
  CXXFLAGS_1:=-O1 $(W_FLAGS) $(F_FLAGS) $(S_FLAGS) $(TSAN_FLAGS) $(CXXFLAGS_0)
  LDFLAGS_1:=$(LDFLAGS) $(TSAN_LINK) $(LDFLAGS_0)
else ifeq ($(BUILD_CONFIG), debug)
  CFLAGS_1:=-O0 $(C_W_FLAGS) $(C_F_FLAGS) $(D_FLAGS) $(GDB_FLAGS) $(CFLAGS_0)
  CXXFLAGS_1:=-O0 $(W_FLAGS) $(F_FLAGS) $(D_FLAGS) $(GDB_FLAGS) $(CXXFLAGS_0)
  LDFLAGS_1:=$(LDFLAGS) $(LDFLAGS_0)
else ifeq ($(BUILD_CONFIG), reldbg)
  CFLAGS_1:=-O3 -g $(C_W_FLAGS) $(C_F_FLAGS) $(R_FLAGS) $(CFLAGS_0)
  CXXFLAGS_1:=-O3 -g $(W_FLAGS) $(F_FLAGS) $(R_FLAGS) $(CXXFLAGS_0)
  LDFLAGS_1:=$(LDFLAGS) $(LDFLAGS_0)
else ifeq ($(BUILD_CONFIG), release)
  CFLAGS_1:=-O3 $(C_W_FLAGS) $(C_F_FLAGS) $(R_FLAGS) $(CFLAGS_0)
  CXXFLAGS_1:=-O3 $(W_FLAGS) $(F_FLAGS) $(R_FLAGS) $(CXXFLAGS_0)
  LDFLAGS_1:=$(LDFLAGS) $(LDFLAGS_0)
else
  $(error Unknown configuration: $(BUILD_CONFIG))
endif

# Add LTO
ifeq ($(LTO), True)
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
CXXFLAGS_F:=$(CXXSTD) $(CXXFLAGS_2) $(CXXFLAGS) $(CPPFLAGS)
LDFLAGS_F:=$(LDFLAGS_2)

# Visual feedback rules
ifneq ($(VERBOSE), "False")
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
ifeq ("$(OPERATING_SYSTEM)", "macos")
  SED:=gsed
  ifeq (, $(shell which $(SED)))
    $(error "ERROR: failed to find $(SED) on path, consider running 'brew install gnu-sed'")
  endif	
else
  SED:=sed
endif

# Include dependency files
-include $(DEP_FILES)

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


