#!/usr/bin/env bash
# Usage: format_compile_checks.sh <c++ compiler> <PlussTils include dir> <PlussQit include dir> <PlussQit src dir> <generated dir> [sysroot]
# Literal patterns are validated while compiling: each bad pattern below must be rejected with its named error,
# and the good ones must compile.
set -u
CXX="$1"; INC="$2"; QINC="$3"; QSRC="$4"; GEN="$5"; SYSROOT="${6:-}"
ARGS=(-std=c++20 -fsyntax-only -I "$INC" -I "$QINC" -I "$QSRC" -I "$GEN")
[ -n "$SYSROOT" ] && ARGS+=(-isysroot "$SYSROOT")
tmp="$(mktemp -d)"; trap 'rm -rf "$tmp"' EXIT
fail=0

compile() {  # $1 = statement
  printf '#include <pluss/tils/kit/output.h>\n#include <sstream>\nint main(){ std::ostringstream os; pluss::tils::OutputEngine out(os); (void)out; %s }\n' "$1" > "$tmp/t.cpp"
  "$CXX" "${ARGS[@]}" "$tmp/t.cpp" 2>&1
}

expect_error() {  # $1 = statement, $2 = diagnostic tag
  if ! compile "$1" | grep -q "$2"; then echo "FAIL: [$1] should be rejected with $2"; fail=1; fi
}
expect_ok() {
  if out="$(compile "$1")" && [ -z "$out" ]; then :; else echo "FAIL: [$1] should compile"; echo "$out" | head -5; fail=1; fi
}

expect_error '(void)pluss::tils::format("{} {}", 1);'          format_string_error_missing_argument
expect_error '(void)pluss::tils::format("{2}", 1, 2);'         format_string_error_missing_argument
expect_error '(void)pluss::tils::format("{:{}}", 1);'          format_string_error_missing_argument
expect_error '(void)pluss::tils::format("{", 1);'              format_string_error_unmatched_brace
expect_error '(void)pluss::tils::format("}", 1);'              format_string_error_unmatched_brace
expect_error '(void)pluss::tils::format("{:q}", 1);'           format_string_error_bad_spec
expect_error '(void)pluss::tils::format("{:.}", 1);'           format_string_error_bad_spec
expect_error '(void)pluss::tils::format("{} {0}", 1);'         format_string_error_mixed_indexing
expect_error 'out.writelnf("{} {}", 1);'                       format_string_error_missing_argument
expect_error 'out.infof("{:z}", 1);'                           format_string_error_bad_spec
expect_error 'pluss::tils::print(os, "{}{}", 1);'              format_string_error_missing_argument

expect_ok '(void)pluss::tils::format("{} {:>{}.{}f}", 1, 2.0, 8, 3);'
expect_ok '(void)pluss::tils::format("{1} {0} {1}", 1, 2);'
expect_ok '(void)pluss::tils::format("{{}} {:*^9}", "ab");'
expect_ok '(void)pluss::tils::format("no fields");'
expect_ok '(void)pluss::tils::format(pluss::tils::runtime_format("{} {}"), 1);'  # a run-time pattern is only checked when it runs
expect_ok 'out.writelnf("{:03d}|{}", 5, "x"); out.warnf("{}", 1);'
exit $fail
