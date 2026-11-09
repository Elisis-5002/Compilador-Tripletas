package com.calculadora.gui;

import com.calculadora.backend.CalcOutput;

import javax.swing.BorderFactory;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTabbedPane;
import javax.swing.JTable;
import javax.swing.SwingConstants;
import javax.swing.table.DefaultTableCellRenderer;
import javax.swing.table.DefaultTableModel;
import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.Font;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

/**
 * Panel que presenta las secciones devueltas por el motor nativo.
 *
 * <p>Cada sección del protocolo ({@code TOKENS}, {@code SENTENCIAS},
 * {@code TRIPLETAS}, {@code PASOS}, {@code RESULTADOS}, ...) se muestra en una
 * pestaña con su propio {@link JTable}. De este modo, el grid de resultados
 * paso a paso de la serie de Taylor convive con las demás representaciones
 * sin que la interfaz tenga que conocer cada operación.</p>
 *
 * <p>Las tablas son de solo lectura, alinean a la derecha los valores
 * numéricos y ajustan el ancho de sus columnas al contenido. Cuando el motor
 * reporta un error de validación se muestra un mensaje en lugar de una tabla
 * vacía.</p>
 */
public final class ResultadosPanel extends JPanel {

    private static final long serialVersionUID = 1L;

    /**
     * Orden en el que se desea mostrar las secciones conocidas. El cálculo
     * paso a paso va primero porque es el grid principal de la asignación;
     * los resultados finales quedan en la pestaña siguiente.
     */
    private static final List<String> ORDEN_PREFERIDO = List.of(
            CalcOutput.PASOS,
            CalcOutput.RESULTADOS,
            CalcOutput.TRIPLETAS,
            CalcOutput.SENTENCIAS,
            CalcOutput.TOKENS,
            CalcOutput.GRAMATICA);

    private static final Color COLOR_TEXTO = new Color(70, 70, 70);
    private static final Color COLOR_ERROR = new Color(176, 0, 32);
    private static final Color COLOR_LINEA = new Color(225, 225, 225);

    private final JTabbedPane pestanas = new JTabbedPane();
    private final JLabel mensaje = new JLabel("", SwingConstants.CENTER);

    /** Crea el panel vacío con un mensaje de bienvenida. */
    public ResultadosPanel() {
        super(new BorderLayout());
        mensaje.setForeground(COLOR_TEXTO);
        mensaje.setBorder(BorderFactory.createEmptyBorder(24, 24, 24, 24));
        limpiar();
    }

    /** Restablece el panel a su estado inicial. */
    public void limpiar() {
        mostrarMensaje("Seleccione una operación y presione Calcular.");
    }

    /**
     * Muestra un mensaje informativo en el área de resultados.
     *
     * @param texto mensaje para el usuario
     */
    public void mostrarMensaje(String texto) {
        mensaje.setForeground(COLOR_TEXTO);
        mensaje.setText("<html><div style='text-align:center;'>"
                + escapar(texto).replace("\n", "<br>") + "</div></html>");
        mostrarSolo(mensaje);
    }

    /**
     * Muestra un error de validación o de ejecución con un estilo destacado.
     *
     * @param texto mensaje de error
     */
    public void mostrarError(String texto) {
        mensaje.setForeground(COLOR_ERROR);
        mensaje.setText("<html><div style='text-align:center;'>"
                + "<b>No se pudo completar el cálculo</b><br><br>"
                + escapar(texto).replace("\n", "<br>") + "</div></html>");
        mostrarSolo(mensaje);
    }

    /**
     * Construye una pestaña por cada sección disponible en la salida del
     * motor. Las secciones desconocidas se agregan al final conservando el
     * orden en que llegaron.
     *
     * @param salida salida interpretada del motor nativo
     */
    public void mostrar(CalcOutput salida) {
        if (salida == null) {
            mostrarMensaje("El motor no devolvió resultados.");
            return;
        }
        if (salida.hayError()) {
            mostrarError(salida.getError());
            return;
        }

        pestanas.removeAll();
        Set<String> pendientes = new LinkedHashSet<>(salida.nombresSecciones());
        int agregadas = 0;

        for (String nombre : ORDEN_PREFERIDO) {
            if (pendientes.remove(nombre)) {
                CalcOutput.Tabla tabla = salida.tabla(nombre);
                if (tabla != null) {
                    pestanas.addTab(tituloSeccion(nombre), crearPanelTabla(tabla));
                    agregadas++;
                }
            }
        }
        for (String nombre : pendientes) {
            CalcOutput.Tabla tabla = salida.tabla(nombre);
            if (tabla != null) {
                pestanas.addTab(tituloSeccion(nombre), crearPanelTabla(tabla));
                agregadas++;
            }
        }

        if (agregadas == 0) {
            mostrarMensaje("El motor no devolvió secciones de resultados.");
            return;
        }

        pestanas.setSelectedIndex(0);
        mostrarSolo(pestanas);
    }

