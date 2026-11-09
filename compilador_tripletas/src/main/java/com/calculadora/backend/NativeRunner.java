package com.calculadora.backend;

import com.calculadora.model.Operacion;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.Writer;
import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.TimeUnit;

/**
 * Invoca el ejecutable nativo escrito en C mediante {@link Runtime#exec}.
 *
 * <p>El motor nativo recibe la operación como argumentos de línea de comandos
 * y los datos del usuario por la entrada estándar (una línea por dato). A
 * cambio, escribe en la salida estándar un protocolo de texto con secciones
 * delimitadas por líneas que comienzan con {@code @@}. La clase
 * {@link CalcOutput} se encarga de interpretar ese protocolo.</p>
 *
 * <p>La ejecución se realiza fuera del hilo de la interfaz (normalmente desde
 * un {@code SwingWorker}) y tiene un tiempo máximo de espera para que la
 * aplicación no quede bloqueada si el binario nativo falla.</p>
 */
public final class NativeRunner {

    /** Tiempo máximo, en segundos, que se espera al proceso nativo. */
    private static final long TIMEOUT_SEGUNDOS = 15;

    /** Tiempo máximo, en milisegundos, para que el hilo lector termine tras el proceso. */
    private static final long ESPERA_LECTOR_MS = 2_000;

    /**
     * Ejecuta una operación en el motor nativo y devuelve su salida ya
     * interpretada.
     *
     * @param exe     ruta del ejecutable nativo (motor.exe o motor)
     * @param op      operación elegida en el menú; sus {@code argsExe()} se
     *                pasan al proceso
     * @param valores datos escritos por el usuario, en el mismo orden que
     *                {@link Operacion#campos()}
     * @return salida interpretada (puede contener un error de validación
     *         emitido por el motor)
     * @throws IOException          si el proceso no pudo lanzarse, no terminó
     *                              a tiempo o su salida quedó incompleta
     * @throws InterruptedException si el hilo que espera es interrumpido
     */
    public CalcOutput ejecutar(Path exe, Operacion op, List<String> valores)
            throws IOException, InterruptedException {

        List<String> comandos = new ArrayList<>();
        comandos.add(exe.toString());
        comandos.addAll(Arrays.asList(op.argsExe()));

        // Uso explícito de java.lang.Runtime, tal como pide la asignación.
        // El directorio de trabajo del proceso es la carpeta del ejecutable;
        // si no tiene padre, se deja el directorio actual de Java.
        Path padre = exe.toAbsolutePath().getParent();
        Process proceso = Runtime.getRuntime().exec(
                comandos.toArray(new String[0]),
                null,
                padre == null ? null : padre.toFile());

        try {
            iniciarDrenajeStderr(proceso);

            // Se lee la salida estándar en un hilo aparte para evitar
            // interbloqueos si el proceso escribe mientras aún se envían datos.
            List<String> lineas = Collections.synchronizedList(new ArrayList<>());
            Thread lector = iniciarLectorStdout(proceso, lineas);

            enviarDatos(proceso, valores);

            if (!proceso.waitFor(TIMEOUT_SEGUNDOS, TimeUnit.SECONDS)) {
                proceso.destroyForcibly();
                throw new IOException("El ejecutable nativo no terminó en "
                        + TIMEOUT_SEGUNDOS + " segundos.");
            }

            lector.join(ESPERA_LECTOR_MS);

            List<String> copia;
            synchronized (lineas) {
                copia = new ArrayList<>(lineas);
            }

            CalcOutput salida = CalcOutput.parse(copia);
            if (!salida.isCompleto()) {
                throw new IOException("El ejecutable nativo terminó de forma inesperada "
                        + "(código " + proceso.exitValue() + ").");
            }
            return salida;
        } finally {
            if (proceso.isAlive()) {
                proceso.destroyForcibly();
            }
        }
    }

    /**
     * Vacía el flujo de error del proceso en segundo plano. El motor escribe
     * allí sus mensajes de ayuda mientras espera datos; si no se drena, el
     * búfer del sistema operativo podría llenarse y bloquear al proceso.
     */
    private static void iniciarDrenajeStderr(Process proceso) {
        Thread drenaje = new Thread(() -> {
            try (InputStream error = proceso.getErrorStream()) {
                error.transferTo(OutputStream.nullOutputStream());
            } catch (IOException ignorada) {
                // El flujo se cierra al terminar o destruir el proceso.
            }
        }, "drenaje-stderr");
        drenaje.setDaemon(true);
        drenaje.start();
    }

    /**
     * Inicia un hilo que lee la salida estándar del motor línea por línea.
     *
     * @return el hilo lector, ya iniciado
     */
    private static Thread iniciarLectorStdout(Process proceso, List<String> lineas) {
        Thread lector = new Thread(() -> {
            try (BufferedReader reader = new BufferedReader(
                    new InputStreamReader(proceso.getInputStream(), StandardCharsets.UTF_8))) {
                String linea;
                while ((linea = reader.readLine()) != null) {
                    lineas.add(linea);
                }
            } catch (IOException ignorada) {
                // El flujo se cierra al terminar o destruir el proceso.
            }
        }, "lector-stdout");
        lector.setDaemon(true);
        lector.start();
        return lector;
    }

    /**
     * Envía los datos del usuario por la entrada estándar, una línea por
     * valor. Se eliminan los saltos de línea internos para no alterar el
     * protocolo de entrada.
     */
    private static void enviarDatos(Process proceso, List<String> valores) {
        try (Writer writer = new OutputStreamWriter(
                proceso.getOutputStream(), StandardCharsets.UTF_8)) {
            for (String valor : valores) {
                writer.write(valor.replace('\r', ' ').replace('\n', ' '));
                writer.write('\n');
            }
        } catch (IOException ignorada) {
            // El motor pudo rechazar los argumentos y cerrar stdin antes de
            // leer todos los datos; su salida (incluido @@ERROR) se leerá igual.
        }
    }
}
