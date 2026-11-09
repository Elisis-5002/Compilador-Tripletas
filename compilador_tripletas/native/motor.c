/**
 * @file motor.c
 * @brief Motor de cálculo nativo de la calculadora (tokens, sentencias y tripletas).
 *
 * @details
 * Este programa es invocado por la aplicación Java mediante
 * {@code java.lang.Runtime}. Implementa las catorce operaciones declaradas en
 * {@code com.calculadora.model.Catalogo} y devuelve los resultados como tablas
 * de texto separadas por tabuladores, de modo que Swing pueda mostrarlas en un
 * grid sin acoplarse a la lógica de cálculo.
 *
 * Protocolo de salida (una línea por registro):
 * @code
 *   @@OP       <titulo de la operacion>
 *   @@SECCION  <col1> <col2> ...      cabecera de una tabla (TAB entre campos)
 *   <dato1>    <dato2> ...            filas de la tabla
 *   @@ERROR    <mensaje>              error de entrada o de calculo
 *   @@FIN                             fin normal de la salida
 * @endcode
 *
 * Las secciones que puede emitir el motor son:
 *   - TOKENS:      lexemas de entrada y del operador.
 *   - SENTENCIAS:  fórmula o algoritmo aplicado.
 *   - TRIPLETAS:   representación de tres direcciones de los pasos.
 *   - PASOS:       cálculo numérico paso a paso para el grid.
 *   - RESULTADOS:  valores finales con su unidad.
 *
 * Invocación:
 * @code
 *   motor <operacion> [figura]
 * @endcode
 *
 * Los datos se leen de la entrada estándar, una línea por dato y en el mismo
 * orden en que los declara el catálogo Java. Los mensajes de ayuda se escriben
 * en la salida de error para no contaminar el protocolo.
 *
 * Compilación:
 * @code
 *   Windows:      gcc -O2 -Wall -Wextra -std=c11 -o native/motor.exe native/motor.c
 *   Linux/macOS:  gcc -O2 -Wall -Wextra -std=c11 -o native/motor native/motor.c -lm
 * @endcode
 *
 * @author Proyecto Calculadora de Tripletas
 * @version 1.0
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
/** Valor de pi si el compilador no lo define en math.h. */
#define M_PI 3.14159265358979323846
#endif

/** Tamaño máximo de una línea de entrada. */
#define MAX_LINEA 256

/** Número máximo de términos de la serie de Taylor. */
#define MAX_TERMINOS 20

/** Límite práctico para los cálculos de combinatoria. */
#define MAX_COMBINATORIA 1000

/* =========================================================================
 * 1. Utilidades del protocolo de salida
 * ========================================================================= */

/** Escribe el título de la operación que se está calculando. */
static void emitir_op(const char *titulo) {
    printf("@@OP\t%s\n", titulo);
}

/**
 * Escribe la cabecera de una sección.
 *
 * @param nombre    nombre de la sección (TOKENS, PASOS, ...)
 * @param columnas  nombres de columna separados por tabuladores
 */
static void emitir_seccion(const char *nombre, const char *columnas) {
    printf("@@%s\t%s\n", nombre, columnas);
}

/** Escribe la marca de fin de salida y vacía el búfer. */
static void emitir_fin(void) {
    printf("@@FIN\n");
    fflush(stdout);
}

/**
 * Emite un error de validación o de cálculo y termina la salida de forma
 * ordenada para que Java pueda mostrar el mensaje al usuario.
 *
 * @return siempre 2, para que las funciones puedan hacer {@code return fallar(...);}
 */
static int fallar(const char *mensaje) {
    printf("@@ERROR\t%s\n", mensaje);
    emitir_fin();
    return 2;
}

/** Escribe una fila de la sección SENTENCIAS. */
static void emitir_sentencia(const char *texto, const char *descripcion) {
    printf("%s\t%s\n", texto, descripcion);
}

/** Contador de tokens de la operación actual. */
static int contador_tokens = 0;

/** Escribe una fila de la sección TOKENS. */
static void emitir_token(const char *tipo, const char *lexema) {
    printf("%d\t%s\t%s\n", ++contador_tokens, tipo, lexema);
}

/** Escribe un token numérico usando el formato compacto de la calculadora. */
static void emitir_token_numero(double valor) {
    char buffer[64];
    snprintf(buffer, sizeof buffer, "%.10g", valor);
    emitir_token("NUMERO", buffer);
}

/** Contador de variables temporales de las tripletas. */
static int contador_temporales = 0;

/**
 * Escribe una tripleta y devuelve el nombre de su resultado temporal.
 *
 * @param operador   operación de tres direcciones (pow, fact, *, /, ...)
 * @param operando1  primer operando
 * @param operando2  segundo operando, o "-" si es unaria
 * @param resultado  búfer donde se escribe el nombre del temporal (t1, t2, ...)
 * @param tam        tamaño del búfer de resultado
 */
static void emitir_tripleta(const char *operador, const char *operando1,
                            const char *operando2, char *resultado, size_t tam) {
    contador_temporales++;
    snprintf(resultado, tam, "t%d", contador_temporales);
    printf("%s\t%s\t%s\t%s\n", resultado, operador, operando1, operando2);
}

/** Escribe una fila de la sección RESULTADOS. */
static void emitir_resultado(const char *variable, const char *valor, const char *unidad) {
    printf("%s\t%s\t%s\n", variable, valor, unidad);
}

/** Escribe un resultado numérico con formato compacto (10 cifras significativas). */
static void emitir_resultado_numero(const char *variable, double valor, const char *unidad) {
    char buffer[64];
    if (valor == 0.0) {
        valor = 0.0; // evita mostrar "-0"
    }
    snprintf(buffer, sizeof buffer, "%.10g", valor);
    emitir_resultado(variable, buffer, unidad);
}

/** Escribe un resultado numérico con 10 decimales fijos. */
static void emitir_resultado_fijo(const char *variable, double valor, const char *unidad) {
    char buffer[64];
    if (valor == 0.0) {
        valor = 0.0; // evita mostrar "-0.0000000000"
    }
    snprintf(buffer, sizeof buffer, "%.10f", valor);
    emitir_resultado(variable, buffer, unidad);
}

/* Cabeceras de sección reutilizadas por todas las operaciones. */
static void seccion_tokens(void) {
    emitir_seccion("TOKENS", "#\tTipo\tLexema");
}

static void seccion_sentencias(void) {
    emitir_seccion("SENTENCIAS", "Sentencia\tDescripcion");
}

static void seccion_tripletas(void) {
    emitir_seccion("TRIPLETAS", "#\tOperador\tOperando 1\tOperando 2");
}

