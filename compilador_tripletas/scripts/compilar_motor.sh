#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# Compila el motor nativo en C para Linux o macOS usando GCC.
# Uso: ./scripts/compilar_motor.sh
# ---------------------------------------------------------------------------
set -euo pipefail

cd "$(dirname "$0")/.."

if ! command -v gcc >/dev/null 2>&1; then
    echo "[ERROR] gcc no está en el PATH." >&2
    echo "Instale build-essential (Debian/Ubuntu) o Xcode Command Line Tools (macOS)." >&2
    exit 1
fi

if [ ! -d native ]; then
    echo "[ERROR] No existe la carpeta native." >&2
    exit 1
fi

echo "Compilando native/motor.c ..."
gcc -O2 -Wall -Wextra -Wpedantic -std=c11 -o native/motor native/motor.c -lm
echo "[OK] Motor compilado en native/motor"
