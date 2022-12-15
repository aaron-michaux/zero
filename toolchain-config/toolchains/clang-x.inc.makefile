
# Attempt to find the gcc installation

# ----------------------------------------------------------------------------- Get the architecture

ifeq ($(shell uname), Darwin)
   $(error "ERROR: llvm not supported on Darwin")
endif

# ------------------------------------------------------ This is the "toolchain" file to be included
TOOLCHAIN_NAME?=llvm
CC_MAJOR_VERSION?=14
TOOLCHAIN_ROOT?=/opt/cc/clang-$(CC_MAJOR_VERSION)

# Executables, should be symlinks like: /usr/local/bin/gcc-11 
CC:=$(TOOLCHAIN_ROOT)/bin/clang
CXX:=$(TOOLCHAIN_ROOT)/bin/clang++
LLD:=$(TOOLCHAIN_ROOT)/bin/ld.lld
RANLIB:=$(TOOLCHAIN_ROOT)/bin/llvm-ranlib
AR:=$(TOOLCHAIN_ROOT)/bin/llvm-ar
LLVM_COV:=$(TOOLCHAIN_ROOT)/bin/llvm-cov
LLVM_PROFDATA:=$(TOOLCHAIN_ROOT)/bin/llvm-profdata

# Header/library directories
STDHDR_DIR:=$(TOOLCHAIN_ROOT)/include
CPPLIB_DIR:=$(TOOLCHAIN_ROOT)/lib

# Check that we found the installation
ifeq (, $(shell test -s $(CC) && echo "found"))
   $(error "ERROR: failed to find CC=$(CC)")
endif
ifeq (, $(shell test -s $(CXX) && echo "found"))
   $(error "ERROR: failed to find CXX=$(CXX)")
endif
ifeq (, $(shell test -s $(RANLIB) && echo "found"))
   $(error "ERROR: failed to find RANLIB=$(RANLIB)")
endif
ifeq (, $(shell test -s $(AR) && echo "found"))
   $(error "ERROR: failed to find RANLIB=$(AR)")
endif
ifeq (, $(shell test -d $(STDHDR_DIR) && echo "found"))
   $(error "ERROR: failed to find c++ headers at $(STDHDR_DIR)")
endif
ifeq (, $(shell test -d $(ARCHDR_DIR) && echo "found"))
   $(error "ERROR: failed to find c++ headers at $(ARCHDR_DIR)")
endif
ifeq (, $(shell test -d $(CPPLIB_DIR) && echo "found"))
   $(error "ERROR: failed to find c++ library at $(CPPLIB_DIR)")
endif