static void seccion_pasos_generico(void) {
    emitir_seccion("PASOS", "Paso\tDescripcion\tResultado");
}

static void seccion_resultados(void) {
    emitir_seccion("RESULTADOS", "Variable\tValor\tUnidad");
}

/* =========================================================================
 * 2. Lectura y validación de datos
 * ========================================================================= */

/**
 * Lee una línea de la entrada estándar y elimina los saltos de línea.
 *
 * @return 1 si se leyó una línea; 0 al llegar al final de la entrada
 */
static int leer_linea(char *destino, size_t tam) {
    if (fgets(destino, (int) tam, stdin) == NULL) {
        return 0;
    }
    size_t n = strlen(destino);
    while (n > 0 && (destino[n - 1] == '\n' || destino[n - 1] == '\r')) {
        destino[--n] = '\0';
    }
    return 1;
}

/** Elimina los espacios iniciales y finales de una cadena (modifica el búfer). */
static char *recortar(char *texto) {
    while (isspace((unsigned char) *texto)) {
        texto++;
    }
    char *fin = texto + strlen(texto);
    while (fin > texto && isspace((unsigned char) fin[-1])) {
        fin--;
    }
    *fin = '\0';
    return texto;
}

/**
 * Convierte texto a double aceptando punto o coma decimal y rechazando
 * caracteres sobrantes, NaN e infinito.
 *
 * @return 1 si la conversión fue válida; 0 en caso contrario
 */
static int interpretar_double(const char *texto, double *valor) {
    char limpio[MAX_LINEA * 2];
    strncpy(limpio, texto, sizeof limpio - 1);
    limpio[sizeof limpio - 1] = '\0';

    for (char *p = limpio; *p != '\0'; p++) {
        if (*p == ',') {
            *p = '.';
        }
    }

    errno = 0;
    char *fin = NULL;
    double convertido = strtod(limpio, &fin);
    if (fin == limpio) {
        return 0;
    }
    while (isspace((unsigned char) *fin)) {
        fin++;
    }
    if (*fin != '\0' || errno == ERANGE || !isfinite(convertido)) {
        return 0;
    }
    *valor = convertido;
    return 1;
}

/**
 * Escribe un mensaje de ayuda en stderr y lee un número desde stdin.
 *
 * @return 1 si se leyó bien, 0 si no había datos, -1 si el dato es inválido
 */
static int leer_double(const char *prompt, double *valor) {
    char linea[MAX_LINEA];
    fprintf(stderr, "%s: ", prompt);
    fflush(stderr);
    if (!leer_linea(linea, sizeof linea)) {
        return 0;
    }
    return interpretar_double(recortar(linea), valor) ? 1 : -1;
}

/**
 * Lee un número entero (sin parte decimal).
 *
 * @return 1 si es válido, 0 si no había datos, -1 si no es entero
 */
static int leer_entero(const char *prompt, long *valor) {
    double numero = 0.0;
    int estado = leer_double(prompt, &numero);
    if (estado != 1) {
        return estado;
    }
    if (numero != floor(numero) || numero < (double) LONG_MIN || numero > (double) LONG_MAX) {
        return -1;
    }
    *valor = (long) numero;
    return 1;
}

/**
 * Variante de {@link leer_double} que emite el error del protocolo y devuelve
 * 2 si el dato falta o es inválido. El llamador solo debe hacer
 * {@code if (leer_double_o_fallar(...)) return 2;}.
 */
static int leer_double_o_fallar(const char *prompt, const char *mensaje, double *valor) {
    int estado = leer_double(prompt, valor);
    if (estado == 1) {
        return 0;
    }
    return fallar(estado == 0 ? "Entrada incompleta: faltan datos." : mensaje);
}

/** Variante de {@link leer_entero} con emisión automática del error. */
static int leer_entero_o_fallar(const char *prompt, const char *mensaje, long *valor) {
    int estado = leer_entero(prompt, valor);
    if (estado == 1) {
        return 0;
    }
    return fallar(estado == 0 ? "Entrada incompleta: faltan datos." : mensaje);
}

/* =========================================================================
 * 3. Utilidades matemáticas y de formato
 * ========================================================================= */

/** Formatea un número con 10 cifras significativas y punto decimal. */
static void formato_numero(double valor, char *destino, size_t tam) {
    if (valor == 0.0) {
        valor = 0.0; // normaliza el cero negativo (-0.0) a 0.0
    }
    snprintf(destino, tam, "%.10g", valor);
}

/**
 * Formatea un resultado entero grande: sin decimales si cabe exactamente en
 * un double; en notación científica si supera 10^15.
 */
static void formato_entero_grande(double valor, char *destino, size_t tam) {
    if (fabs(valor) < 1e15) {
        snprintf(destino, tam, "%.0f", valor);
    } else {
        snprintf(destino, tam, "%.10g", valor);
    }
}

/** Calcula n! como double para evitar desbordamiento de enteros. */
static double factorial_double(int n) {
    double factorial = 1.0;
    for (int i = 2; i <= n; i++) {
        factorial *= (double) i;
    }
    return factorial;
}

/* =========================================================================
 * 4. Operaciones
 * ========================================================================= */

/**
 * Raíz cuadrada.
 * Entrada: radicando x >= 0.
 */
static int op_raiz(void) {
    emitir_op("Raiz cuadrada");

    double radicando = 0.0;
    if (leer_double_o_fallar("Radicando x", "El radicando debe ser un numero valido.", &radicando)) {
        return 2;
    }
    if (radicando < 0.0) {
        return fallar("El radicando debe ser mayor o igual que 0.");
    }

    double raiz = sqrt(radicando);
    char xbuf[64];
    char rbuf[64];
    char sentencia[128];
    formato_numero(radicando, xbuf, sizeof xbuf);
    formato_numero(raiz, rbuf, sizeof rbuf);
    snprintf(sentencia, sizeof sentencia, "x = %s", xbuf);

    seccion_tokens();
    emitir_token("NUMERO", xbuf);
    emitir_token("OPERADOR", "sqrt");

    seccion_sentencias();
    emitir_sentencia("R = sqrt(x)", "Raiz cuadrada del radicando");
    emitir_sentencia(sentencia, "Sustitucion del dato de entrada");

    seccion_tripletas();
    char t1[16];
    emitir_tripleta("sqrt", xbuf, "-", t1, sizeof t1);

    seccion_pasos_generico();
    printf("1\tsqrt(%s)\t%s\n", xbuf, rbuf);

    seccion_resultados();
    emitir_resultado("Raiz cuadrada", rbuf, "unidades");
    emitir_resultado("Radicando", xbuf, "unidades");

    emitir_fin();
    return 0;
}

