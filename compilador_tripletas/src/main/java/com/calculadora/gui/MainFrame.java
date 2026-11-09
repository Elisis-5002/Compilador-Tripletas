package com.calculadora.gui;

import com.calculadora.backend.CalcOutput;
import com.calculadora.backend.ExeLocator;
import com.calculadora.backend.NativeRunner;
import com.calculadora.model.Campo;
import com.calculadora.model.Catalogo;
import com.calculadora.model.Operacion;

import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JMenuItem;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JProgressBar;
import javax.swing.JScrollPane;
import javax.swing.JSplitPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;
import javax.swing.JTree;
import javax.swing.SwingUtilities;
import javax.swing.SwingWorker;
import javax.swing.ToolTipManager;
import javax.swing.WindowConstants;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeCellRenderer;
import javax.swing.tree.TreePath;
import javax.swing.tree.TreeSelectionModel;
import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.Font;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.Toolkit;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutionException;

/**
 * Ventana principal de la calculadora.
 *
 * <p>La ventana ofrece un árbol con las operaciones agrupadas por categoría,
 * un formulario que se construye dinámicamente a partir de
 * {@link Operacion#campos()} y un área de resultados con una pestaña por
 * sección del protocolo nativo. La sección {@code PASOS} es la que muestra el
 * cálculo de la serie de Taylor fila por fila, tal como pide la asignación.</p>
 *
 * <p>El motor se ejecuta en un hilo de fondo con {@link SwingWorker}; de este
 * modo la interfaz permanece fluida y se puede cancelar visualmente la espera
 * mediante una barra de progreso indeterminada.</p>
 */
public final class MainFrame extends JFrame {

    private static final long serialVersionUID = 1L;

    private static final int ANCHO_VENTANA = 1180;
    private static final int ALTO_VENTANA = 680;
    private static final int ANCHO_MENU = 270;

    private final JTree arbolOperaciones;
    private final JLabel etiquetaTitulo = new JLabel(" ");
    private final JLabel etiquetaFormula = new JLabel(" ");
    private final JPanel panelCampos = new JPanel(new GridBagLayout());
    private final List<JTextField> campos = new ArrayList<>();
    private final JButton botonCalcular = new JButton("Calcular");
    private final JButton botonLimpiar = new JButton("Limpiar");
    private final ResultadosPanel panelResultados = new ResultadosPanel();
    private final JLabel etiquetaEstado = new JLabel("Listo.");
    private final JProgressBar progreso = new JProgressBar();

    /** Operación seleccionada actualmente en el árbol. */
    private Operacion operacionActual;

    /** Crea y configura la ventana principal. Debe invocarse en el EDT. */
    public MainFrame() {
        super("Calculadora con tokens, sentencias y tripletas");
        setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
        setJMenuBar(crearMenuBar());

        arbolOperaciones = crearArbolOperaciones();

        setLayout(new BorderLayout(10, 10));
        ((javax.swing.JComponent) getContentPane())
                .setBorder(BorderFactory.createEmptyBorder(8, 8, 8, 8));

        JSplitPane division = new JSplitPane(
                JSplitPane.HORIZONTAL_SPLIT,
                crearPanelOperaciones(),
                crearPanelPrincipal());
        division.setDividerLocation(ANCHO_MENU);
        division.setResizeWeight(0.0);
        division.setContinuousLayout(true);

        add(division, BorderLayout.CENTER);
        add(crearBarraEstado(), BorderLayout.SOUTH);

        configurarEventos();
        setPreferredSize(new Dimension(ANCHO_VENTANA, ALTO_VENTANA));
        setMinimumSize(new Dimension(900, 600));
        pack();
        setLocationRelativeTo(null);
        seleccionarPrimeraOperacion();
    }

    /* ---------------------------------------------------------------------
     * Construcción de la interfaz
     * --------------------------------------------------------------------- */

