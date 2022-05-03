
raw_headers()
{
    cat <<EOF
# Utilities Library
any
bitset
chrono
compare
csetjmp
csignal
cstdarg
cstdlib
ctime
functional
initializer_list
optional
source_location
stacktrace
tuple
type_traits
typeindex
typeinfo
utility
variant
version

# Dynamic Memory Management
memory
memory_resource
new
scoped_allocator

# Numeric Limits
cfloat
cinttypes
climits
cstdint
limits

# Error handling
cassert
cerrno
exception
stdexcept
system_error

# Strings
cctype
charconv
cstring
cuchar
cwchar
cwctype
format
string
string_view

# Containers
array
deque
forward_list
list
map
queue
set
span
stack
unordered_map
unordered_set
vector

# Iterators
iterator

# Ranges
ranges

# Algorithms
algorithm
execution

# Numerics
bit
cfenv
cmath
complex
numbers
numeric
random
ratio
valarray

# Localization
clocale
locale

# Input/output
cstdio
fstream
iomanip
ios
iosfwd
iostream
istream
ostream
# spanstream
sstream
streambuf
syncstream

# Filesystem
filesystem

# Atomic
atomic

# Thread support library
barrier
condition_variable
future
latch
mutex
semaphore
shared_mutex
stop_token
thread
EOF
}

headers()
{
    raw_headers | grep -v \# | sed 's,^\s*,,' | sed 's,\s*$,,' | grep -Ev '^$'
}

# headers | while read L ; do
#     cat <<EOF
# build \$gcmdir/std/$L: cpp_sysheaders
#      header = $L

# EOF
# done

printf "gcmdir:\n"
printf '\t%s\n' "@mkdir -p \$(GCMDIR)"
printf '\t%s\n' "@rm -f gcm.cache"
printf '\t%s\n' "@ln -s \$(GCMDIR) gcm.cache"
printf '\n'

headers | while read L ; do
printf '%s\n' "\$(GCMDIR)/\$(TOOLCHAIN_ROOT)/include/c++/11.2.0/$L.gcm: | gcmdir"
printf '\t%s\n' "@echo \" \e[36m\e[1m⚡\e[0m c++-system-header $L\""
printf '\t%s\n' "@\$(CXX) -x c++-system-header \$(CXXFLAGS_F) $L"
printf '\n'
done

printf '%s' "\$(GCMDIR)/libstdcppm:"
headers | while read L ; do
    printf ' \\\n     %s' "\$(GCMDIR)/\$(TOOLCHAIN_ROOT)/include/c++/11.2.0/$L.gcm"
done
printf '\n'
printf '\t%s\n' "@touch \$(GCMDIR)/libstdcppm"
	