/**
 * Seno mediante la serie de Taylor.
 *
 * sin(x) = sum_{n=0}^{N-1} (-1)^n * x^(2n+1) / (2n+1)!
 *
 * Entrada: ángulo en grados y número de términos (1..20).
 * El ángulo se normaliza a (-180, 180] para mejorar la convergencia.
 */
static int op_taylor(void) {
    emitir_op("Seno (serie de Taylor)");

    double angulo = 0.0;
    long terminos_largo = 0;
    if (leer_double_o_fallar("Angulo en grados", "El angulo debe ser un numero valido.", &angulo)) {
        return 2;
    }
    if (leer_entero_o_fallar("Numero de terminos n", "El numero de terminos debe ser un entero valido.",
                             &terminos_largo)) {
        return 2;
    }
    if (terminos_largo < 1 || terminos_largo > MAX_TERMINOS) {
        return fallar("El numero de terminos debe estar entre 1 y 20.");
    }

    const int terminos = (int) terminos_largo;

    // Normalización del ángulo para que la serie converja con pocos términos.
    double normalizado = fmod(angulo, 360.0);
    if (normalizado > 180.0) {
        normalizado -= 360.0;
    } else if (normalizado < -180.0) {
        normalizado += 360.0;
    }
    const double x = normalizado * M_PI / 180.0;

    double signo[MAX_TERMINOS];
    double potencia[MAX_TERMINOS];
    double factorial[MAX_TERMINOS];
    double termino[MAX_TERMINOS];
    double acumulada[MAX_TERMINOS];

    double suma = 0.0;
    for (int n = 0; n < terminos; n++) {
        const int exponente = 2 * n + 1;
        signo[n] = (n % 2 == 0) ? 1.0 : -1.0;
        potencia[n] = pow(x, exponente);
        factorial[n] = factorial_double(exponente);
        termino[n] = signo[n] * potencia[n] / factorial[n];
        suma += termino[n];
        acumulada[n] = suma;
    }

    char xbuf[64];
    char angbuf[64];
    char normbuf[64];
    char termbuf[32];
    formato_numero(x, xbuf, sizeof xbuf);
    formato_numero(angulo, angbuf, sizeof angbuf);
    formato_numero(normalizado, normbuf, sizeof normbuf);
    snprintf(termbuf, sizeof termbuf, "%d", terminos);

    seccion_tokens();
    emitir_token("NUMERO", angbuf);
    emitir_token("NUMERO", termbuf);
    emitir_token("OPERADOR", "taylor");
    emitir_token("OPERADOR", "*");
    emitir_token("NUMERO", "3.141592653589793");
    emitir_token("OPERADOR", "/");
    emitir_token("NUMERO", "180");

    seccion_sentencias();
    emitir_sentencia("x = angulo * PI / 180", "Conversion de grados a radianes");
    emitir_sentencia("suma = 0", "Inicializacion del acumulador");
    emitir_sentencia("termino = (-1)^n * x^(2n+1) / (2n+1)!", "Termino general de la serie");
    emitir_sentencia("suma = suma + termino", "Suma parcial de la serie");

    // Tripletas: se representan las cinco operaciones de cada término.
    seccion_tripletas();
    double anterior = 0.0;
    for (int n = 0; n < terminos; n++) {
        char expbuf[16];
        char pbuf[64];
        char fbuf[64];
        char sgnbuf[16];
        char acbuf[64];
        char t1[16], t2[16], t3[16], t4[16], t5[16];

        snprintf(expbuf, sizeof expbuf, "%d", 2 * n + 1);
        formato_numero(potencia[n], pbuf, sizeof pbuf);
        formato_numero(factorial[n], fbuf, sizeof fbuf);
        snprintf(sgnbuf, sizeof sgnbuf, "%d", signo[n] < 0 ? -1 : 1);
        formato_numero(anterior, acbuf, sizeof acbuf);

        emitir_tripleta("pow", xbuf, expbuf, t1, sizeof t1);
        emitir_tripleta("fact", expbuf, "-", t2, sizeof t2);
        emitir_tripleta("/", t1, t2, t3, sizeof t3);
        emitir_tripleta("*", sgnbuf, t3, t4, sizeof t4);
        emitir_tripleta("+", acbuf, t4, t5, sizeof t5);

        anterior = acumulada[n];
    }

    // Grid paso a paso: una fila por término.
    emitir_seccion("PASOS", "n\tSigno\tx^(2n+1)\t(2n+1)!\tTermino\tSuma acumulada");
    for (int n = 0; n < terminos; n++) {
        printf("%d\t%.0f\t%.10f\t%.0f\t%.10f\t%.10f\n",
               n, signo[n], potencia[n], factorial[n], termino[n], acumulada[n]);
    }

    const double referencia = sin(x);
    const double error_absoluto = fabs(suma - referencia);

    seccion_resultados();
    emitir_resultado("Angulo original", angbuf, "grados");
    emitir_resultado("Angulo normalizado", normbuf, "grados");
    emitir_resultado("x (radianes)", xbuf, "rad");
    emitir_resultado("Terminos usados", termbuf, "-");
    emitir_resultado_fijo("Serie de Taylor", suma, "-");
    emitir_resultado_fijo("Referencia sin(x)", referencia, "-");
    {
        char errbuf[64];
        snprintf(errbuf, sizeof errbuf, "%.10e", error_absoluto);
        emitir_resultado("Error absoluto", errbuf, "-");
    }

    emitir_fin();
    return 0;
}

/**
 * Fórmula general de la ecuación cuadrática: a*x^2 + b*x + c = 0.
 * Entrada: coeficientes a (distinto de 0), b y c.
 * Soporta raíces reales y complejas conjugadas.
 */
