#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# Compila el motor nativo y el proyecto Java en un solo paso.
# Uso: ./scripts/compilar_todo.sh
# ---------------------------------------------------------------------------
set -euo pipefail

cd "$(dirname "$0")/.."

./scripts/compilar_motor.sh
./scripts/compilar_java.sh

echo
echo "[OK] Proyecto listo. Ejecute ./scripts/ejecutar.sh"
