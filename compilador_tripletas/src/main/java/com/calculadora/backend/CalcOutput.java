package com.calculadora.backend;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * Resultado ya interpretado de una ejecución del motor nativo.
 *
 * Protocolo (una línea por registro, campos separados por TAB):
 *   @@SECCION  col1  col2 ...    cabecera de una tabla
 *   dato1      dato2 ...         filas de la tabla
 *   @@ERROR    mensaje           error de entrada o de cálculo
 *   @@FIN                        fin de la salida (el motor terminó bien)
 */
public final class CalcOutput 
{

    /** Una tabla: cabecera + filas. */
    public static final class Tabla {
        public final String[] columnas;
        public final List<String[]> filas = new ArrayList<>();

        Tabla(String[] columnas) 
        {
            this.columnas = columnas;
        }
    }

    public static final String TOKENS = "TOKENS";
    public static final String SENTENCIAS = "SENTENCIAS";
    public static final String TRIPLETAS = "TRIPLETAS";
    public static final String PASOS = "PASOS";
    public static final String RESULTADOS = "RESULTADOS";
    public static final String GRAMATICA = "GRAMATICA";

    private String titulo = "";
    private String error;
    private boolean completo;
    private final Map<String, Tabla> secciones = new LinkedHashMap<>();

    private CalcOutput() { }

    /** Interpreta las líneas que el motor escribió en su salida estándar. */
    public static CalcOutput parse(List<String> lineas) 
    {
        CalcOutput out = new CalcOutput();
        Tabla actual = null;

        for (String linea : lineas) 
            {
            if (linea.startsWith("@@")) 
                {
                String[] p = linea.split("\t", -1);
                String tag = p[0].substring(2);
                switch (tag) {
                    case "OP" -> out.titulo = p.length > 1 ? p[1] : "";
                    case "ERROR" -> out.error = p.length > 1 ? p[1] : "Error desconocido";
                    case "FIN" -> out.completo = true;
                    default -> {
                        actual = new Tabla(Arrays.copyOfRange(p, 1, p.length));
                        out.secciones.put(tag, actual);
                    }
                }
            } 
            
            else if (actual != null && !linea.isEmpty()) 
                actual.filas.add(linea.split("\t", -1));
        }
        return out;
    }

    public String getTitulo() { return titulo; }

    /** Mensaje de error, o null si el cálculo fue correcto. */
    public String getError() { return error; }

    public boolean hayError() { return error != null; }

    /** true si el motor llegó a escribir la marca @@FIN. */
    public boolean isCompleto() { return completo; }

    /** Devuelve la tabla de una sección, o null si no existe. */
    public Tabla tabla(String seccion) { return secciones.get(seccion); }

    /**
     * Devuelve los nombres de las secciones en el mismo orden en que el motor
     * nativo las escribió. La interfaz gráfica usa esta lista para construir
     * una pestaña por sección sin depender de un conjunto fijo de nombres.
     *
     * @return copia inmutable de los nombres de sección
     */
    public List<String> nombresSecciones() { return List.copyOf(secciones.keySet()); }
}