static int op_general(void) {
    emitir_op("Formula general (cuadratica)");

    double a = 0.0, b = 0.0, c = 0.0;
    if (leer_double_o_fallar("Coeficiente a", "El coeficiente a debe ser un numero valido.", &a)) {
        return 2;
    }
    if (leer_double_o_fallar("Coeficiente b", "El coeficiente b debe ser un numero valido.", &b)) {
        return 2;
    }
    if (leer_double_o_fallar("Coeficiente c", "El coeficiente c debe ser un numero valido.", &c)) {
        return 2;
    }
    if (a == 0.0) {
        return fallar("El coeficiente a no puede ser 0: la ecuacion no seria cuadratica.");
    }

    const double discriminante = b * b - 4.0 * a * c;
    const double denominador = 2.0 * a;
    const double raiz = sqrt(fabs(discriminante));

    char abuf[64], bbuf[64], cbuf[64];
    char dbuf[64], denbuf[64], raizbuf[64];
    formato_numero(a, abuf, sizeof abuf);
    formato_numero(b, bbuf, sizeof bbuf);
    formato_numero(c, cbuf, sizeof cbuf);
    formato_numero(discriminante, dbuf, sizeof dbuf);
    formato_numero(denominador, denbuf, sizeof denbuf);
    formato_numero(raiz, raizbuf, sizeof raizbuf);

    seccion_tokens();
    emitir_token("NUMERO", abuf);
    emitir_token("NUMERO", bbuf);
    emitir_token("NUMERO", cbuf);
    emitir_token("OPERADOR", "cuadratica");

    seccion_sentencias();
    emitir_sentencia("discriminante = b^2 - 4*a*c", "Calculo del discriminante");
    emitir_sentencia("x = (-b +/- sqrt(discriminante)) / (2*a)", "Formula general");
    if (discriminante < 0.0) {
        emitir_sentencia("discriminante < 0", "Las raices son complejas conjugadas");
    }

    seccion_tripletas();
    char t_b2[16], t_4a[16], t_4ac[16], t_disc[16], t_den[16], t_negb[16];
    char t_raiz[16], t_num1[16], t_num2[16], t_x1[16], t_x2[16];
    emitir_tripleta("*", bbuf, bbuf, t_b2, sizeof t_b2);
    emitir_tripleta("*", "4", abuf, t_4a, sizeof t_4a);
    emitir_tripleta("*", t_4a, cbuf, t_4ac, sizeof t_4ac);
    emitir_tripleta("-", t_b2, t_4ac, t_disc, sizeof t_disc);
    emitir_tripleta("*", "2", abuf, t_den, sizeof t_den);
    emitir_tripleta("neg", bbuf, "-", t_negb, sizeof t_negb);
    if (discriminante >= 0.0) {
        emitir_tripleta("sqrt", t_disc, "-", t_raiz, sizeof t_raiz);
        emitir_tripleta("+", t_negb, t_raiz, t_num1, sizeof t_num1);
        emitir_tripleta("-", t_negb, t_raiz, t_num2, sizeof t_num2);
        emitir_tripleta("/", t_num1, t_den, t_x1, sizeof t_x1);
        emitir_tripleta("/", t_num2, t_den, t_x2, sizeof t_x2);
    } else {
        emitir_tripleta("neg", t_disc, "-", t_raiz, sizeof t_raiz);
        emitir_tripleta("sqrt", t_raiz, "-", t_num1, sizeof t_num1);
        emitir_tripleta("/", t_negb, t_den, t_x1, sizeof t_x1);
        emitir_tripleta("/", t_num1, t_den, t_x2, sizeof t_x2);
    }

    seccion_pasos_generico();
    printf("1\tb^2\t%.10g\n", b * b);
    printf("2\t4*a*c\t%.10g\n", 4.0 * a * c);
    printf("3\tdiscriminante = b^2 - 4*a*c\t%s\n", dbuf);
    if (discriminante >= 0.0) {
        const double x1 = (-b + raiz) / denominador;
        const double x2 = (-b - raiz) / denominador;
        printf("4\tsqrt(discriminante)\t%s\n", raizbuf);
        printf("5\t-b\t%.10g\n", -b);
        printf("6\t2*a\t%s\n", denbuf);
        printf("7\tx1 = (-b + sqrt(d)) / (2*a)\t%.10f\n", x1);
        printf("8\tx2 = (-b - sqrt(d)) / (2*a)\t%.10f\n", x2);
    } else {
        double real = -b / denominador;
        const double imag = fabs(raiz / denominador);
        if (real == 0.0) {
            real = 0.0;
        }
        printf("4\t-discriminante\t%.10g\n", -discriminante);
        printf("5\tsqrt(-discriminante)\t%s\n", raizbuf);
        printf("6\tparte real = -b / (2*a)\t%.10f\n", real);
        printf("7\tparte imaginaria = sqrt(-d) / |2*a|\t%.10f\n", imag);
    }

    seccion_resultados();
    emitir_resultado("Discriminante", dbuf, "-");
    if (discriminante > 0.0) {
        emitir_resultado("Tipo de soluciones", "dos raices reales distintas", "-");
    } else if (discriminante == 0.0) {
        emitir_resultado("Tipo de soluciones", "una raiz real doble", "-");
    } else {
        emitir_resultado("Tipo de soluciones", "dos raices complejas conjugadas", "-");
    }

    if (discriminante >= 0.0) {
        emitir_resultado_fijo("x1", (-b + raiz) / denominador, "-");
        emitir_resultado_fijo("x2", (-b - raiz) / denominador, "-");
    } else {
        double real = -b / denominador;
        const double imag = fabs(raiz / denominador);
        if (real == 0.0) {
            real = 0.0;
        }
        char x1buf[128];
        char x2buf[128];
        snprintf(x1buf, sizeof x1buf, "%.10g + %.10gi", real, imag);
        snprintf(x2buf, sizeof x2buf, "%.10g - %.10gi", real, imag);
        emitir_resultado("x1", x1buf, "-");
        emitir_resultado("x2", x2buf, "-");
    }

    emitir_fin();
    return 0;
}

/**
 * Potenciación x^n.
 * Entrada: base x y exponente n. Se admiten exponentes reales, pero una base
 * negativa con exponente no entero no produce un resultado real.
 */
