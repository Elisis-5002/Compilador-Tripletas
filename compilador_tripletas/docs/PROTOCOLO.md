# Protocolo de comunicación Java ↔ motor nativo

Documento de referencia del contrato que usan `NativeRunner` (Java) y
`native/motor.c` (C). El objetivo es que el motor pueda reimplementarse en otro
lenguaje, incluido ensamblador, sin cambiar la interfaz gráfica.

---

## 1. Invocación

```text
motor <operacion> [figura]
```

| Argumento | Descripción |
|---|---|
| `operacion` | `raiz`, `taylor`, `general`, `potenciacion`, `volumen`, `fuerza`, `trabajo`, `potencia`, `combinaciones` o `permutaciones` |
| `figura` | Solo para `volumen`: `cubo`, `caja`, `cilindro`, `esfera` o `cono` |

Ejemplos:

```text
motor raiz
motor taylor
motor volumen esfera
```

`NativeRunner` construye el comando con `java.lang.Runtime.exec(String[])`, por
lo que la ruta del ejecutable puede contener espacios y los argumentos no
requieren comillas manuales.

---

## 2. Entrada estándar (stdin)

Los datos se envían **una línea por dato**, en el mismo orden en que aparecen
en `com.calculadora.model.Catalogo` para la operación elegida. El motor lee
exactamente la cantidad de líneas que necesita.

Ejemplo para `taylor`:

```text
45
10
```

Reglas:

- Se acepta punto o coma como separador decimal (`45.5` o `45,5`).
- Los espacios iniciales y finales se ignoran.
- Los enteros se validan sin parte decimal.
- Si falta una línea, el motor responde con un error de entrada incompleta.

El motor escribe sus mensajes de ayuda en **stderr**; Java los drena en un hilo
aparte para que no se llenen los búferes del sistema operativo.

---

## 3. Salida estándar (stdout)

La salida es texto **UTF-8** con campos separados por **tabuladores** (`\t`) y
registros separados por `\n`.

### 3.1. Marcas de control

| Línea | Significado |
|---|---|
| `@@OP<TAB><titulo>` | Título de la operación (opcional, normalmente la primera línea) |
| `@@<SECCION><TAB><col1><TAB><col2>...` | Inicio de una tabla y sus columnas |
| `<dato1><TAB><dato2>...` | Fila de la tabla activa |
| `@@ERROR<TAB><mensaje>` | Error de validación o de cálculo |
| `@@FIN` | Fin normal de la salida |

Cualquier marca distinta de `OP`, `ERROR` y `FIN` se interpreta como una
sección. Esto permite agregar secciones nuevas sin cambiar el parser.

### 3.2. Secciones estándar

| Sección | Columnas | Uso |
|---|---|---|
| `TOKENS` | `#`, `Tipo`, `Lexema` | Lexemas de entrada y del operador |
| `SENTENCIAS` | `Sentencia`, `Descripcion` | Fórmula o algoritmo aplicado |
| `TRIPLETAS` | `#`, `Operador`, `Operando 1`, `Operando 2` | Código de tres direcciones |
| `PASOS` | Depende de la operación | Cálculo numérico paso a paso |
| `RESULTADOS` | `Variable`, `Valor`, `Unidad` | Valores finales |

### 3.3. Ejemplo completo (`taylor 45 10`, abreviado)

```text
@@OP	Seno (serie de Taylor)
@@TOKENS	#	Tipo	Lexema
1	NUMERO	45
2	NUMERO	10
3	OPERADOR	taylor
@@SENTENCIAS	Sentencia	Descripcion
x = angulo * PI / 180	Conversion de grados a radianes
suma = 0	Inicializacion del acumulador
@@TRIPLETAS	#	Operador	Operando 1	Operando 2
t1	pow	0.7853981634	1
t2	fact	1	-
t3	/	t1	t2
t4	*	1	t3
t5	+	0	t4
@@PASOS	n	Signo	x^(2n+1)	(2n+1)!	Termino	Suma acumulada
0	1	0.7853981634	1	0.7853981634	0.7853981634
1	-1	0.484473	6	-0.0807455122	0.7046526512
...
@@RESULTADOS	Variable	Valor	Unidad
Angulo original	45	grados
Serie de Taylor	0.7071067812	-
Referencia sin(x)	0.7071067812	-
Error absoluto	0.0000000000e+00	-
@@FIN
```

> Los valores exactos pueden variar ligeramente según la plataforma y el
> redondeo de `libm`, pero el formato es estable.

---

## 4. Errores y códigos de salida

Cuando la entrada no es válida, el motor emite:

```text
@@ERROR	El radicando debe ser mayor o igual que 0.
@@FIN
```

y termina con código `2`. `@@FIN` se emite también en los errores para que
Java pueda distinguir un error de validación (mensaje para el usuario) de una
caída inesperada del proceso (excepción de E/S).

Casos:

| Situación | Salida | Código |
|---|---|---|
| Cálculo correcto | secciones + `@@FIN` | 0 |
| Error de validación | `@@ERROR` + `@@FIN` | 2 |
| Argumentos de línea de comandos incorrectos | `@@ERROR` + `@@FIN` | 2 |
| Caída inesperada | sin `@@FIN` | distinto de 0 |

`NativeRunner` lanza `IOException` si no aparece `@@FIN`, si el proceso no
termina en 15 segundos o si no puede lanzarse.

---

## 5. Reglas del parser Java (`CalcOutput`)

1. Las líneas se leen como UTF-8.
2. Si una línea empieza por `@@`, se procesa como marca de control.
3. `@@OP` guarda el título; `@@ERROR` guarda el mensaje; `@@FIN` marca
   `completo = true`.
4. Cualquier otra marca crea una tabla nueva con sus columnas y la convierte
   en la sección activa.
5. Las líneas que no empiezan por `@@` se agregan como filas de la sección
   activa; las líneas vacías se ignoran.
6. Las filas se parten con `split("\t", -1)` para conservar campos vacíos al
   final.
7. La interfaz rellena con cadenas vacías las filas que tengan menos columnas
   que la cabecera, de modo que ninguna tabla provoca errores de índice.

---

## 6. Concurrencia y timeout

`NativeRunner` sigue este orden para evitar interbloqueos:

1. Lanza el proceso con `Runtime.exec`.
2. Inicia un hilo que drena `stderr`.
3. Inicia un hilo que lee `stdout` línea por línea.
4. Envía los datos por `stdin` y cierra el flujo.
5. Espera al proceso con un máximo de **15 segundos**.
6. Espera a que el hilo lector termine (máximo 2 segundos adicionales).
7. Interpreta la salida con `CalcOutput.parse`.
8. En `finally`, destruye el proceso si todavía está vivo.

El hilo lector se inicia antes de escribir en `stdin` para que un motor que
responda antes de consumir todos los datos no bloquee la escritura.

---

## 7. Cómo agregar una operación

1. Agregar la operación a `com.calculadora.model.Catalogo` con sus campos en
   el orden en que el motor los leerá.
2. Implementar la función correspondiente en `native/motor.c` y registrarla en
   `main`.
3. Emitir las secciones deseadas (`TOKENS`, `SENTENCIAS`, `TRIPLETAS`,
   `PASOS`, `RESULTADOS`) y terminar con `emitir_fin()`.
4. No es necesario modificar `MainFrame`: el formulario y las pestañas se
   generan dinámicamente.
