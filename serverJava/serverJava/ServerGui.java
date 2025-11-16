package serverJava;

import Utils.Rect;
import Utils.CrocVariant;
import Utils.FruitVariant;

import javax.swing.*;
import javax.swing.border.TitledBorder;
import java.awt.*;
import java.util.List;

public class ServerGui {

    private final GameServer server;
    private final JFrame frame;
    private final JComboBox<Integer> clientCombo;

    private final JRadioButton crocRadio;
    private final JRadioButton fruitRadio;
    private final JComboBox<String> variantCombo;

    private final JSpinner segmentsSpinner;     // N divisiones
    private final JComboBox<Integer> posCombo;  // posición 1..N
    private final JButton deleteFruitBtn;       // eliminar fruta

    // Para que "Delete fruit" sepa dónde actuar
    private enum Target { NONE, VINE, PLATFORM }
    private Target lastTarget = Target.NONE;
    private int lastIndex = -1;

    public ServerGui(GameServer server) {
        this.server = server;

        frame = new JFrame("DonCEy Kong Jr - Trapper Control");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setLayout(new BorderLayout());

        // ---------- TOP PANEL ----------
        JPanel top = new JPanel(new FlowLayout(FlowLayout.LEFT));
        top.add(new JLabel("Target client:"));

        clientCombo = new JComboBox<>();
        top.add(clientCombo);

        JButton refreshBtn = new JButton("Refresh clients");
        refreshBtn.addActionListener(e -> refreshClients());
        top.add(refreshBtn);

        // Modo
        crocRadio  = new JRadioButton("Crocodile", true);
        fruitRadio = new JRadioButton("Fruit");
        ButtonGroup modeGroup = new ButtonGroup();
        modeGroup.add(crocRadio);
        modeGroup.add(fruitRadio);

        top.add(new JLabel("  Mode:"));
        top.add(crocRadio);
        top.add(fruitRadio);

        // Variantes
        variantCombo = new JComboBox<>();
        top.add(new JLabel("  Variant:"));
        top.add(variantCombo);

        // Segments & Position
        segmentsSpinner = new JSpinner(new SpinnerNumberModel(5, 1, 20, 1));
        ((JSpinner.DefaultEditor)segmentsSpinner.getEditor()).getTextField().setColumns(2);
        posCombo = new JComboBox<>();
        top.add(new JLabel("  Segments:"));
        top.add(segmentsSpinner);
        top.add(new JLabel("  Position:"));
        top.add(posCombo);

        // Delete fruit
        deleteFruitBtn = new JButton("Delete fruit");
        deleteFruitBtn.addActionListener(e -> deleteFruitAtSelection());
        top.add(deleteFruitBtn);

        // Eventos
        crocRadio.addActionListener(e -> loadVariantsForCroc());
        fruitRadio.addActionListener(e -> loadVariantsForFruit());
        segmentsSpinner.addChangeListener(e -> fillPositions());

        // Cargas iniciales
        loadVariantsForCroc();
        fillPositions();

        frame.add(top, BorderLayout.NORTH);

        // ---------- CENTER PANEL: vines & platforms ----------
        JPanel center = new JPanel(new GridLayout(2, 1));

        // Vines panel
        JPanel vinesPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        vinesPanel.setBorder(new TitledBorder("Vines"));
        List<Rect> vines = server.getVines();
        for (int i = 0; i < vines.size(); i++) {
            final int idx = i;
            JButton b = new JButton("Vine " + i);
            b.addActionListener(e -> { lastTarget = Target.VINE; lastIndex = idx; spawnOnVine(idx); });
            vinesPanel.add(b);
        }

        // Platforms panel
        JPanel platformsPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        platformsPanel.setBorder(new TitledBorder("Platforms"));
        List<Rect> plats = server.getPlatforms();
        for (int i = 0; i < plats.size(); i++) {
            final int idx = i;
            JButton b = new JButton("Plat " + i);
            b.addActionListener(e -> { lastTarget = Target.PLATFORM; lastIndex = idx; spawnOnPlatform(idx); });
            platformsPanel.add(b);
        }

        center.add(vinesPanel);
        center.add(platformsPanel);
        frame.add(center, BorderLayout.CENTER);

        frame.pack();
        frame.setLocationRelativeTo(null);
        refreshClients();
    }

