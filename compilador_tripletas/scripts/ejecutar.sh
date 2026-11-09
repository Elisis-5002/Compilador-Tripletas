#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# Compila (si hace falta) y ejecuta la aplicación.
# Requiere que native/motor exista.
# Uso: ./scripts/ejecutar.sh
# ---------------------------------------------------------------------------
set -euo pipefail

cd "$(dirname "$0")/.."

if [ ! -f target/CalculadoraTripletas.jar ]; then
    ./scripts/compilar_java.sh
fi

if [ ! -f native/motor ]; then
    echo "[AVISO] No existe native/motor. Ejecute ./scripts/compilar_motor.sh" >&2
fi

java -jar target/CalculadoraTripletas.jar
