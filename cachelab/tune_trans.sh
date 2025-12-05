#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

TRANS_FILE="trans.c"
BACKUP_FILE="${TRANS_FILE}.tune.bak"

cp "$TRANS_FILE" "$BACKUP_FILE"
restore_original() {
    mv "$BACKUP_FILE" "$TRANS_FILE"
}
trap restore_original EXIT

best_misses=""
best_length=""
best_width=""

for length in {4..36}; do
    for width in {4..36}; do
        sed -i "72s/.*/        int length = $length;/" "$TRANS_FILE"
        sed -i "73s/.*/        int width = $width;/" "$TRANS_FILE"

        echo "[info] Rebuilding for length=$length width=$width"
        make >/dev/null

        echo "[info] Running test-trans for length=$length width=$width"
        test_output=$(./test-trans -M 61 -N 67)
        echo "$test_output"

        result_line=$(printf '%s
' "$test_output" | tail -n 1)
        misses=${result_line##*:}

        if ! [[ $misses =~ ^[0-9]+$ ]]; then
            echo "[warn] Unable to parse miss count from: $result_line" >&2
            continue
        fi

        echo "[info] length=$length width=$width misses=$misses"

        if [[ -z $best_misses || $misses -lt $best_misses ]]; then
            best_misses=$misses
            best_length=$length
            best_width=$width
            echo "[info] New best found: length=$length width=$width misses=$misses"
        fi
    done
done

printf '\nBest combo => length=%s width=%s misses=%s\n' "$best_length" "$best_width" "$best_misses"