    /**
     * Construye el árbol de operaciones agrupadas por categoría.
     *
     * @return árbol configurado con el listener de selección
     */
    private JTree crearArbolOperaciones() {
        DefaultMutableTreeNode raiz = new DefaultMutableTreeNode("Operaciones");
        Map<String, DefaultMutableTreeNode> categorias = new LinkedHashMap<>();

        for (Operacion operacion : Catalogo.TODAS) {
            DefaultMutableTreeNode categoria = categorias.get(operacion.categoria());
            if (categoria == null) {
                categoria = new DefaultMutableTreeNode(operacion.categoria());
                categorias.put(operacion.categoria(), categoria);
                raiz.add(categoria);
            }
            categoria.add(new DefaultMutableTreeNode(operacion));
        }

        JTree arbol = new JTree(raiz);
        arbol.setRootVisible(false);
        arbol.setShowsRootHandles(true);
        arbol.getSelectionModel().setSelectionMode(TreeSelectionModel.SINGLE_TREE_SELECTION);
        arbol.setCellRenderer(new RendererArbol());
        ToolTipManager.sharedInstance().registerComponent(arbol);

        arbol.addTreeSelectionListener(evento -> {
            Object seleccionado = arbol.getLastSelectedPathComponent();
            if (seleccionado instanceof DefaultMutableTreeNode nodo
                    && nodo.getUserObject() instanceof Operacion operacion) {
                mostrarOperacion(operacion);
            }
        });

        for (int fila = 0; fila < arbol.getRowCount(); fila++) {
            arbol.expandRow(fila);
        }
        return arbol;
    }

    /** Crea el panel izquierdo que contiene el árbol de operaciones. */
    private JPanel crearPanelOperaciones() {
        JPanel panel = new JPanel(new BorderLayout());
        panel.setBorder(BorderFactory.createTitledBorder("Operaciones"));
        panel.setPreferredSize(new Dimension(ANCHO_MENU, 0));
        panel.add(new JScrollPane(arbolOperaciones), BorderLayout.CENTER);
        return panel;
    }

    /** Crea el panel derecho: encabezado, formulario y resultados. */
    private JPanel crearPanelPrincipal() {
        JPanel panel = new JPanel(new BorderLayout(8, 8));

        etiquetaTitulo.setFont(new Font("SansSerif", Font.BOLD, 18));
        etiquetaTitulo.setForeground(new Color(25, 45, 90));

        etiquetaFormula.setFont(new Font("SansSerif", Font.PLAIN, 13));
        etiquetaFormula.setForeground(new Color(0, 102, 153));
        etiquetaFormula.setBorder(BorderFactory.createEmptyBorder(2, 0, 6, 0));

        JPanel textos = new JPanel();
        textos.setLayout(new javax.swing.BoxLayout(textos, javax.swing.BoxLayout.Y_AXIS));
        textos.add(etiquetaTitulo);
        textos.add(etiquetaFormula);

        panelCampos.setBorder(BorderFactory.createTitledBorder("Datos de entrada"));

        JPanel acciones = new JPanel(new FlowLayout(FlowLayout.RIGHT, 8, 4));
        botonCalcular.setFont(new Font("SansSerif", Font.BOLD, 13));
        acciones.add(botonLimpiar);
        acciones.add(botonCalcular);

        JPanel zonaFormulario = new JPanel(new BorderLayout(4, 4));
        zonaFormulario.add(panelCampos, BorderLayout.CENTER);
        zonaFormulario.add(acciones, BorderLayout.SOUTH);

        JPanel cabecera = new JPanel(new BorderLayout(6, 6));
        cabecera.add(textos, BorderLayout.NORTH);
        cabecera.add(zonaFormulario, BorderLayout.CENTER);

        panel.add(cabecera, BorderLayout.NORTH);
        panel.add(panelResultados, BorderLayout.CENTER);
        return panel;
    }

