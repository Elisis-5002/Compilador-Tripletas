# Calculadora con tokens, sentencias y tripletas

Aplicación de escritorio desarrollada con **Java 17 + Swing** que invoca un
**motor nativo escrito en C** mediante `java.lang.Runtime`. El motor calcula la
operación seleccionada y devuelve tablas de **tokens**, **sentencias**,
**tripletas**, **pasos** y **resultados**, que la interfaz muestra en un grid
con una pestaña por sección.

Incluye la **serie de Taylor del seno** con visualización paso a paso, tal como
pide la asignación: el usuario introduce el ángulo y el número de términos, el
motor calcula cada término y la interfaz muestra una fila por iteración
(`n`, signo, potencia, factorial, término y suma acumulada).

![Captura de la aplicación con la serie de Taylor](docs/captura_ui.png)

---

## 1. Cumplimiento de la asignación

| Requisito | Implementación |
|---|---|
| Aplicación Java en NetBeans 21 | Proyecto Maven (`pom.xml`) listo para abrir en NetBeans 21 |
| Usar `Runtime` para invocar código nativo | `com.calculadora.backend.NativeRunner` usa `Runtime.getRuntime().exec(...)` |
| Función en ensamblador o C | `native/motor.c`, compilado a `motor.exe` / `motor` |
| Calcular la serie de Taylor del seno | Operación `taylor` del motor |
| Ambiente gráfico | Ventana Swing con árbol de operaciones y formularios |
| Grid con resultados paso a paso | Pestaña **Paso a paso** (`JTable`) con una fila por término |
| Buenas prácticas | Separación por capas, Javadoc, validación, hilos de fondo, scripts |
| Documentación exhaustiva | Este README, `docs/PROTOCOLO.md` y Javadoc en todo el código |
| Proyecto descargable en ZIP | `CalculadoraTripletas.zip` en la raíz del repositorio |

---

## 2. Requisitos

- **JDK 17 o superior** (el `pom.xml` compila con `release 17`).
- **NetBeans 21** (opcional; también se puede compilar con scripts).
- **GCC / MinGW-w64** para compilar el motor nativo:
  - Windows: MinGW-w64 con `gcc` en el `PATH`.
  - Linux: `gcc` y `libm` (`build-essential` en Debian/Ubuntu).
  - macOS: Xcode Command Line Tools.
- No se requieren dependencias externas de Java: todo usa la biblioteca
  estándar (`Swing`, `java.io`, `java.nio`).

> **Nota sobre Java 21:** el proyecto usa Java 17 para ser compatible con el
> JDK instalado en el equipo de desarrollo. NetBeans 21 funciona con JDK 17 o
> superior. Si desea compilar con JDK 21, cambie
> `<maven.compiler.release>17</maven.compiler.release>` por `21` en `pom.xml`.

---

## 3. Estructura del proyecto

```text
compilador_tripletas/
├── pom.xml
├── README.md
├── docs/
│   └── PROTOCOLO.md
├── native/
│   └── motor.c                 Motor nativo en C (14 operaciones)
├── scripts/
│   ├── compilar_motor.bat      Compila motor.exe con MinGW
│   ├── compilar_motor.sh       Compila motor con GCC
│   ├── compilar_java.bat       Compila el proyecto con javac y crea el JAR
│   ├── compilar_java.sh
│   ├── compilar_todo.bat       Compila motor + Java en un paso
│   ├── compilar_todo.sh
│   ├── ejecutar.bat            Compila si hace falta y ejecuta la app
│   ├── ejecutar.sh
│   └── probar_motor.sh         Pruebas de humo del motor nativo
└── src/main/java/com/calculadora/
    ├── Main.java               Punto de entrada Swing
    ├── backend/
    │   ├── CalcOutput.java     Parser del protocolo del motor
    │   ├── ExeLocator.java     Localiza motor.exe / motor
    │   └── NativeRunner.java   Invocación con Runtime y timeout
    ├── gui/
    │   ├── MainFrame.java      Ventana principal y formulario dinámico
    │   └── ResultadosPanel.java Grid por pestañas
    └── model/
        ├── Campo.java          Dato de entrada
        ├── Catalogo.java       Catálogo de las 14 operaciones
        └── Operacion.java      Metadatos de una operación
```

---

## 4. Compilación y ejecución

### 4.1. Compilar el motor nativo

**Windows (MinGW-w64):**

```bat
cd compilador_tripletas
scripts\compilar_motor.bat
```

o directamente:

```bat
gcc -O2 -Wall -Wextra -std=c11 -o native\motor.exe native\motor.c
```

**Linux / macOS:**

```bash
cd compilador_tripletas
./scripts/compilar_motor.sh
```

o directamente:

```bash
gcc -O2 -Wall -Wextra -std=c11 -o native/motor native/motor.c -lm
```

El ejecutable debe quedar en `native/motor.exe` (Windows) o `native/motor`
(Linux/macOS). `ExeLocator` lo busca en esa carpeta y en las carpetas
superiores del directorio de trabajo.

