package com.calculadora.backend;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

/**
 * Localiza el ejecutable nativo
 *
 * Orden de búsqueda:
 *  1. propiedad del sistema -Dmotor.exe=ruta
 *  2. variable de entorno MOTOR_EXE
 *  3. native/ y la raíz del directorio de trabajo, de la carpeta del
 *     programa y de hasta 4 carpetas superiores
 */
public final class ExeLocator 
{

    private ExeLocator() { }

    /** Nombre del ejecutable según el sistema operativo. */
    public static String nombre() 
    {
        return System.getProperty("os.name", "").toLowerCase().contains("win") ? "motor.exe" : "motor";
    }

    /** @return ruta absoluta del ejecutable o null si no se encontró. */
    public static Path buscar() 
    {
        String prop = System.getProperty("motor.exe");

        if (prop != null && Files.isRegularFile(Paths.get(prop))) 
            return Paths.get(prop).toAbsolutePath();
        
        String env = System.getenv("MOTOR_EXE");

        if (env != null && Files.isRegularFile(Paths.get(env))) 
            return Paths.get(env).toAbsolutePath();

        List<Path> bases = new ArrayList<>();
        bases.add(Paths.get(System.getProperty("user.dir", ".")).toAbsolutePath());
        
        try 
        {
            Path loc = Paths.get(ExeLocator.class.getProtectionDomain().getCodeSource().getLocation().toURI());
            bases.add(Files.isDirectory(loc) ? loc : loc.getParent());
        } 
        
        catch (Exception ignorada) 
        {
            // basta con el directorio de trabajo
        }

        for (Path base : bases) 
            {
            Path d = base;
            
            for (int i = 0; i < 5 && d != null; i++, d = d.getParent()) 
            {
                Path a = d.resolve("native").resolve(nombre());
                if (Files.isRegularFile(a)) return a;
                Path b = d.resolve(nombre());
                if (Files.isRegularFile(b)) return b;
            }
        }
        return null;
    }
}