    /** Crea la barra inferior con el estado y la barra de progreso. */
    private JPanel crearBarraEstado() {
        JPanel barra = new JPanel(new BorderLayout(8, 0));
        barra.setBorder(BorderFactory.createEmptyBorder(4, 6, 0, 6));

        etiquetaEstado.setFont(new Font("SansSerif", Font.PLAIN, 12));

        progreso.setIndeterminate(true);
        progreso.setVisible(false);
        progreso.setPreferredSize(new Dimension(140, 14));

        barra.add(etiquetaEstado, BorderLayout.CENTER);
        barra.add(progreso, BorderLayout.EAST);
        return barra;
    }

    /** Crea la barra de menús. */
    private JMenuBar crearMenuBar() {
        JMenuBar barra = new JMenuBar();

        JMenu menuArchivo = new JMenu("Archivo");
        JMenuItem salir = new JMenuItem("Salir");
        salir.addActionListener(evento -> dispose());
        menuArchivo.add(salir);

        JMenu menuAyuda = new JMenu("Ayuda");
        JMenuItem comoCompilar = new JMenuItem("Cómo compilar el motor nativo");
        comoCompilar.addActionListener(evento -> mostrarAyudaMotor());
        JMenuItem acercaDe = new JMenuItem("Acerca de");
        acercaDe.addActionListener(evento -> mostrarAcercaDe());
        menuAyuda.add(comoCompilar);
        menuAyuda.add(acercaDe);

        barra.add(menuArchivo);
        barra.add(menuAyuda);
        return barra;
    }

    /** Conecta los botones y el botón por defecto del formulario. */
    private void configurarEventos() {
        botonCalcular.addActionListener(evento -> calcular());
        botonLimpiar.addActionListener(evento -> limpiar());
        getRootPane().setDefaultButton(botonCalcular);
    }

    /** Selecciona la primera operación del catálogo al abrir la ventana. */
    private void seleccionarPrimeraOperacion() {
        DefaultMutableTreeNode raiz = (DefaultMutableTreeNode) arbolOperaciones.getModel().getRoot();
        if (raiz.getChildCount() == 0) {
            return;
        }
        DefaultMutableTreeNode categoria = (DefaultMutableTreeNode) raiz.getChildAt(0);
        if (categoria.getChildCount() == 0) {
            return;
        }
        DefaultMutableTreeNode primera = (DefaultMutableTreeNode) categoria.getChildAt(0);
        TreePath camino = new TreePath(primera.getPath());
        arbolOperaciones.setSelectionPath(camino);
        arbolOperaciones.scrollPathToVisible(camino);
    }

    /* ---------------------------------------------------------------------
     * Formulario dinámico
     * --------------------------------------------------------------------- */

    /**
     * Actualiza el encabezado y reconstruye los campos del formulario para la
     * operación seleccionada.
     *
     * @param operacion operación elegida en el árbol
     */
    private void mostrarOperacion(Operacion operacion) {
        operacionActual = operacion;
        etiquetaTitulo.setText(operacion.categoria() + " · " + operacion.titulo());
        etiquetaFormula.setText("<html>" + escaparHtml(operacion.formula()) + "</html>");

        panelCampos.removeAll();
        campos.clear();

        GridBagConstraints restricciones = new GridBagConstraints();
        restricciones.insets = new Insets(4, 6, 4, 6);
        restricciones.anchor = GridBagConstraints.WEST;

        int fila = 0;
        for (Campo campo : operacion.campos()) {
            restricciones.gridx = 0;
            restricciones.gridy = fila;
            restricciones.weightx = 0.0;
            restricciones.fill = GridBagConstraints.NONE;
            JLabel etiqueta = new JLabel(campo.etiqueta() + ":");
            etiqueta.setFont(new Font("SansSerif", Font.PLAIN, 13));
            panelCampos.add(etiqueta, restricciones);

            restricciones.gridx = 1;
            restricciones.weightx = 1.0;
            restricciones.fill = GridBagConstraints.HORIZONTAL;
            JTextField texto = new JTextField(18);
            texto.setFont(new Font(Font.MONOSPACED, Font.PLAIN, 13));
            panelCampos.add(texto, restricciones);

            restricciones.gridx = 2;
            restricciones.weightx = 0.0;
            restricciones.fill = GridBagConstraints.NONE;
            JLabel ayuda = new JLabel(campo.unidad());
            ayuda.setForeground(new Color(110, 110, 110));
            ayuda.setFont(new Font("SansSerif", Font.ITALIC, 11));
            panelCampos.add(ayuda, restricciones);

            campos.add(texto);
            fila++;
        }

        panelCampos.revalidate();
        panelCampos.repaint();
        panelResultados.limpiar();
        etiquetaEstado.setText("Operación seleccionada: " + operacion.titulo());

        SwingUtilities.invokeLater(() -> {
            if (!campos.isEmpty()) {
                campos.get(0).requestFocusInWindow();
            }
        });
    }