static int op_potenciacion(void) {
    emitir_op("Potenciacion");

    double base = 0.0, exponente = 0.0;
    if (leer_double_o_fallar("Base x", "La base debe ser un numero valido.", &base)) {
        return 2;
    }
    if (leer_double_o_fallar("Exponente n", "El exponente debe ser un numero valido.", &exponente)) {
        return 2;
    }
    if (base == 0.0 && exponente <= 0.0) {
        return fallar("Si la base es 0, el exponente debe ser mayor que 0.");
    }
    if (base < 0.0 && exponente != floor(exponente)) {
        return fallar("Una base negativa con exponente no entero no produce un resultado real.");
    }

    const double resultado = pow(base, exponente);
    if (!isfinite(resultado)) {
        return fallar("El resultado no es un numero real finito.");
    }

    char xbuf[64], nbuf[64], rbuf[64];
    formato_numero(base, xbuf, sizeof xbuf);
    formato_numero(exponente, nbuf, sizeof nbuf);
    formato_numero(resultado, rbuf, sizeof rbuf);

    seccion_tokens();
    emitir_token("NUMERO", xbuf);
    emitir_token("NUMERO", nbuf);
    emitir_token("OPERADOR", "pow");

    seccion_sentencias();
    emitir_sentencia("R = x^n", "Potenciacion de la base por el exponente");

    seccion_tripletas();
    char t1[16];
    emitir_tripleta("pow", xbuf, nbuf, t1, sizeof t1);

    seccion_pasos_generico();
    if (exponente == floor(exponente) && exponente >= 0.0 && exponente <= 20.0) {
        const int veces = (int) exponente;
        if (veces == 0) {
            printf("0\tx^0 = 1\t1\n");
        } else {
            double parcial = 1.0;
            for (int i = 1; i <= veces; i++) {
                parcial *= base;
                printf("%d\tx^%d\t%.10g\n", i, i, parcial);
            }
        }
    } else {
        printf("1\tpow(%s, %s)\t%s\n", xbuf, nbuf, rbuf);
    }

    seccion_resultados();
    emitir_resultado("Potencia", rbuf, "-");
    emitir_resultado("Base", xbuf, "-");
    emitir_resultado("Exponente", nbuf, "-");

    emitir_fin();
    return 0;
}

/* -------------------------------------------------------------------------
 * Volúmenes
 * ------------------------------------------------------------------------- */

/** Emite la sección TOKENS de una operación de volumen. */
static void tokens_dimensiones(const char *figura, const double *valores, int cantidad) {
    seccion_tokens();
    for (int i = 0; i < cantidad; i++) {
        emitir_token_numero(valores[i]);
    }
    emitir_token("OPERADOR", "volumen");
    emitir_token("OPERADOR", figura);
}

/** Emite el resultado final de un volumen. */
static void resultado_volumen(const char *figura, double volumen) {
    emitir_resultado("Figura", figura, "-");
    emitir_resultado_numero("Volumen", volumen, "unidades^3");
}

/** Volumen de un cubo: V = a^3. */
static int volumen_cubo(void) {
    emitir_op("Volumen - cubo");

    double arista = 0.0;
    if (leer_double_o_fallar("Arista a", "La arista debe ser un numero valido.", &arista)) {
        return 2;
    }
    if (arista < 0.0) {
        return fallar("La arista debe ser mayor o igual que 0.");
    }

    const double cuadrado = arista * arista;
    const double volumen = cuadrado * arista;
    const double valores[] = {arista};

    char abuf[64], cbuf[64], vbuf[64];
    formato_numero(arista, abuf, sizeof abuf);
    formato_numero(cuadrado, cbuf, sizeof cbuf);
    formato_numero(volumen, vbuf, sizeof vbuf);

    tokens_dimensiones("cubo", valores, 1);

    seccion_sentencias();
    emitir_sentencia("V = a^3", "Volumen de un cubo de arista a");

    seccion_tripletas();
    char t1[16], t2[16];
    emitir_tripleta("*", abuf, abuf, t1, sizeof t1);
    emitir_tripleta("*", t1, abuf, t2, sizeof t2);

    seccion_pasos_generico();
    printf("1\ta^2\t%s\n", cbuf);
    printf("2\ta^3\t%s\n", vbuf);

    seccion_resultados();
    resultado_volumen("cubo", volumen);

    emitir_fin();
    return 0;
}

/** Volumen de una caja: V = l * w * h. */
static int volumen_caja(void) {
    emitir_op("Volumen - caja");

    double largo = 0.0, ancho = 0.0, alto = 0.0;
    if (leer_double_o_fallar("Largo l", "El largo debe ser un numero valido.", &largo)) {
        return 2;
    }
    if (leer_double_o_fallar("Ancho w", "El ancho debe ser un numero valido.", &ancho)) {
        return 2;
    }
    if (leer_double_o_fallar("Alto h", "El alto debe ser un numero valido.", &alto)) {
        return 2;
    }
    if (largo < 0.0 || ancho < 0.0 || alto < 0.0) {
        return fallar("Las dimensiones deben ser mayores o iguales que 0.");
    }

    const double area = largo * ancho;
    const double volumen = area * alto;
    const double valores[] = {largo, ancho, alto};

    char lbuf[64], wbuf[64], hbuf[64], abuf[64], vbuf[64];
    formato_numero(largo, lbuf, sizeof lbuf);
    formato_numero(ancho, wbuf, sizeof wbuf);
    formato_numero(alto, hbuf, sizeof hbuf);
    formato_numero(area, abuf, sizeof abuf);
    formato_numero(volumen, vbuf, sizeof vbuf);

    tokens_dimensiones("caja", valores, 3);

    seccion_sentencias();
    emitir_sentencia("V = l * w * h", "Volumen de una caja rectangular");

    seccion_tripletas();
    char t1[16], t2[16];
    emitir_tripleta("*", lbuf, wbuf, t1, sizeof t1);
    emitir_tripleta("*", t1, hbuf, t2, sizeof t2);

    seccion_pasos_generico();
    printf("1\tl * w\t%s\n", abuf);
    printf("2\t(l * w) * h\t%s\n", vbuf);

    seccion_resultados();
    resultado_volumen("caja", volumen);

    emitir_fin();
    return 0;
}

/** Volumen de un cilindro: V = PI * r^2 * h. */
static int volumen_cilindro(void) {
    emitir_op("Volumen - cilindro");

    double radio = 0.0, altura = 0.0;
    if (leer_double_o_fallar("Radio r", "El radio debe ser un numero valido.", &radio)) {
        return 2;
    }
    if (leer_double_o_fallar("Altura h", "La altura debe ser un numero valido.", &altura)) {
        return 2;
    }
    if (radio < 0.0 || altura < 0.0) {
        return fallar("El radio y la altura deben ser mayores o iguales que 0.");
    }

    const double radio2 = radio * radio;
    const double area_base = M_PI * radio2;
    const double volumen = area_base * altura;
    const double valores[] = {radio, altura};

    char rbuf[64], hbuf[64], r2buf[64], abuf[64], vbuf[64];
    formato_numero(radio, rbuf, sizeof rbuf);
    formato_numero(altura, hbuf, sizeof hbuf);
    formato_numero(radio2, r2buf, sizeof r2buf);
    formato_numero(area_base, abuf, sizeof abuf);
    formato_numero(volumen, vbuf, sizeof vbuf);

    tokens_dimensiones("cilindro", valores, 2);

    seccion_sentencias();
    emitir_sentencia("V = PI * r^2 * h", "Volumen de un cilindro");

    seccion_tripletas();
    char t1[16], t2[16], t3[16];
    emitir_tripleta("*", rbuf, rbuf, t1, sizeof t1);
    emitir_tripleta("*", "3.141592653589793", t1, t2, sizeof t2);
    emitir_tripleta("*", t2, hbuf, t3, sizeof t3);

    seccion_pasos_generico();
    printf("1\tr^2\t%s\n", r2buf);
    printf("2\tPI * r^2\t%s\n", abuf);
    printf("3\t(PI * r^2) * h\t%s\n", vbuf);

    seccion_resultados();
    resultado_volumen("cilindro", volumen);

    emitir_fin();
    return 0;
}