    /** Reemplaza el contenido visible del panel por un único componente. */
    private void mostrarSolo(Component componente) {
        removeAll();
        add(componente, BorderLayout.CENTER);
        revalidate();
        repaint();
    }

    /**
     * Crea el panel de una tabla con su barra de desplazamiento y un pie que
     * indica el número de filas y columnas.
     */
    private JPanel crearPanelTabla(CalcOutput.Tabla tabla) {
        DefaultTableModel modelo = new DefaultTableModel(tabla.columnas, 0) {
            private static final long serialVersionUID = 1L;

            @Override
            public boolean isCellEditable(int fila, int columna) {
                return false;
            }
        };

        for (String[] fila : tabla.filas) {
            Object[] valores = new Object[tabla.columnas.length];
            for (int i = 0; i < valores.length; i++) {
                valores[i] = i < fila.length ? fila[i] : "";
            }
            modelo.addRow(valores);
        }

        JTable tablaUi = new JTable(modelo);
        tablaUi.setAutoResizeMode(JTable.AUTO_RESIZE_OFF);
        tablaUi.setRowHeight(22);
        tablaUi.setFillsViewportHeight(true);
        tablaUi.setShowGrid(true);
        tablaUi.setGridColor(COLOR_LINEA);
        tablaUi.setFont(new Font(Font.MONOSPACED, Font.PLAIN, 12));
        tablaUi.getTableHeader().setFont(new Font("SansSerif", Font.BOLD, 12));
        tablaUi.setDefaultRenderer(Object.class, new RendererNumerico());

        for (int columna = 0; columna < tablaUi.getColumnCount(); columna++) {
            tablaUi.getColumnModel().getColumn(columna)
                    .setPreferredWidth(anchoColumna(tabla, columna));
        }

        JScrollPane desplazamiento = new JScrollPane(tablaUi);
        desplazamiento.setBorder(BorderFactory.createEmptyBorder());

        JLabel pie = new JLabel(" " + tabla.filas.size() + " fila(s) · "
                + tabla.columnas.length + " columna(s)");
        pie.setFont(new Font("SansSerif", Font.PLAIN, 11));
        pie.setForeground(Color.GRAY);
        pie.setBorder(BorderFactory.createEmptyBorder(4, 8, 4, 8));

        JPanel panel = new JPanel(new BorderLayout());
        panel.add(desplazamiento, BorderLayout.CENTER);
        panel.add(pie, BorderLayout.SOUTH);
        return panel;
    }

    /**
     * Calcula un ancho razonable para una columna a partir de la cabecera y
     * de los valores más largos de la tabla.
     */
    private static int anchoColumna(CalcOutput.Tabla tabla, int columna) {
        int maximo = tabla.columnas[columna].length();
        for (String[] fila : tabla.filas) {
            if (columna < fila.length) {
                maximo = Math.max(maximo, fila[columna].length());
            }
        }
        int ancho = maximo * 8 + 24;
        return Math.max(90, Math.min(480, ancho));
    }

    /** Traduce el nombre técnico de una sección a un título legible. */
    private static String tituloSeccion(String nombre) {
        return switch (nombre) {
            case CalcOutput.RESULTADOS -> "Resultados";
            case CalcOutput.PASOS -> "Paso a paso";
            case CalcOutput.TRIPLETAS -> "Tripletas";
            case CalcOutput.SENTENCIAS -> "Sentencias";
            case CalcOutput.TOKENS -> "Tokens";
            case CalcOutput.GRAMATICA -> "Gramática";
            default -> nombre;
        };
    }

    /** Escapa caracteres especiales para insertar texto en HTML. */
    private static String escapar(String texto) {
        if (texto == null) {
            return "";
        }
        return texto.replace("&", "&amp;")
                .replace("<", "&lt;")
                .replace(">", "&gt;");
    }

    /**
     * Renderizador que alinea a la derecha los valores que representan
     * números (incluidos los que usan coma decimal) y a la izquierda el resto.
     */
    private static final class RendererNumerico extends DefaultTableCellRenderer {

        private static final long serialVersionUID = 1L;

        @Override
        public Component getTableCellRendererComponent(JTable tabla, Object valor,
                                                       boolean seleccionada, boolean foco,
                                                       int fila, int columna) {
            Component componente = super.getTableCellRendererComponent(
                    tabla, valor, seleccionada, foco, fila, columna);
            String texto = valor == null ? "" : valor.toString();
            setHorizontalAlignment(esNumero(texto) ? SwingConstants.RIGHT : SwingConstants.LEFT);
            return componente;
        }

        /** Indica si el texto se puede interpretar como un número. */
        private static boolean esNumero(String texto) {
            if (texto == null || texto.isBlank()) {
                return false;
            }
            try {
                Double.parseDouble(texto.replace(',', '.'));
                return true;
            } catch (NumberFormatException ex) {
                return false;
            }
        }
    }
}
