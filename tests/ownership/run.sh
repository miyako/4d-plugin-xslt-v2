#!/bin/bash
# Builds the ownership harness against a source tree and runs every test in
# its own process under ASan/UBSan.
#
#   run.sh [--baseline] <source-tree> <build-dir>
#
# Default mode: every test must pass.
# --baseline:   defect_* and dormant_* tests must fail (proving the defect),
#               control_* tests must pass.
set -u

expect_baseline=0
if [ "${1:-}" = "--baseline" ]; then expect_baseline=1; shift; fi
src=$(cd "$1" && pwd)
build=$2
here=$(cd "$(dirname "$0")" && pwd)
mkdir -p "$build"

san="-w -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -g -O1"
inc=(-I "$here/stubs" -I "$here" -I "$src" -I "$src/4D Plugin API" -I "$src/4D Plugin API/Classes" -I "$src/support")

set -e
clang++ -x objective-c++ -Wno-multichar $san "${inc[@]}" -c "$src/4D Plugin API/4DPluginAPI.c" -o "$build/4DPluginAPI.o"
for f in "4D Plugin API/Classes/C_BLOB.cpp" "4D Plugin API/Classes/C_TEXT.cpp" "support/4DPlugin-JSON.cpp"; do
    clang++ -std=c++17 -x objective-c++ $san "${inc[@]}" -c "$src/$f" -o "$build/$(basename "$f" .cpp).o"
done
clang++ -std=c++17 -x objective-c++ $san "${inc[@]}" -c "$src/4DPlugin-XSLT.cpp" -o "$build/4DPlugin-XSLT.o"
clang++ -std=c++17 -x objective-c++ $san "${inc[@]}" -c "$here/fake4d.cpp" -o "$build/fake4d.o"
clang++ -std=c++17 -x objective-c++ $san "${inc[@]}" -c "$here/harness.cpp" -o "$build/harness.o"
clang++ $san "$build"/*.o -framework Foundation -framework CoreFoundation -framework CoreGraphics -o "$build/harness"
set +e

status=0
for t in $("$build/harness" --list); do
    "$build/harness" "$t" > "$build/$t.log" 2>&1
    rc=$?
    if [ $rc -eq 0 ]; then result=PASS; else result=FAIL; fi
    expected=PASS
    if [ $expect_baseline -eq 1 ] && [[ "$t" != control_* ]]; then expected=FAIL; fi
    if [ "$result" = "$expected" ]; then verdict=ok; else verdict=UNEXPECTED; status=1; fi
    echo "$result $t ($verdict)"
    if [ "$result" = FAIL ]; then grep -E "CHECK FAILED|OWNERSHIP VIOLATION|ERROR: AddressSanitizer|runtime error" "$build/$t.log" | head -5 | sed 's/^/    /'; fi
done
exit $status
