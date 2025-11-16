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

    // Position único (1..5)
    private final JComboBox<Integer> posCombo;

    // Selección de contenedor
    private enum Target { NONE, VINE, PLATFORM }
    private Target selectedTarget = Target.NONE;
    private int selectedIndex = -1;

    // Botones acción
    private final JButton sendBtn;
    private final JButton deleteFruitBtn;

    public ServerGui(GameServer server) {
        this.server = server;

        frame = new JFrame("DonCEy Kong Jr - Admin");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setLayout(new BorderLayout());

        // ---------- TOP ----------
        JPanel top = new JPanel(new FlowLayout(FlowLayout.LEFT));
        top.add(new JLabel("Target client:"));

        clientCombo = new JComboBox<>();
        top.add(clientCombo);

        JButton refreshBtn = new JButton("Refresh");
        refreshBtn.addActionListener(e -> refreshClients());
        top.add(refreshBtn);

        // Mode + Variant
        crocRadio  = new JRadioButton("Crocodile", true);
        fruitRadio = new JRadioButton("Fruit");
        ButtonGroup modeGroup = new ButtonGroup();
        modeGroup.add(crocRadio);
        modeGroup.add(fruitRadio);

        top.add(new JLabel("  Mode:"));
        top.add(crocRadio);
        top.add(fruitRadio);

        variantCombo = new JComboBox<>();
        top.add(new JLabel("  Variant:"));
        top.add(variantCombo);

        crocRadio.addActionListener(e -> loadVariantsForCroc());
        fruitRadio.addActionListener(e -> loadVariantsForFruit());
        loadVariantsForCroc(); // default

        // Position único (1..5)
        posCombo = new JComboBox<>(new Integer[]{1,2,3,4,5});
        top.add(new JLabel("  Position:"));
        top.add(posCombo);

        // Acciones
        sendBtn = new JButton("Send");
        deleteFruitBtn = new JButton("Delete fruit");

        sendBtn.addActionListener(e -> doSend());
        deleteFruitBtn.addActionListener(e -> doDeleteFruit());

        top.add(sendBtn);
        top.add(deleteFruitBtn);

        frame.add(top, BorderLayout.NORTH);

        // ---------- CENTER: listas de Vines / Platforms ----------
        JPanel center = new JPanel(new GridLayout(2, 1));

        // Vines
        JPanel vinesPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        vinesPanel.setBorder(new TitledBorder("Vines (click para seleccionar)"));
        List<Rect> vines = server.getVines();
        for (int i = 0; i < vines.size(); i++) {
            final int idx = i;
            JButton b = new JButton("Vine " + i);
            b.addActionListener(e -> selectContainer(Target.VINE, idx));
            vinesPanel.add(b);
        }

        // Platforms
        JPanel platsPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        platsPanel.setBorder(new TitledBorder("Platforms (click para seleccionar)"));
        List<Rect> plats = server.getPlatforms();
        for (int i = 0; i < plats.size(); i++) {
            final int idx = i;
            JButton b = new JButton("Plat " + i);
            b.addActionListener(e -> selectContainer(Target.PLATFORM, idx));
            platsPanel.add(b);
        }

        center.add(vinesPanel);
        center.add(platsPanel);
        frame.add(center, BorderLayout.CENTER);

        frame.pack();
        frame.setLocationRelativeTo(null);
        refreshClients();
    }

    private void selectContainer(Target t, int idx){
        this.selectedTarget = t;
        this.selectedIndex  = idx;
        String name = (t==Target.VINE) ? ("Vine " + idx) : (t==Target.PLATFORM ? ("Plat " + idx) : "(none)");
        JOptionPane.showMessageDialog(frame, "Selected: " + name);
    }

    private void doSend(){
        Integer clientId = (Integer) clientCombo.getSelectedItem();
        if (clientId == null) { msg("No client selected"); return; }
        if (selectedTarget == Target.NONE || selectedIndex < 0) { msg("Select a Vine/Platform first"); return; }

        byte variant = resolveVariantCode();
        int pos = (Integer) posCombo.getSelectedItem(); // 1..5

        switch (selectedTarget){
            case VINE -> {
                if (crocRadio.isSelected()) server.spawnCrocOnVineForClient(clientId, selectedIndex, variant, pos);
                else                        server.spawnFruitOnVineForClient(clientId, selectedIndex, variant, pos);
            }
            case PLATFORM -> {
                if (crocRadio.isSelected()) server.spawnCrocOnPlatformForClient(clientId, selectedIndex, variant, pos);
                else                        server.spawnFruitOnPlatformForClient(clientId, selectedIndex, variant, pos);
            }
            default -> {}
        }
    }

    private void doDeleteFruit(){
        Integer clientId = (Integer) clientCombo.getSelectedItem();
        if (clientId == null) { msg("No client selected"); return; }
        if (selectedTarget == Target.NONE || selectedIndex < 0) { msg("Select a Vine/Platform first"); return; }

        int pos = (Integer) posCombo.getSelectedItem(); // 1..5

        switch (selectedTarget){
            case VINE     -> server.removeFruitOnVineForClient(clientId, selectedIndex, pos);
            case PLATFORM -> server.removeFruitOnPlatformForClient(clientId, selectedIndex, pos);
            default -> {}
        }
    }

    // --- helpers ---
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
            if ("RED".equals(v))  return Utils.CrocVariant.RED.code;
            if ("BLUE".equals(v)) return Utils.CrocVariant.BLUE.code;
            return (byte)0;
        } else {
            if ("BANANA".equals(v)) return Utils.FruitVariant.BANANA.code;
            if ("APPLE".equals(v))  return Utils.FruitVariant.APPLE.code;
            if ("ORANGE".equals(v)) return Utils.FruitVariant.ORANGE.code;
            return (byte)0;
        }
    }
    private void refreshClients(){
        clientCombo.removeAllItems();
        for (Integer id : server.getClientIdsSnapshot()) clientCombo.addItem(id);
    }
    private void msg(String m){ JOptionPane.showMessageDialog(frame, m); }

    public void show(){ frame.setVisible(true); }
}
