
# Attempt to find the gcc installation

# ----------------------------------------------------------------------------- Get the architecture

ifeq ($(shell uname), Darwin)
   IS_DARWIN:=1
   ifeq ($(shell uname -m), arm64)
      ARCH:=aarch64-apple-darwin21
   else
      ARCH:=x86_64-apple-darwin21
   endif
else
   IS_DARWIN:=0
   ARCH:=x86_64-linux-gnu
   # Or could be x86_64-pc-linux-gnu
   ARCH2:=x86_64-pc-linux-gnu
endif

# ------------------------------------------------------ This is the "toolchain" file to be included
TOOLCHAIN_NAME?=gcc
CC_MAJOR_VERSION?=9
TOOLCHAIN_ROOT?=$(shell echo $$(dirname $$(dirname $$(which $(TOOLCHAIN_NAME)))))

ifeq ($(TOOLCHAIN_ROOT), /)
   TOOLCHAIN_ROOT:=/usr
endif

# Executables, should be symlinks like: /usr/local/bin/gcc-11 
CC:=$(TOOLCHAIN_ROOT)/bin/gcc-$(CC_MAJOR_VERSION)
CXX:=$(TOOLCHAIN_ROOT)/bin/g++-$(CC_MAJOR_VERSION)
RANLIB:=$(TOOLCHAIN_ROOT)/bin/gcc-ranlib-$(CC_MAJOR_VERSION)
AR:=$(TOOLCHAIN_ROOT)/bin/gcc-ar-$(CC_MAJOR_VERSION)
GCOV:=$(TOOLCHAIN_ROOT)/bin/gcov-$(CC_MAJOR_VERSION)

# Header/library directories
STDHDR_DIR:=$(TOOLCHAIN_ROOT)/include/c++/$(CC_MAJOR_VERSION)
ARCHDR_DIR:=$(STDHDR_DIR)/$(ARCH)
CPPLIB_DIR:=$(TOOLCHAIN_ROOT)/lib/gcc/$(CC_MAJOR_VERSION)

# Look for ARCHDR_DIR
ifeq (, $(shell test -d $(ARCHDR_DIR) && echo "found"))
   ARCHDR_DIR:=$(TOOLCHAIN_ROOT)/include/$(ARCH)/c++/$(CC_MAJOR_VERSION)
endif
ifeq (, $(shell test -d $(ARCHDR_DIR) && echo "found"))
   ARCHDR_DIR:=$(STDHDR_DIR)/$(ARCH2)
endif
ifeq (, $(shell test -d $(ARCHDR_DIR) && echo "found"))
   ARCHDR_DIR:=$(TOOLCHAIN_ROOT)/include/$(ARCH2)/c++/$(CC_MAJOR_VERSION)
endif

# Look again for CPPLIB_DIR
ifeq (, $(shell test -d $(CPPLIB_DIR) && echo "found"))
   CPPLIB_DIR:=$(TOOLCHAIN_ROOT)/lib/gcc/$(ARCH)/$(CC_MAJOR_VERSION)
endif

# Check that we found the installation
ifeq (, $(shell which $(CC)))
   $(error "ERROR: failed to find CC=$(CC)")
endif
ifeq (, $(shell which $(CXX)))
   $(error "ERROR: failed to find CXX=$(CXX)")
endif
ifeq (, $(shell which $(RANLIB)))
   $(error "ERROR: failed to find RANLIB=$(RANLIB)")
endif
ifeq (, $(shell which $(AR)))
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

