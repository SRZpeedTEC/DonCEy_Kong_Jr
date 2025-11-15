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
    private final JComboBox<String> variantCombo;  // << nuevo

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

        // Cambiar opciones de variante cuando cambia el modo
        crocRadio.addActionListener(e -> loadVariantsForCroc());
        fruitRadio.addActionListener(e -> loadVariantsForFruit());

        // carga inicial
        loadVariantsForCroc();

        frame.add(top, BorderLayout.NORTH);

        // ---------- CENTER PANEL: vines & platforms ----------
        JPanel center = new JPanel(new GridLayout(2, 1));

        JPanel vinesPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        vinesPanel.setBorder(new TitledBorder("Vines"));
        List<Rect> vines = server.getVines();
        for (int i = 0; i < vines.size(); i++) {
            final int idx = i;
            JButton b = new JButton("Vine " + i);
            b.addActionListener(e -> spawnOnVine(idx));
            vinesPanel.add(b);
        }

        JPanel platformsPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        platformsPanel.setBorder(new TitledBorder("Platforms"));
        List<Rect> plats = server.getPlatforms();
        for (int i = 0; i < plats.size(); i++) {
            final int idx = i;
            JButton b = new JButton("Plat " + i);
            b.addActionListener(e -> spawnOnPlatform(idx));
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

    private Integer getSelectedClientId() {
        return (Integer) clientCombo.getSelectedItem();
    }

    // Click en “Vine N”
    private void spawnOnVine(int vineIndex) {
        Integer clientId = getSelectedClientId();
        if (clientId == null) {
            JOptionPane.showMessageDialog(frame, "No client selected", "Error", JOptionPane.ERROR_MESSAGE);
            return;
        }
        byte variant = resolveVariantCode();
        if (crocRadio.isSelected()) {
            server.spawnCrocOnVineForClient(clientId, vineIndex, variant);   // << pasa variant
        } else {
            server.spawnFruitOnVineForClient(clientId, vineIndex, variant);  // << pasa variant
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
        if (crocRadio.isSelected()) {
            server.spawnCrocOnPlatformForClient(clientId, platformIndex, variant);  // << pasa variant
        } else {
            server.spawnFruitOnPlatformForClient(clientId, platformIndex, variant); // << pasa variant
        }
    }

    public void show() { frame.setVisible(true); }
}