/** Volumen de una esfera: V = (4/3) * PI * r^3. */
static int volumen_esfera(void) {
    emitir_op("Volumen - esfera");

    double radio = 0.0;
    if (leer_double_o_fallar("Radio r", "El radio debe ser un numero valido.", &radio)) {
        return 2;
    }
    if (radio < 0.0) {
        return fallar("El radio debe ser mayor o igual que 0.");
    }

    const double radio2 = radio * radio;
    const double radio3 = radio2 * radio;
    const double factor = 4.0 / 3.0;
    const double volumen = factor * M_PI * radio3;
    const double valores[] = {radio};

    char rbuf[64], r2buf[64], r3buf[64], fbuf[64], vbuf[64];
    formato_numero(radio, rbuf, sizeof rbuf);
    formato_numero(radio2, r2buf, sizeof r2buf);
    formato_numero(radio3, r3buf, sizeof r3buf);
    formato_numero(factor, fbuf, sizeof fbuf);
    formato_numero(volumen, vbuf, sizeof vbuf);

    tokens_dimensiones("esfera", valores, 1);

    seccion_sentencias();
    emitir_sentencia("V = (4/3) * PI * r^3", "Volumen de una esfera");

    seccion_tripletas();
    char t1[16], t2[16], t3[16], t4[16], t5[16];
    emitir_tripleta("*", rbuf, rbuf, t1, sizeof t1);
    emitir_tripleta("*", t1, rbuf, t2, sizeof t2);
    emitir_tripleta("/", "4", "3", t3, sizeof t3);
    emitir_tripleta("*", t3, "3.141592653589793", t4, sizeof t4);
    emitir_tripleta("*", t4, t2, t5, sizeof t5);

    seccion_pasos_generico();
    printf("1\tr^2\t%s\n", r2buf);
    printf("2\tr^3\t%s\n", r3buf);
    printf("3\t4/3\t%s\n", fbuf);
    printf("4\t(4/3) * PI * r^3\t%s\n", vbuf);

    seccion_resultados();
    resultado_volumen("esfera", volumen);

    emitir_fin();
    return 0;
}

/** Volumen de un cono: V = PI * r^2 * h / 3. */
static int volumen_cono(void) {
    emitir_op("Volumen - cono");

    double radio = 0.0, altura = 0.0;
    if (leer_double_o_fallar("Radio r", "El radio debe ser un numero valido.", &radio)) {
        return 2;
    }
    if (leer_double_o_fallar("Altura h", "La altura debe ser un numero valido.", &altura)) {
        return 2;
    }
    if (radio < 0.0 || altura < 0.0) {
        return fallar("El radio y la altura deben ser mayores o iguales que 0.");
    }

    const double radio2 = radio * radio;
    const double area_base = M_PI * radio2;
    const double producto = area_base * altura;
    const double volumen = producto / 3.0;
    const double valores[] = {radio, altura};

    char rbuf[64], hbuf[64], r2buf[64], abuf[64], pbuf[64], vbuf[64];
    formato_numero(radio, rbuf, sizeof rbuf);
    formato_numero(altura, hbuf, sizeof hbuf);
    formato_numero(radio2, r2buf, sizeof r2buf);
    formato_numero(area_base, abuf, sizeof abuf);
    formato_numero(producto, pbuf, sizeof pbuf);
    formato_numero(volumen, vbuf, sizeof vbuf);

    tokens_dimensiones("cono", valores, 2);

    seccion_sentencias();
    emitir_sentencia("V = PI * r^2 * h / 3", "Volumen de un cono");

    seccion_tripletas();
    char t1[16], t2[16], t3[16], t4[16];
    emitir_tripleta("*", rbuf, rbuf, t1, sizeof t1);
    emitir_tripleta("*", "3.141592653589793", t1, t2, sizeof t2);
    emitir_tripleta("*", t2, hbuf, t3, sizeof t3);
    emitir_tripleta("/", t3, "3", t4, sizeof t4);

    seccion_pasos_generico();
    printf("1\tr^2\t%s\n", r2buf);
    printf("2\tPI * r^2\t%s\n", abuf);
    printf("3\t(PI * r^2) * h\t%s\n", pbuf);
    printf("4\t((PI * r^2) * h) / 3\t%s\n", vbuf);

    seccion_resultados();
    resultado_volumen("cono", volumen);

    emitir_fin();
    return 0;
}

/** Despacha la figura de volumen indicada en la línea de comandos. */
static int op_volumen(const char *figura) {
    if (strcmp(figura, "cubo") == 0) {
        return volumen_cubo();
    }
    if (strcmp(figura, "caja") == 0) {
        return volumen_caja();
    }
    if (strcmp(figura, "cilindro") == 0) {
        return volumen_cilindro();
    }
    if (strcmp(figura, "esfera") == 0) {
        return volumen_esfera();
    }
    if (strcmp(figura, "cono") == 0) {
        return volumen_cono();
    }
    return fallar("Figura desconocida. Use: cubo, caja, cilindro, esfera o cono.");
}

/* -------------------------------------------------------------------------
 * Física
 * ------------------------------------------------------------------------- */

/** Fuerza: F = m * a. Entrada: masa m > 0 y aceleración a. */
static int op_fuerza(void) {
    emitir_op("Fuerza");

    double masa = 0.0, aceleracion = 0.0;
    if (leer_double_o_fallar("Masa m", "La masa debe ser un numero valido.", &masa)) {
        return 2;
    }
    if (leer_double_o_fallar("Aceleracion a", "La aceleracion debe ser un numero valido.", &aceleracion)) {
        return 2;
    }
    if (masa <= 0.0) {
        return fallar("La masa debe ser mayor que 0.");
    }

    const double fuerza = masa * aceleracion;

    char mbuf[64], abuf[64], fbuf[64];
    formato_numero(masa, mbuf, sizeof mbuf);
    formato_numero(aceleracion, abuf, sizeof abuf);
    formato_numero(fuerza, fbuf, sizeof fbuf);

    seccion_tokens();
    emitir_token("NUMERO", mbuf);
    emitir_token("NUMERO", abuf);
    emitir_token("OPERADOR", "fuerza");

    seccion_sentencias();
    emitir_sentencia("F = m * a", "Segunda ley de Newton");

    seccion_tripletas();
    char t1[16];
    emitir_tripleta("*", mbuf, abuf, t1, sizeof t1);

    seccion_pasos_generico();
    printf("1\tm * a\t%s\n", fbuf);

    seccion_resultados();
    emitir_resultado("Fuerza", fbuf, "N");
    emitir_resultado("Masa", mbuf, "kg");
    emitir_resultado("Aceleracion", abuf, "m/s^2");

    emitir_fin();
    return 0;
}

