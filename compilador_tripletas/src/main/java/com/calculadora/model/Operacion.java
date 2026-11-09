package com.calculadora.model;

/**
 * Describe una operación del menú y cómo invocarla en el ejecutable nativo.
 *
 * @param categoria categoría del menú
 * @param titulo    nombre mostrado en el menú
 * @param formula   fórmula mostrada como ayuda
 * @param argsExe   argumentos para motor.exe, p. ej. {"volumen","esfera"}
 * @param campos    datos que debe capturar el usuario, EN EL ORDEN en que el motor los lee
 */
public record Operacion(String categoria, String titulo, String formula,
                        String[] argsExe, Campo[] campos) {

    /** Lo que muestra el JList del menú. */
    @Override
    public String toString() {
        return titulo;
    }
}