### 4.2. Abrir en NetBeans 21

1. `File → Open Project…`
2. Seleccione la carpeta `compilador_tripletas` (la que contiene `pom.xml`).
3. NetBeans reconocerá el proyecto Maven y descargará el plugin del compilador
   si aún no lo tiene.
4. Compile el motor nativo (paso 4.1).
5. Ejecute el proyecto con `Run → Run Project` (F6). La clase principal es
   `com.calculadora.Main`.

### 4.3. Compilar y ejecutar sin NetBeans ni Maven

Para compilar el motor y el proyecto Java de una sola vez:

```bat
scripts\compilar_todo.bat
scripts\ejecutar.bat
```

```bash
./scripts/compilar_todo.sh
./scripts/ejecutar.sh
```

También puede compilar solo el Java con `scripts/compilar_java.*`.

El JAR resultante es `target/CalculadoraTripletas.jar` y puede ejecutarse con:

```bash
java -jar target/CalculadoraTripletas.jar
```

> La aplicación no incluye el binario nativo dentro del JAR. Debe compilarlo
> una vez en `native/` antes de calcular.

---

## 5. Uso de la aplicación

1. Seleccione una operación en el árbol de la izquierda (agrupadas por
   Matemáticas, Geometría, Física y Combinatoria).
2. Complete los campos del formulario. Los datos se envían al motor, que es
   quien los valida; si hay un error, la interfaz muestra el mensaje exacto.
3. Presione **Calcular**.
4. Revise las pestañas del grid:
   - **Paso a paso:** cálculo numérico fila por fila (en Taylor, una fila por
     término). Es la pestaña que se abre por defecto.
   - **Resultados:** valores finales con su unidad.
   - **Tripletas:** representación de tres direcciones de las operaciones.
   - **Sentencias:** fórmula o algoritmo aplicado.
   - **Tokens:** lexemas de entrada y del operador.

### Ejemplo: serie de Taylor del seno

Entrada:

```text
Ángulo: 45
Términos: 10
```

La pestaña **Paso a paso** muestra filas como:

```text
n   Signo   x^(2n+1)     (2n+1)!   Termino       Suma acumulada
0   1       0.7853981634 1.000000  0.7853981634  0.7853981634
1   -1      0.484473   6.000000    -0.0807455122 0.7046526512
2   1       0.298858   120.000000  0.0024903946  0.7071430458
...
```

Al final, **Resultados** compara la serie con `sin(x)` de `math.h` e incluye el
error absoluto.

---

## 6. Arquitectura

```text
┌──────────────────────────────┐
│ MainFrame (Swing, EDT)       │
│  ├── árbol de operaciones    │
│  ├── formulario dinámico     │
│  └── ResultadosPanel (grid)  │
└──────────────┬───────────────┘
               │ SwingWorker
               ▼
┌──────────────────────────────┐
│ NativeRunner                 │
│  Runtime.getRuntime().exec   │
│  stdin: datos del usuario    │
│  stdout: protocolo @@...     │
└──────────────┬───────────────┘
               │ proceso nativo
               ▼
┌──────────────────────────────┐
│ native/motor.c               │
│  calcula la operación        │
│  emite TOKENS, SENTENCIAS,   │
│  TRIPLETAS, PASOS,           │
│  RESULTADOS y @@FIN          │
└──────────────────────────────┘
               │
               ▼
┌──────────────────────────────┐
│ CalcOutput.parse(...)        │
│  Map<String, Tabla>          │
└──────────────────────────────┘
```

La interfaz **no** realiza los cálculos: solo captura datos y presenta las
tablas que devuelve el proceso nativo. Esto mantiene separadas las
responsabilidades y permite cambiar el motor (por ejemplo, por una versión en
ensamblador) sin tocar Swing.

---

## 7. Operaciones implementadas

| Categoría | Operación | Argumentos del motor | Entradas |
|---|---|---|---|
| Matemáticas | Raíz cuadrada | `raiz` | radicando ≥ 0 |
| Matemáticas | Seno (serie de Taylor) | `taylor` | ángulo en grados, términos 1–20 |
| Matemáticas | Fórmula general | `general` | a ≠ 0, b, c |
| Matemáticas | Potenciación | `potenciacion` | base x, exponente n |
| Geometría | Volumen — cubo | `volumen cubo` | arista a ≥ 0 |
| Geometría | Volumen — caja | `volumen caja` | largo, ancho, alto ≥ 0 |
| Geometría | Volumen — cilindro | `volumen cilindro` | radio, altura ≥ 0 |
| Geometría | Volumen — esfera | `volumen esfera` | radio ≥ 0 |
| Geometría | Volumen — cono | `volumen cono` | radio, altura ≥ 0 |
| Física | Fuerza | `fuerza` | masa > 0, aceleración |
| Física | Trabajo | `trabajo` | fuerza ≥ 0, distancia ≥ 0, ángulo |
| Física | Potencia | `potencia` | trabajo, tiempo > 0 |
| Combinatoria | Combinaciones | `combinaciones` | n entero 0–1000, 0 ≤ r ≤ n |
| Combinatoria | Permutaciones | `permutaciones` | n entero 0–1000, 0 ≤ r ≤ n |

