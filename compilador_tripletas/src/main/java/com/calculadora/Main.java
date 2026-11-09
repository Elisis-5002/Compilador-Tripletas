package com.calculadora;

import com.calculadora.gui.MainFrame;

import javax.swing.SwingUtilities;
import javax.swing.UIManager;

/**
 * Punto de entrada de la aplicación.
 * Muestra el menú de operaciones; los cálculos los realiza el ejecutable en C
 * (native/motor.exe) invocado con Runtime.
 */
public final class Main 
{
    private Main() { }

    public static void main(String[] args) 
    {
        SwingUtilities.invokeLater(() -> {
            try {
                UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
            } catch (Exception ignorada) {
                // se usa el aspecto por defecto de Swing
            }
            new MainFrame().setVisible(true);
        });
    }
}
