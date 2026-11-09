#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# Compila el proyecto Java con javac y genera un JAR ejecutable.
# Uso: ./scripts/compilar_java.sh
# ---------------------------------------------------------------------------
set -euo pipefail

cd "$(dirname "$0")/.."

if ! command -v javac >/dev/null 2>&1; then
    echo "[ERROR] javac no está en el PATH. Instale un JDK 17 o superior." >&2
    exit 1
fi

rm -rf target/classes
mkdir -p target/classes

find src/main/java -name '*.java' -print0 \
    | xargs -0 javac -encoding UTF-8 -d target/classes

jar --create --file target/CalculadoraTripletas.jar \
    --main-class com.calculadora.Main \
    -C target/classes .

echo "[OK] JAR generado en target/CalculadoraTripletas.jar"
