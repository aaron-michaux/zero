gcmdir:
	@mkdir -p $(GCMDIR)
	@rm -f gcm.cache
	@ln -s $(GCMDIR) gcm.cache

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/any.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header any"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) any

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/bitset.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header bitset"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) bitset

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/chrono.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header chrono"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) chrono

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/compare.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header compare"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) compare

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/csetjmp.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header csetjmp"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) csetjmp

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/csignal.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header csignal"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) csignal

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cstdarg.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header cstdarg"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) cstdarg

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cstdlib.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header cstdlib"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) cstdlib

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/ctime.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header ctime"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) ctime

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/functional.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header functional"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) functional

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/initializer_list.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header initializer_list"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) initializer_list

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/optional.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header optional"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) optional

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/source_location.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header source_location"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) source_location

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/tuple.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header tuple"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) tuple

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/type_traits.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header type_traits"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) type_traits

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/typeindex.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header typeindex"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) typeindex

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/typeinfo.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header typeinfo"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) typeinfo

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/utility.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header utility"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) utility

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/variant.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header variant"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) variant

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/version.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header version"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) version

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/memory.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header memory"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) memory

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/memory_resource.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header memory_resource"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) memory_resource

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/new.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header new"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) new

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/scoped_allocator.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header scoped_allocator"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) scoped_allocator

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cfloat.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header cfloat"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) cfloat

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cinttypes.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header cinttypes"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) cinttypes

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/climits.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header climits"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) climits

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cstdint.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header cstdint"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) cstdint

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/limits.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header limits"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) limits

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cassert.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header cassert"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) cassert

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cerrno.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header cerrno"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) cerrno

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/exception.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header exception"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) exception

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/stdexcept.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header stdexcept"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) stdexcept

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/system_error.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header system_error"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) system_error

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cctype.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header cctype"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) cctype

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/charconv.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header charconv"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) charconv

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cstring.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header cstring"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) cstring

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cuchar.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header cuchar"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) cuchar

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cwchar.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header cwchar"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) cwchar

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cwctype.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header cwctype"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) cwctype

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/string.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header string"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) string

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/string_view.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header string_view"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) string_view

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/array.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header array"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) array

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/deque.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header deque"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) deque

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/forward_list.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header forward_list"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) forward_list

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/list.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header list"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) list

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/map.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header map"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) map

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/queue.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header queue"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) queue

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/set.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header set"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) set

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/stack.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header stack"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) stack

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/unordered_map.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header unordered_map"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) unordered_map

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/unordered_set.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header unordered_set"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) unordered_set

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/vector.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header vector"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) vector

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/iterator.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header iterator"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) iterator

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/ranges.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header ranges"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) ranges

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/algorithm.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header algorithm"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) algorithm

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/bit.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header bit"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) bit

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cfenv.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header cfenv"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) cfenv

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cmath.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header cmath"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) cmath

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/complex.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header complex"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) complex

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/numbers.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header numbers"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) numbers

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/numeric.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header numeric"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) numeric

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/random.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header random"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) random

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/ratio.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header ratio"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) ratio

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/valarray.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header valarray"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) valarray

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/clocale.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header clocale"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) clocale

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/locale.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header locale"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) locale

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cstdio.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header cstdio"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) cstdio

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/fstream.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header fstream"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) fstream

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/iomanip.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header iomanip"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) iomanip

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/ios.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header ios"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) ios

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/iosfwd.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header iosfwd"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) iosfwd

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/iostream.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header iostream"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) iostream

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/istream.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header istream"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) istream

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/ostream.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header ostream"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) ostream

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/sstream.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header sstream"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) sstream

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/streambuf.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header streambuf"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) streambuf

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/syncstream.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header syncstream"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) syncstream

gcm.cache/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/atomic.gcm: | gcmdir
	@echo " \e[36m\e[1m⚡\e[0m c++-system-header atomic"
	@$(CXX) -x c++-system-header $(CXXFLAGS_F) atomic

gcm.cache/libstdcppm: \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/any.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/bitset.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/chrono.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/compare.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/csetjmp.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/csignal.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cstdarg.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cstdlib.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/ctime.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/functional.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/initializer_list.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/optional.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/source_location.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/tuple.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/type_traits.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/typeindex.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/typeinfo.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/utility.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/variant.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/version.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/memory.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/memory_resource.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/new.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/scoped_allocator.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cfloat.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cinttypes.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/climits.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cstdint.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/limits.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cassert.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cerrno.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/exception.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/stdexcept.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/system_error.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cctype.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/charconv.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cstring.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cuchar.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cwchar.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cwctype.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/string.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/string_view.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/array.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/deque.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/forward_list.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/list.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/map.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/queue.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/set.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/stack.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/unordered_map.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/unordered_set.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/vector.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/iterator.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/ranges.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/algorithm.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/bit.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cfenv.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cmath.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/complex.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/numbers.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/numeric.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/random.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/ratio.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/valarray.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/clocale.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/locale.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/cstdio.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/fstream.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/iomanip.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/ios.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/iosfwd.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/iostream.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/istream.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/ostream.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/sstream.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/streambuf.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/syncstream.gcm \
     $(GCMDIR)/$(TOOLCHAIN_ROOT)/include/c++/11.2.0/atomic.gcm
	@touch $(GCMDIR)/libstdcppm
