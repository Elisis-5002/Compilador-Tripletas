package com.calculadora.model;

import java.util.List;

/**
 * Catálogo de todas las operaciones del menú.
 * El orden de los Campo debe coincidir con el orden de lectura de motor.c.
 */
public final class Catalogo 
{

    private Catalogo() { }

    private static Campo c(String etiqueta, String unidad) 
    {
        return new Campo(etiqueta, unidad);
    }

    private static Operacion op(String cat, String titulo, String formula, Campo[] campos, String... args) 
    {
        return new Operacion(cat, titulo, formula, args, campos);
    }

    public static final List<Operacion> TODAS = List.of(
        op("Matemáticas", "Raíz cuadrada", "R = √x",

           new Campo[]{ c("Radicando x", "x ≥ 0") }, "raiz"),

        op("Matemáticas", "Seno (serie de Taylor)", "sen(x) = x − x³/3! + x⁵/5! − …",

           new Campo[]{ c("Ángulo", "grados"), c("Número de términos n", "entero de 1 a 20") }, "taylor"),

        op("Matemáticas", "Fórmula general (cuadrática)", "x = (−b ± √(b² − 4ac)) / 2a",

           new Campo[]{ c("Coeficiente a", "a ≠ 0"), c("Coeficiente b", ""), c("Coeficiente c", "") }, "general"),

        op("Matemáticas", "Potenciación", "R = xⁿ",
        
           new Campo[]{ c("Base x", ""), c("Exponente n", "") }, "potenciacion"),

        op("Geometría", "Volumen — cubo", "V = a³",

           new Campo[]{ c("Arista a", "unidades") }, "volumen", "cubo"),

        op("Geometría", "Volumen — caja", "V = l · w · h",

           new Campo[]{ c("Largo l", "unidades"), c("Ancho w", "unidades"), c("Alto h", "unidades") }, "volumen", "caja"),

        op("Geometría", "Volumen — cilindro", "V = π · r² · h",

           new Campo[]{ c("Radio r", "unidades"), c("Altura h", "unidades") }, "volumen", "cilindro"),

        op("Geometría", "Volumen — esfera", "V = 4/3 · π · r³",

           new Campo[]{ c("Radio r", "unidades") }, "volumen", "esfera"),

        op("Geometría", "Volumen — cono", "V = π · r² · h / 3",

           new Campo[]{ c("Radio r", "unidades"), c("Altura h", "unidades") }, "volumen", "cono"),

        op("Física", "Fuerza", "F = m · a",

           new Campo[]{ c("Masa m", "kg, m > 0"), c("Aceleración a", "m/s²") }, "fuerza"),

        op("Física", "Trabajo", "W = F · d · cos θ",

           new Campo[]{ c("Fuerza F", "N, F ≥ 0"), c("Distancia d", "m, d ≥ 0"),

                        c("Ángulo θ", "grados (0 si F y d son paralelas)") }, "trabajo"),

        op("Física", "Potencia", "P = W / t",
        
           new Campo[]{ c("Trabajo W", "J"), c("Tiempo t", "s, t > 0") }, "potencia"),

        op("Combinatoria", "Combinaciones", "C(n, r) = n! / (r! · (n − r)!)",

           new Campo[]{ c("Total de elementos n", "entero ≥ 0"), c("Elementos elegidos r", "entero, 0 ≤ r ≤ n") },

           "combinaciones"),

        op("Combinatoria", "Permutaciones", "P(n, r) = n! / (n − r)!",

           new Campo[]{ c("Total de elementos n", "entero ≥ 0"), c("Elementos elegidos r", "entero, 0 ≤ r ≤ n") },
           
           "permutaciones")
    );
}