    /* ---------------------------------------------------------------------
     * Ejecución del cálculo
     * --------------------------------------------------------------------- */

    /** Valida el formulario y lanza el motor nativo en segundo plano. */
    private void calcular() {
        if (operacionActual == null) {
            avisar("Seleccione una operación en el árbol de la izquierda.");
            return;
        }

        List<String> valores = new ArrayList<>();
        for (int i = 0; i < campos.size(); i++) {
            String valor = campos.get(i).getText().trim();
            if (valor.isEmpty()) {
                avisar("Complete el campo \"" + operacionActual.campos()[i].etiqueta() + "\".");
                campos.get(i).requestFocusInWindow();
                return;
            }
            valores.add(valor);
        }

        Path ejecutable = ExeLocator.buscar();
        if (ejecutable == null) {
            mostrarAyudaMotor();
            return;
        }

        panelResultados.limpiar();
        botonCalcular.setEnabled(false);
        progreso.setVisible(true);
        etiquetaEstado.setText("Ejecutando motor nativo: " + ejecutable.getFileName() + " ...");

        final Operacion operacion = operacionActual;
        SwingWorker<CalcOutput, Void> trabajador = new SwingWorker<>() {
            @Override
            protected CalcOutput doInBackground() throws Exception {
                return new NativeRunner().ejecutar(ejecutable, operacion, valores);
            }

            @Override
            protected void done() {
                botonCalcular.setEnabled(true);
                progreso.setVisible(false);
                try {
                    CalcOutput salida = get();
                    if (salida.hayError()) {
                        panelResultados.mostrarError(salida.getError());
                        etiquetaEstado.setText("El motor rechazó los datos: " + salida.getError());
                        Toolkit.getDefaultToolkit().beep();
                    } else {
                        panelResultados.mostrar(salida);
                        etiquetaEstado.setText("Cálculo completado: " + salida.getTitulo());
                    }
                } catch (InterruptedException ex) {
                    Thread.currentThread().interrupt();
                    mostrarFallo("Ejecución interrumpida.", ex);
                } catch (ExecutionException ex) {
                    Throwable causa = ex.getCause() != null ? ex.getCause() : ex;
                    mostrarFallo(causa.getMessage(), causa);
                }
            }
        };
        trabajador.execute();
    }

    /** Limpia los campos y el área de resultados. */
    private void limpiar() {
        for (JTextField campo : campos) {
            campo.setText("");
        }
        panelResultados.limpiar();
        etiquetaEstado.setText("Campos limpiados.");
        if (!campos.isEmpty()) {
            campos.get(0).requestFocusInWindow();
        }
    }

    /** Muestra un aviso de validación de la interfaz. */
    private void avisar(String mensaje) {
        JOptionPane.showMessageDialog(this, mensaje, "Datos incompletos",
                JOptionPane.WARNING_MESSAGE);
    }