/** Trabajo: W = F * d * cos(theta). Entrada: F >= 0, d >= 0, ángulo en grados. */
static int op_trabajo(void) {
    emitir_op("Trabajo");

    double fuerza = 0.0, distancia = 0.0, angulo = 0.0;
    if (leer_double_o_fallar("Fuerza F", "La fuerza debe ser un numero valido.", &fuerza)) {
        return 2;
    }
    if (leer_double_o_fallar("Distancia d", "La distancia debe ser un numero valido.", &distancia)) {
        return 2;
    }
    if (leer_double_o_fallar("Angulo theta", "El angulo debe ser un numero valido.", &angulo)) {
        return 2;
    }
    if (fuerza < 0.0 || distancia < 0.0) {
        return fallar("La fuerza y la distancia deben ser mayores o iguales que 0.");
    }

    const double radianes = angulo * M_PI / 180.0;
    const double coseno = cos(radianes);
    const double trabajo = fuerza * distancia * coseno;

    char fbuf[64], dbuf[64], angbuf[64], radbuf[64], cosbuf[64], wbuf[64];
    formato_numero(fuerza, fbuf, sizeof fbuf);
    formato_numero(distancia, dbuf, sizeof dbuf);
    formato_numero(angulo, angbuf, sizeof angbuf);
    formato_numero(radianes, radbuf, sizeof radbuf);
    formato_numero(coseno, cosbuf, sizeof cosbuf);
    formato_numero(trabajo, wbuf, sizeof wbuf);

    seccion_tokens();
    emitir_token("NUMERO", fbuf);
    emitir_token("NUMERO", dbuf);
    emitir_token("NUMERO", angbuf);
    emitir_token("OPERADOR", "trabajo");

    seccion_sentencias();
    emitir_sentencia("rad = theta * PI / 180", "Conversion del angulo a radianes");
    emitir_sentencia("W = F * d * cos(rad)", "Trabajo realizado por una fuerza");

    seccion_tripletas();
    char t1[16], t2[16], t3[16], t4[16], t5[16];
    emitir_tripleta("*", angbuf, "3.141592653589793", t1, sizeof t1);
    emitir_tripleta("/", t1, "180", t2, sizeof t2);
    emitir_tripleta("cos", t2, "-", t3, sizeof t3);
    emitir_tripleta("*", fbuf, dbuf, t4, sizeof t4);
    emitir_tripleta("*", t4, t3, t5, sizeof t5);

    seccion_pasos_generico();
    printf("1\trad = theta * PI / 180\t%s\n", radbuf);
    printf("2\tcos(rad)\t%s\n", cosbuf);
    printf("3\tF * d\t%.10g\n", fuerza * distancia);
    printf("4\t(F * d) * cos(rad)\t%s\n", wbuf);

    seccion_resultados();
    emitir_resultado("Trabajo", wbuf, "J");
    emitir_resultado("Coseno del angulo", cosbuf, "-");
    emitir_resultado("Angulo en radianes", radbuf, "rad");

    emitir_fin();
    return 0;
}

/** Potencia: P = W / t. Entrada: trabajo W y tiempo t > 0. */
static int op_potencia(void) {
    emitir_op("Potencia");

    double trabajo = 0.0, tiempo = 0.0;
    if (leer_double_o_fallar("Trabajo W", "El trabajo debe ser un numero valido.", &trabajo)) {
        return 2;
    }
    if (leer_double_o_fallar("Tiempo t", "El tiempo debe ser un numero valido.", &tiempo)) {
        return 2;
    }
    if (tiempo <= 0.0) {
        return fallar("El tiempo debe ser mayor que 0.");
    }

    const double potencia = trabajo / tiempo;

    char wbuf[64], tbuf[64], pbuf[64];
    formato_numero(trabajo, wbuf, sizeof wbuf);
    formato_numero(tiempo, tbuf, sizeof tbuf);
    formato_numero(potencia, pbuf, sizeof pbuf);

    seccion_tokens();
    emitir_token("NUMERO", wbuf);
    emitir_token("NUMERO", tbuf);
    emitir_token("OPERADOR", "potencia");

    seccion_sentencias();
    emitir_sentencia("P = W / t", "Potencia como trabajo por unidad de tiempo");

    seccion_tripletas();
    char t1[16];
    emitir_tripleta("/", wbuf, tbuf, t1, sizeof t1);

    seccion_pasos_generico();
    printf("1\tW / t\t%s\n", pbuf);

    seccion_resultados();
    emitir_resultado("Potencia", pbuf, "W");
    emitir_resultado("Trabajo", wbuf, "J");
    emitir_resultado("Tiempo", tbuf, "s");

    emitir_fin();
    return 0;
}

/* -------------------------------------------------------------------------
 * Combinatoria
 * ------------------------------------------------------------------------- */

/**
 * Combinaciones: C(n, r) = n! / (r! * (n-r)!).
 * Se calcula de forma iterativa para evitar factoriales gigantes.
 */
