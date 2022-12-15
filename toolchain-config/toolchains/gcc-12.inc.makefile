
TOOLCHAIN_NAME:=gcc-12
CC_MAJOR_VERSION:=12

include project-config/toolchains/gcc-x.inc.makefile

CPP_INC:=-fPIC -nostdinc++ -isystem$(STDHDR_DIR) -isystem$(ARCHDR_DIR)

LINK_LIBCPP_SO:=-lm -L$(CPPLIB_DIR) -Wl,-rpath,$(CPPLIB_DIR) -lstdc++ 
LINK_LIBCPP_A:=-lm -L$(CPPLIB_DIR) -static-libgcc -static-libstdc++ -lstdc++ 
#LINK_LIBCPP_A:=-lm -Wl,-Bstatic -L$(CPPLIB_DIR) -lstdc++ -Wl,-Bdynamic 

GDB_FLAGS:=-g3 -gdwarf-2 -fno-omit-frame-pointer -fno-optimize-sibling-calls
W_FLAGS:=-Wall -Wextra -Wpedantic -Winvalid-pch -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wno-unused-parameter -Woverloaded-virtual -Wconversion -Wsign-conversion -Wnull-dereference -Wdouble-promotion -Wno-padded -Wformat-nonliteral -Wno-format-nonliteral -Wno-shadow -Wno-sign-conversion -Wno-conversion -Wno-cast-align -Wno-double-promotion -Wno-unused-but-set-variable -Wno-unused-variable
C_W_FLAGS:=-Wall -Wextra -Wpedantic -Winvalid-pch
D_FLAGS:=-DDEBUG_BUILD
R_FLAGS:=-DRELEASE_BUILD -DNDEBUG
O_FLAG:=-O2
C_F_FLAGS:=-fPIC -fvisibility=hidden -fdiagnostics-color=always -fmax-errors=4
F_FLAGS:=$(C_F_FLAGS) -fmodules-ts -fvisibility-inlines-hidden
S_FLAGS:=-g -fno-omit-frame-pointer -fno-optimize-sibling-calls
L_FLAGS:=
LTO_FLAGS:=-ffat-lto-objects
LTO_LINK:=-fuse-linker-plugin -flto
ASAN_FLAGS:=-g3 -gdwarf-2 -DADDRESS_SANITIZE -fsanitize=address
ASAN_LINK:=-Wl,-rpath,$(CPPLIB_DIR) -fsanitize=address
USAN_FLAGS:=-g3 -gdwarf-2 -DUNDEFINED_SANITIZE -fsanitize=undefined
USAN_LINK:=-Wl,-rpath,$(CPPLIB_DIR) -fsanitize=undefined
TSAN_FLAGS:=-g3 -gdwarf-2 -DTHREAD_SANITIZE -fsanitize=thread -fPIE
TSAN_LINK:=-Wl,-rpath,$(CPPLIB_DIR) -fsanitize=thread -fPIE
PROF_FLAGS:=-DBENCHMARK_BUILD -fno-omit-frame-pointer -g
PROF_LINK:=-lbenchmark