La lista exacta de campos y ayudas se define en
`com.calculadora.model.Catalogo`; el formulario se construye a partir de esa
lista, por lo que agregar una operación nueva requiere tocar el catálogo y el
motor, no la interfaz.

### Serie de Taylor

La fórmula implementada es:

```text
sen(x) = Σ_{n=0}^{N-1} (-1)^n · x^(2n+1) / (2n+1)!
```

Detalles de implementación:

- El ángulo se convierte de grados a radianes.
- Se normaliza a `(-180, 180]` con `fmod` para mejorar la convergencia en
  ángulos grandes.
- El factorial se calcula como `double` para evitar desbordamiento de enteros
  (hasta `41!` con 20 términos).
- Se emite una fila de **PASOS** por cada término y una **tripleta** por cada
  operación intermedia (`pow`, `fact`, `/`, `*`, `+`).
- Se compara el resultado con `sin(x)` de `math.h` y se reporta el error
  absoluto.

---

## 8. Protocolo Java ↔ C

El contrato completo está en [`docs/PROTOCOLO.md`](docs/PROTOCOLO.md). Resumen:

- El motor recibe `motor <operacion> [figura]`.
- Los datos llegan por **stdin**, una línea por dato, en el orden del catálogo.
- La salida va por **stdout** en UTF-8 y con tabuladores como separador.
- Cada sección empieza con `@@NOMBRE` y sus columnas.
- `@@ERROR` reporta validaciones y `@@FIN` marca el final correcto.
- Los mensajes de ayuda del motor van por **stderr** y Java los drena en un
  hilo aparte para evitar bloqueos.

---

## 9. Solución de problemas

| Síntoma | Causa / solución |
|---|---|
| «No se encontró el ejecutable nativo» | Compile `native/motor.c` con los scripts o con `gcc`. Verifique que el archivo esté en `native/`. |
| `gcc` no se reconoce | Instale MinGW-w64 y agregue `C:\mingw64\bin` al `PATH`; cierre y abra la terminal. |
| `mvn` no se reconoce | Use NetBeans 21 o los scripts `compilar_java.*`; Maven no es obligatorio para ejecutar. |
| El motor devuelve un error de validación | Revise los límites de cada campo en la tabla de operaciones; el mensaje del motor indica la causa. |
| La ventana se congela | No debería: el motor se ejecuta en un `SwingWorker`. Si ocurre, revise que el binario nativo no esté esperando datos. |
| Acentos raros en la consola del motor | El protocolo usa texto ASCII en el motor para máxima portabilidad; la interfaz Java muestra los textos con acentos. |
| El antivirus bloquea `motor.exe` | Es un ejecutable compilado localmente; puede compilarlo de nuevo o excluirlo. |

---

## 10. Buenas prácticas aplicadas

- **Separación por capas:** modelo (`Catalogo`), backend (`NativeRunner`,
  `CalcOutput`, `ExeLocator`) e interfaz (`MainFrame`, `ResultadosPanel`).
- **Interfaz responsiva:** los cálculos se ejecutan en `SwingWorker`; la EDT
  solo actualiza componentes Swing.
- **Validación en el motor:** una única fuente de verdad para los límites de
  cada operación; Java muestra el mensaje sin duplicar reglas.
- **Timeout y drenaje de flujos:** `NativeRunner` evita interbloqueos y limita
  la espera a 15 segundos.
- **Formato y codificación:** UTF-8 en Java, `Locale` independiente en C
  (punto decimal), separación por tabuladores.
- **Documentación:** Javadoc en clases y métodos públicos, README y documento
  de protocolo.
- **Scripts reproducibles:** compilación del motor y del proyecto Java sin
  depender de un IDE.
- **Pruebas de humo:** `scripts/probar_motor.sh` verifica las 14 operaciones y
  varios errores de validación.

---

## 11. Entregable

El archivo `CalculadoraTripletas.zip` (en la raíz del repositorio) contiene el
proyecto completo: fuentes Java, motor en C, scripts, documentación y `pom.xml`.

Además incluye dos binarios de conveniencia:

- `native/motor.exe`: motor ya compilado para Windows x64.
- `dist/CalculadoraTripletas.jar`: aplicación Java ya compilada.

Para usarlo:

1. Descomprima el ZIP.
2. Ejecute `java -jar dist/CalculadoraTripletas.jar` desde la carpeta del
   proyecto, o abra la carpeta en NetBeans 21.
3. Si va a modificar el motor, recompile con `scripts/compilar_motor.*`; si
   modifica el Java, recompile con `scripts/compilar_java.*`.

En Linux o macOS debe recompilar el motor (`./scripts/compilar_motor.sh`),
porque el `motor.exe` incluido es para Windows.
