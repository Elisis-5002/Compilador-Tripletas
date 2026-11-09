#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# Pruebas de humo del motor nativo.
#
# Compila el motor si es necesario y ejecuta una batería de casos válidos e
# inválidos. Comprueba que todas las operaciones terminen con @@FIN y que los
# errores de validación emitan @@ERROR.
#
# Uso: ./scripts/probar_motor.sh            (compila antes)
#      ./scripts/probar_motor.sh --sin-compilar
# ---------------------------------------------------------------------------
set -euo pipefail

cd "$(dirname "$0")/.."

EJECUTABLE="native/motor"
if [ ! -x "$EJECUTABLE" ] && [ -x "native/motor.exe" ]; then
    # En Windows (Git Bash) el binario se llama motor.exe.
    EJECUTABLE="native/motor.exe"
fi

if [ "${1:-}" != "--sin-compilar" ]; then
    ./scripts/compilar_motor.sh
fi

if [ ! -x "$EJECUTABLE" ]; then
    echo "[ERROR] No existe native/motor ni native/motor.exe." >&2
    echo "Ejecute ./scripts/compilar_motor.sh (Linux/macOS) o scripts\\compilar_motor.bat (Windows)." >&2
    exit 1
fi

fallos=0

# probar <descripcion> <entrada> <argumentos...>
probar() {
    local descripcion="$1"
    local entrada="$2"
    shift 2
    local salida
    salida=$(printf '%s\n' "$entrada" | "$EJECUTABLE" "$@" 2>/dev/null || true)
    if ! grep -q '^@@FIN$' <<<"$salida"; then
        echo "[FALLO] $descripcion: no se emitio @@FIN"
        fallos=$((fallos + 1))
        return
    fi
    echo "[OK] $descripcion"
}

# probar_error <descripcion> <entrada> <argumentos...>
probar_error() {
    local descripcion="$1"
    local entrada="$2"
    shift 2
    local salida
    salida=$(printf '%s\n' "$entrada" | "$EJECUTABLE" "$@" 2>/dev/null || true)
    if ! grep -q '^@@ERROR' <<<"$salida"; then
        echo "[FALLO] $descripcion: se esperaba @@ERROR"
        fallos=$((fallos + 1))
        return
    fi
    echo "[OK] $descripcion"
}

# probar_contiene <descripcion> <texto esperado> <entrada> <argumentos...>
probar_contiene() {
    local descripcion="$1"
    local esperado="$2"
    local entrada="$3"
    shift 3
    local salida
    salida=$(printf '%s\n' "$entrada" | "$EJECUTABLE" "$@" 2>/dev/null || true)
    if ! grep -qF -- "$esperado" <<<"$salida"; then
        echo "[FALLO] $descripcion: no se encontro '$esperado'"
        fallos=$((fallos + 1))
        return
    fi
    echo "[OK] $descripcion"
}

# ------------------------- Casos válidos -------------------------
probar_contiene "raiz(9) = 3" "Raiz cuadrada	3	unidades" "9" raiz
probar_contiene "taylor: 10 pasos" "9	-1	" $'45\n10' taylor
probar_contiene "taylor: seccion de resultados" "Serie de Taylor" $'45\n10' taylor
probar_contiene "cuadratica con dos raices" "x1" $'1\n-3\n2' general
probar "cuadratica con raices complejas" $'1\n0\n1' general
probar_contiene "potenciacion 2^10 = 1024" "1024" $'2\n10' potenciacion
probar_contiene "volumen cubo 3 = 27" "27" "3" volumen cubo
probar_contiene "volumen caja 2x3x4 = 24" "24" $'2\n3\n4' volumen caja
probar "volumen cilindro" $'1\n1' volumen cilindro
probar "volumen esfera" "1" volumen esfera
probar "volumen cono" $'1\n1' volumen cono
probar_contiene "fuerza 10*2 = 20" "20" $'10\n2' fuerza
probar_contiene "trabajo 10*5*cos(0) = 50" "50" $'10\n5\n0' trabajo
probar_contiene "potencia 100/2 = 50" "50" $'100\n2' potencia
probar_contiene "combinaciones C(5,2) = 10" "10" $'5\n2' combinaciones
probar_contiene "permutaciones P(5,2) = 20" "20" $'5\n2' permutaciones

# ------------------------- Casos inválidos -------------------------
probar_error "raiz negativa" "-1" raiz
probar_error "taylor con 0 terminos" $'45\n0' taylor
probar_error "cuadratica con a = 0" $'0\n1\n1' general
probar_error "combinaciones con r > n" $'5\n6' combinaciones
probar_error "operacion desconocida" "1" desconocida

echo
if [ "$fallos" -ne 0 ]; then
    echo "Resultado: $fallos prueba(s) fallaron." >&2
    exit 1
fi
echo "Resultado: todas las pruebas pasaron."
