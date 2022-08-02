
# These will be set from the outside
TARGET?=zero
SOURCES?=$(shell find src -type f -name '*.cpp' -o -name '*.c')
TOOLCHAIN_NAME?=gcc-11
TOOLCHAIN_CONFIG?=release
STATIC_LIBCPP?=0
VERBOSE?=0
LTO?=0
UNITY_BUILD?=0
BUILD_TESTS?=0
BUILD_EXAMPLES?=0
BENCHMARK?=0
CXXSTD?=-std=c++2b

# -------------------------------------------------------------------------- Configure build options

ifneq ("$(BUILD_TESTS)", "0")
  SOURCES+= $(shell find testcases -type f -name '*.cpp' -o -name '*.c')
  CPPFLAGS+= -DCATCH_BUILD -DCATCH_CONFIG_PREFIX_ALL -DCATCH_CONFIG_COLOUR_ANSI -isystemtestcases
endif

ifneq ("$(BUILD_EXAMPLES)", "0")
  SOURCES+= $(shell find examples -type f -name '*.cpp' -o -name '*.c')
  CPPFLAGS+= -DBUILD_EXAMPLES -Wno-unused-function
endif

ifneq ("$(BENCHMARK)", "0")
  SOURCES+= $(shell find benchmark -type f -name '*.cpp' -o -name '*.c')
  CPPFLAGS+= -DBENCHMARK_BUILD
  LDFLAGS+= -L/usr/local/lib -lbenchmark
endif

BOOST_DEFINES:=-DBOOST_NO_TYPEID -DBOOST_ERROR_CODE_HEADER_ONLY -DBOOST_ASIO_SEPARATE_COMPILATION -DBOOST_ASIO_NO_DEPRECATED -DBOOST_ASIO_DISABLE_VISIBILITY
DEFINES:=$(BOOST_DEFINES) -DUSE_ASIO
WARNINGS:=-Wno-variadic-macros

# Configure includes
INCDIRS:=-Isrc -isystemcontrib/include -isystem/usr/local/include
LDFLAGS+=-lunifex -lpthread -lssl -lcrypto
CFLAGS+= $(INCDIRS) $(DEFINES)
CXXFLAGS+= -DFMT_HEADER_ONLY $(INCDIRS) $(DEFINES) $(WARNINGS)

# -------------------------------------------------------- Check that we're in the correct directory
# Every shell command is executed in the same invocation
MKFILE_PATH:=$(abspath $(lastword $(MAKEFILE_LIST)))
MKFILE_DIR:=$(patsubst %/,%,$(dir $(MKFILE_PATH)))
ifneq ("$(MKFILE_DIR)", "$(CURDIR)")
  $(error Should run from $(MKFILE_DIR) not $(CURDIR))
endif

# -------------------------------------------------- Include makefile environment and standard rules

include project-config/env.inc.makefile

-include $(DEP_FILES)

# -------------------------------------------------------------------------------------------- Rules
# Standard Rules
.PHONY: clean info test deps test-scan module-deps coverage

all: $(TARGETDIR)/$(TARGET)

test: | all
	$(TARGETDIR)/$(TARGET) src/main.cpp

$(TARGETDIR)/$(TARGET): $(OBJECTS)
	@echo "$(BANNER)link $(TARGET)$(BANEND)"
	mkdir -p $(dir $@)
	$(CC) -o $@ $^ $(LDFLAGS_F)
	@$(RECIPETAIL)

clean:
	@echo rm -rf $(BUILDDIR) $(TARGETDIR)
	@rm -rf $(BUILDDIR) $(TARGETDIR)

coverage: $(TARGETDIR)/$(TARGET)
	@echo "running target"
	$(TARGETDIR)/$(TARGET)
	@echo "generating coverage"
	gcovr --gcov-executable $(GCOV) --root . --object-directory $(BUILDDIR) --exclude contrib --exclude testcases

coverage_html: $(TARGETDIR)/$(TARGET)
	@echo "running target"
	$(TARGETDIR)/$(TARGET)
	@echo "running lcov to generate coverage"
	lcov --gcov-tool $(GCOV) -c --directory $(BUILDDIR) --output-file $(TARGETDIR)/app_info.info
	genhtml $(TARGETDIR)/app_info.info --output-directory doc/html/coverage

llvm_coverage_html: $(TARGETDIR)/$(TARGET)
	@echo "running target"
	$(TARGETDIR)/$(TARGET)
	@echo "generating coverage..."
	$(LLVM_PROFDATA) merge -o default.prof default.profraw
	$(LLVM_COV) export -format lcov -instr-profile default.prof $(TARGETDIR)/$(TARGET) > $(TARGETDIR)/app_info.info
	rm -f default.profraw default.prof
	genhtml $(TARGETDIR)/app_info.info --output-directory doc/html/coverage

info:
	@echo "CURDIR:        $(CURDIR)"
	@echo "MKFILE_DIR:    $(MKFILE_DIR)"
	@echo "TARGET:        $(TARGET)"
	@echo "TARGETDIR:     $(TARGETDIR)"
	@echo "BUILDDIR:      $(BUILDDIR)"
	@echo "COMP_DATABASE: $(COMP_DATABASE)"
	@echo "CONFIG:        $(TOOLCHAIN_CONFIG)"
	@echo "VERBOSE:       $(VERBOSE)"
	@echo "CC:            $(CC)"
	@echo "CXX:           $(CXX)"
	@echo "CFLAGS:        $(CFLAGS_F)"
	@echo "CXXFLAGS:      $(CXXFLAGS_F)"
	@echo "LDFLAGS:       $(LDFLAGS_F)"
	@echo "SOURCES:"
	@echo "$(SOURCES)" | sed 's,^,   ,'
	@echo "OBJECTS:"
	@echo "$(OBJECTS)" | sed 's,^,   ,'
	@echo "DEP_FILES:"
	@echo "$(DEP_FILES)" | sed 's,^,   ,'