    /** Presenta un fallo de ejecución en la barra de estado y en un diálogo. */
    private void mostrarFallo(String mensaje, Throwable causa) {
        String texto = (mensaje == null || mensaje.isBlank())
                ? "Error desconocido al ejecutar el motor nativo."
                : mensaje;
        etiquetaEstado.setText("Error: " + texto);
        panelResultados.mostrarError(texto);
        JOptionPane.showMessageDialog(this, texto, "Error de ejecución",
                JOptionPane.ERROR_MESSAGE);
        if (causa != null) {
            System.err.println("Fallo al ejecutar el motor nativo: " + causa);
        }
    }

    /* ---------------------------------------------------------------------
     * Ayuda
     * --------------------------------------------------------------------- */

    /** Muestra las instrucciones para compilar el motor nativo. */
    private void mostrarAyudaMotor() {
        Path rutaEsperada = Paths.get(System.getProperty("user.dir", "."))
                .toAbsolutePath()
                .resolve("native")
                .resolve(ExeLocator.nombre());

        String texto = """
                No se encontró el ejecutable nativo del motor de cálculo.

                Compílelo antes de presionar Calcular.

                Windows (MinGW-w64):
                  gcc -O2 -Wall -Wextra -std=c11 -o native/motor.exe native/motor.c

                Linux o macOS:
                  gcc -O2 -Wall -Wextra -std=c11 -o native/motor native/motor.c -lm

                También puede usar los scripts incluidos:
                  scripts/compilar_motor.bat   (Windows)
                  scripts/compilar_motor.sh    (Linux/macOS)

                Ruta buscada:
                  %s
                """.formatted(rutaEsperada);

        JTextArea area = new JTextArea(texto);
        area.setEditable(false);
        area.setFont(new Font(Font.MONOSPACED, Font.PLAIN, 12));
        area.setBackground(new Color(250, 250, 250));
        area.setBorder(BorderFactory.createEmptyBorder(8, 8, 8, 8));

        JScrollPane desplazamiento = new JScrollPane(area);
        desplazamiento.setPreferredSize(new Dimension(760, 330));

        JOptionPane.showMessageDialog(this, desplazamiento,
                "Cómo compilar el motor nativo", JOptionPane.INFORMATION_MESSAGE);
    }

    /** Muestra la información de la aplicación. */
    private void mostrarAcercaDe() {
        String texto = """
                Calculadora con tokens, sentencias y tripletas
                Versión 1.0

                La interfaz Swing invoca un motor nativo escrito en C mediante
                java.lang.Runtime. El motor calcula la operación y devuelve
                tablas de tokens, sentencias, tripletas, pasos y resultados,
                que esta ventana presenta en un grid por pestañas.

                Incluye la serie de Taylor del seno con visualización paso a paso.
                Tecnologías: Java 17 + Swing + C11.
                """;
        JOptionPane.showMessageDialog(this, texto, "Acerca de",
                JOptionPane.INFORMATION_MESSAGE);
    }

    /** Escapa caracteres especiales para insertar la fórmula en HTML. */
    private static String escaparHtml(String texto) {
        if (texto == null) {
            return "";
        }
        return texto.replace("&", "&amp;")
                .replace("<", "&lt;")
                .replace(">", "&gt;");
    }

    /**
     * Renderizador del árbol que resalta las categorías y deja las
     * operaciones con tipografía normal.
     */
    private static final class RendererArbol extends DefaultTreeCellRenderer {

        private static final long serialVersionUID = 1L;

        @Override
        public Component getTreeCellRendererComponent(JTree arbol, Object valor,
                                                       boolean seleccionado, boolean expandido,
                                                       boolean hoja, int fila, boolean foco) {
            super.getTreeCellRendererComponent(arbol, valor, seleccionado, expandido,
                    hoja, fila, foco);
            Font fuente = getFont();
            if (fuente != null) {
                boolean esOperacion = valor instanceof DefaultMutableTreeNode nodo
                        && nodo.getUserObject() instanceof Operacion;
                setFont(fuente.deriveFont(esOperacion ? Font.PLAIN : Font.BOLD));
            }
            return this;
        }
    }
}
