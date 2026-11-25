# Usage: tests/run_tests.sh [path/to/hybrid_binary_translator] [name-filter]
#   TIMEOUT=10 controls the per-test timeout in seconds.

set -u
script_dir="$(cd "$(dirname "$0")" && pwd)"
translator="${1:-$script_dir/../build-rc/hybrid_binary_translator}"
filter="${2:-}"
timeout_s="${TIMEOUT:-10}"

if [[ ! -x "$translator" ]]; then
    echo "translator not found or not executable: $translator" >&2
    echo "build it with: cmake -S hybrid_binary_translator -B hybrid_binary_translator/build-rc -DCMAKE_BUILD_TYPE=Release && cmake --build hybrid_binary_translator/build-rc -j" >&2
    exit 2
fi

pass=0; fail=0; failed=()
for bin in "$script_dir"/cases/*.bin; do
    name="$(basename "$bin" .bin)"
    [[ -n "$filter" && "$name" != *"$filter"* ]] && continue
    expect_file="${bin%.bin}.expect"
    if [[ ! -f "$expect_file" ]]; then
        printf 'SKIP  %-20s (no .expect file)\n' "$name"; continue
    fi

    expected="$(grep -v '^#' "$expect_file")"
    actual_out="$(perl -e 'alarm shift; exec @ARGV' "$timeout_s" "$translator" "$bin" 2>&1)"
    code=$?
    if (( code == 142 )); then actual_out+=$'\n'"(timed out after ${timeout_s}s)"; fi
    actual="${actual_out:+$actual_out$'\n'}exit=$code"

    if [[ "$actual" == "$expected" ]]; then
        printf 'PASS  %s\n' "$name"; pass=$((pass + 1))
    else
        printf 'FAIL  %s\n' "$name"; fail=$((fail + 1)); failed+=("$name")
        diff <(echo "$expected") <(echo "$actual") | sed -n 's/^</    expected:/p; s/^>/    actual:  /p'
    fi
done

echo
echo "$pass passed, $fail failed"
(( fail > 0 )) && echo "failed: ${failed[*]}"
(( fail == 0 ))