    // --- Helpers GUI ---

    private void loadVariantsForCroc(){
        variantCombo.removeAllItems();
        variantCombo.addItem("RED");
        variantCombo.addItem("BLUE");
        variantCombo.setSelectedIndex(0);
    }

    private void loadVariantsForFruit(){
        variantCombo.removeAllItems();
        variantCombo.addItem("BANANA");
        variantCombo.addItem("APPLE");
        variantCombo.addItem("ORANGE");
        variantCombo.setSelectedIndex(0);
    }

    private void fillPositions(){
        posCombo.removeAllItems();
        int N = getSegments();
        for (int i=1;i<=N;i++) posCombo.addItem(i);
        posCombo.setSelectedIndex(0);
    }

    private int getSegments(){ return (int) segmentsSpinner.getValue(); }
    private int getPosition(){ return (Integer) posCombo.getSelectedItem(); }

    private byte resolveVariantCode(){
        String v = (String) variantCombo.getSelectedItem();
        if (crocRadio.isSelected()){
            if ("RED".equals(v))  return CrocVariant.RED.code;
            if ("BLUE".equals(v)) return CrocVariant.BLUE.code;
            return (byte)0;
        } else {
            if ("BANANA".equals(v)) return FruitVariant.BANANA.code;
            if ("APPLE".equals(v))  return FruitVariant.APPLE.code;
            if ("ORANGE".equals(v)) return FruitVariant.ORANGE.code;
            return (byte)0;
        }
    }

    private void refreshClients() {
        clientCombo.removeAllItems();
        for (Integer id : server.getClientIdsSnapshot()) clientCombo.addItem(id);
    }

    private Integer getSelectedClientId() { return (Integer) clientCombo.getSelectedItem(); }

    // Click en “Vine N”
    private void spawnOnVine(int vineIndex) {
        Integer clientId = getSelectedClientId();
        if (clientId == null) {
            JOptionPane.showMessageDialog(frame, "No client selected", "Error", JOptionPane.ERROR_MESSAGE);
            return;
        }
        byte variant = resolveVariantCode();
        int N = getSegments(), pos = getPosition();
        if (crocRadio.isSelected()) {
            server.spawnCrocOnVineForClient(clientId, vineIndex, variant, pos, N);
        } else {
            server.spawnFruitOnVineForClient(clientId, vineIndex, variant, pos, N);
        }
    }

    // Click en “Plat N”
    private void spawnOnPlatform(int platformIndex) {
        Integer clientId = getSelectedClientId();
        if (clientId == null) {
            JOptionPane.showMessageDialog(frame, "No client selected", "Error", JOptionPane.ERROR_MESSAGE);
            return;
        }
        byte variant = resolveVariantCode();
        int N = getSegments(), pos = getPosition();
        if (crocRadio.isSelected()) {
            server.spawnCrocOnPlatformForClient(clientId, platformIndex, variant, pos, N);
        } else {
            server.spawnFruitOnPlatformForClient(clientId, platformIndex, variant, pos, N);
        }
    }

    // “Delete fruit” actúa sobre el último contenedor clicado (liana/plataforma) usando el segmento actual
    private void deleteFruitAtSelection(){
        Integer clientId = getSelectedClientId();
        if (clientId == null) {
            JOptionPane.showMessageDialog(frame, "No client selected", "Error", JOptionPane.ERROR_MESSAGE);
            return;
        }
        if (lastTarget == Target.NONE || lastIndex < 0) {
            JOptionPane.showMessageDialog(frame, "Click primero en una Vine/Plat para seleccionar el contenedor.", "Info", JOptionPane.INFORMATION_MESSAGE);
            return;
        }
        int N = getSegments(), pos = getPosition();
        switch (lastTarget){
            case VINE -> server.removeFruitOnVineForClient(clientId, lastIndex, pos, N);
            case PLATFORM -> server.removeFruitOnPlatformForClient(clientId, lastIndex, pos, N);
            default -> { /* nada */ }
        }
    }

    public void show() { frame.setVisible(true); }
}
