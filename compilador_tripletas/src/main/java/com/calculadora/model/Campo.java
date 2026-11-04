package compilador_tripletas.src.main.java.com.calculadora.model;

/**
 * Un dato de entrada que el usuario debe escribir para una operación.
 *
 * @param etiqueta texto que se muestra junto al campo
 * @param unidad   unidad o ayuda breve, puede ser cadena vacía
 */
public record Campo(String etiqueta, String unidad) { }