static int op_combinaciones(void) {
    emitir_op("Combinaciones");

    long n = 0, r = 0;
    if (leer_entero_o_fallar("Total de elementos n", "n debe ser un entero valido.", &n)) {
        return 2;
    }
    if (leer_entero_o_fallar("Elementos elegidos r", "r debe ser un entero valido.", &r)) {
        return 2;
    }
    if (n < 0 || n > MAX_COMBINATORIA) {
        return fallar("n debe estar entre 0 y 1000.");
    }
    if (r < 0 || r > n) {
        return fallar("r debe cumplir 0 <= r <= n.");
    }

    double combinaciones = 1.0;
    for (long i = 1; i <= r; i++) {
        combinaciones = combinaciones * (double) (n - r + i) / (double) i;
    }
    if (!isfinite(combinaciones)) {
        return fallar("El resultado es demasiado grande para representarlo.");
    }

    char nbuf[32], rbuf[32], cbuf[128];
    snprintf(nbuf, sizeof nbuf, "%ld", n);
    snprintf(rbuf, sizeof rbuf, "%ld", r);
    formato_entero_grande(combinaciones, cbuf, sizeof cbuf);

    seccion_tokens();
    emitir_token("NUMERO", nbuf);
    emitir_token("NUMERO", rbuf);
    emitir_token("OPERADOR", "combinaciones");

    seccion_sentencias();
    emitir_sentencia("C(n, r) = n! / (r! * (n-r)!)", "Numero de combinaciones sin repeticion");
    emitir_sentencia("C(n, r) = producto (n-r+i)/i", "Calculo iterativo para evitar desbordamiento");

    seccion_tripletas();
    if (r == 0) {
        char t1[16];
        emitir_tripleta("comb", nbuf, rbuf, t1, sizeof t1);
    } else {
        double parcial = 1.0;
        for (long i = 1; i <= r; i++) {
            char pbuf[64], factorbuf[32], ibuf[32];
            char t1[16], t2[16];
            formato_numero(parcial, pbuf, sizeof pbuf);
            snprintf(factorbuf, sizeof factorbuf, "%ld", n - r + i);
            snprintf(ibuf, sizeof ibuf, "%ld", i);
            emitir_tripleta("*", pbuf, factorbuf, t1, sizeof t1);
            emitir_tripleta("/", t1, ibuf, t2, sizeof t2);
            parcial = parcial * (double) (n - r + i) / (double) i;
        }
    }

    emitir_seccion("PASOS", "i\tFactor (n-r+i)/i\tValor parcial");
    if (r == 0) {
        printf("0\t1\t1\n");
    } else {
        double parcial = 1.0;
        for (long i = 1; i <= r; i++) {
            const double factor = (double) (n - r + i) / (double) i;
            parcial *= factor;
            printf("%ld\t%.10g\t%.10f\n", i, factor, parcial);
        }
    }

    seccion_resultados();
    emitir_resultado("Combinaciones C(n, r)", cbuf, "-");
    emitir_resultado("n", nbuf, "-");
    emitir_resultado("r", rbuf, "-");

    emitir_fin();
    return 0;
}

/**
 * Permutaciones: P(n, r) = n! / (n-r)! = producto (n-i), i=0..r-1.
 */
static int op_permutaciones(void) {
    emitir_op("Permutaciones");

    long n = 0, r = 0;
    if (leer_entero_o_fallar("Total de elementos n", "n debe ser un entero valido.", &n)) {
        return 2;
    }
    if (leer_entero_o_fallar("Elementos elegidos r", "r debe ser un entero valido.", &r)) {
        return 2;
    }
    if (n < 0 || n > MAX_COMBINATORIA) {
        return fallar("n debe estar entre 0 y 1000.");
    }
    if (r < 0 || r > n) {
        return fallar("r debe cumplir 0 <= r <= n.");
    }

    double permutaciones = 1.0;
    for (long i = 0; i < r; i++) {
        permutaciones *= (double) (n - i);
    }
    if (!isfinite(permutaciones)) {
        return fallar("El resultado es demasiado grande para representarlo.");
    }

    char nbuf[32], rbuf[32], pbuf[128];
    snprintf(nbuf, sizeof nbuf, "%ld", n);
    snprintf(rbuf, sizeof rbuf, "%ld", r);
    formato_entero_grande(permutaciones, pbuf, sizeof pbuf);

    seccion_tokens();
    emitir_token("NUMERO", nbuf);
    emitir_token("NUMERO", rbuf);
    emitir_token("OPERADOR", "permutaciones");

    seccion_sentencias();
    emitir_sentencia("P(n, r) = n! / (n-r)!", "Numero de permutaciones sin repeticion");
    emitir_sentencia("P(n, r) = producto (n-i)", "Calculo iterativo de los r primeros factores");

    seccion_tripletas();
    if (r == 0) {
        char t1[16];
        emitir_tripleta("perm", nbuf, rbuf, t1, sizeof t1);
    } else {
        double parcial = 1.0;
        for (long i = 0; i < r; i++) {
            char pbuf_local[64], factorbuf[32];
            char t1[16];
            formato_numero(parcial, pbuf_local, sizeof pbuf_local);
            snprintf(factorbuf, sizeof factorbuf, "%ld", n - i);
            emitir_tripleta("*", pbuf_local, factorbuf, t1, sizeof t1);
            parcial *= (double) (n - i);
        }
    }

    emitir_seccion("PASOS", "i\tFactor (n-i)\tValor parcial");
    if (r == 0) {
        printf("0\t1\t1\n");
    } else {
        double parcial = 1.0;
        for (long i = 0; i < r; i++) {
            parcial *= (double) (n - i);
            printf("%ld\t%ld\t%.10f\n", i + 1, n - i, parcial);
        }
    }

    seccion_resultados();
    emitir_resultado("Permutaciones P(n, r)", pbuf, "-");
    emitir_resultado("n", nbuf, "-");
    emitir_resultado("r", rbuf, "-");

    emitir_fin();
    return 0;
}

/* =========================================================================
 * 5. Punto de entrada
 * ========================================================================= */

/**
 * Punto de entrada del motor nativo.
 *
 * @param argc número de argumentos
 * @param argv argv[1] = operación; argv[2] = figura para "volumen"
 * @return 0 si el cálculo terminó bien; 2 si hubo un error de validación
 */
int main(int argc, char **argv) {
    if (argc < 2) {
        return fallar("Uso: motor <operacion> [figura]. Consulte el README.");
    }

    const char *operacion = argv[1];
    if (strcmp(operacion, "raiz") == 0) {
        return op_raiz();
    }
    if (strcmp(operacion, "taylor") == 0) {
        return op_taylor();
    }
    if (strcmp(operacion, "general") == 0) {
        return op_general();
    }
    if (strcmp(operacion, "potenciacion") == 0) {
        return op_potenciacion();
    }
    if (strcmp(operacion, "volumen") == 0) {
        if (argc < 3) {
            return fallar("Falta la figura para la operacion volumen.");
        }
        return op_volumen(argv[2]);
    }
    if (strcmp(operacion, "fuerza") == 0) {
        return op_fuerza();
    }
    if (strcmp(operacion, "trabajo") == 0) {
        return op_trabajo();
    }
    if (strcmp(operacion, "potencia") == 0) {
        return op_potencia();
    }
    if (strcmp(operacion, "combinaciones") == 0) {
        return op_combinaciones();
    }
    if (strcmp(operacion, "permutaciones") == 0) {
        return op_permutaciones();
    }

    return fallar("Operacion desconocida. Consulte el README.");
}
