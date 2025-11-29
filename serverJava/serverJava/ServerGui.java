package serverJava;

import Utils.Rect;

import javax.swing.*;
import javax.swing.border.TitledBorder;
import java.awt.*;
import java.util.List;

/**
 * Swing-based admin GUI for the server. Allows selecting a target player slot,
 * choosing an entity type (crocodile/fruit) and variant, picking a discrete
 * position (1..5), and spawning/removing entities on a selected vine/platform.
 */
public class ServerGui {

    /** Backing game server providing level data and spawn/remove actions. */
    private final GameServer server;
    /** Main window frame. */
    private final JFrame frame;
    /** Player slot selector (1 or 2) for the target player. */
    private final JComboBox<Integer> clientCombo;

    /** Entity type toggle: crocodile. */
    private final JRadioButton crocRadio;
    /** Entity type toggle: fruit. */
    private final JRadioButton fruitRadio;
    /** Variant selector for the chosen entity type. */
    private final JComboBox<String> variantCombo;

    /** Discrete position (1..5) along a container (vine/platform). */
    private final JComboBox<Integer> posCombo;

    /** Container the admin is targeting. */
    private enum Target { NONE, VINE, PLATFORM }
    private Target selectedTarget = Target.NONE;
    /** Index of selected container in its list. */
    private int selectedIndex = -1;

    /** Action button to send a spawn request. */
    private final JButton sendBtn;
    /** Action button to remove a fruit at the selected container/position. */
    private final JButton deleteFruitBtn;

    /**
     * Constructs the admin GUI and wires UI actions.
     * @param server bound {@link GameServer} instance
     */
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

        // Position
        posCombo = new JComboBox<>(new Integer[]{1,2,3,4,5});
        top.add(new JLabel("  Position:"));
        top.add(posCombo);

        // Actions
        sendBtn = new JButton("Send");
        deleteFruitBtn = new JButton("Delete fruit");

        sendBtn.addActionListener(e -> doSend());
        deleteFruitBtn.addActionListener(e -> doDeleteFruit());

        top.add(sendBtn);
        top.add(deleteFruitBtn);

        frame.add(top, BorderLayout.NORTH);

        // ---------- CENTER ----------
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

    /**
     * Stores the selected container and notifies the user.
     * @param t   target container type
     * @param idx index within the corresponding list
     */
    private void selectContainer(Target t, int idx){
        this.selectedTarget = t;
        this.selectedIndex  = idx;
        String name = (t==Target.VINE) ? ("Vine " + idx) : (t==Target.PLATFORM ? ("Plat " + idx) : "(none)");
        JOptionPane.showMessageDialog(frame, "Selected: " + name);
    }

    /**
     * Sends a spawn command (croc/fruit) to the server for the chosen
     * player slot, container, variant, and discrete position.
     */
    private void doSend(){
        Integer slotIndex = (Integer) clientCombo.getSelectedItem();
        if (slotIndex == null) {
            msg("No player selected");
            return;
        }
        if (selectedTarget == Target.NONE || selectedIndex < 0) {
            msg("Select a Vine/Platform first");
            return;
        }

        Integer playerClientId = server.getPlayerClientIdForSlot(slotIndex);
        if (playerClientId == null) {
            msg("No player connected in slot " + slotIndex);
            return;
        }

        byte variant = resolveVariantCode();
        int pos = (Integer) posCombo.getSelectedItem();

        switch (selectedTarget){
            case VINE -> {
                if (crocRadio.isSelected())
                    server.spawnCrocOnVineForClient(playerClientId, selectedIndex, variant, pos);
                else
                    server.spawnFruitOnVineForClient(playerClientId, selectedIndex, variant, pos);
            }
            case PLATFORM -> {
                if (crocRadio.isSelected())
                    server.spawnCrocOnPlatformForClient(playerClientId, selectedIndex, variant, pos);
                else
                    server.spawnFruitOnPlatformForClient(playerClientId, selectedIndex, variant, pos);
            }
            default -> {}
        }
    }

    /**
     * Issues a fruit removal at the selected container and position for
     * the chosen player slot.
     */
    private void doDeleteFruit(){
        Integer slotIndex = (Integer) clientCombo.getSelectedItem();
        if (slotIndex == null) {
            msg("No player selected");
            return;
        }
        if (selectedTarget == Target.NONE || selectedIndex < 0) {
            msg("Select a Vine/Platform first");
            return;
        }

        Integer playerClientId = server.getPlayerClientIdForSlot(slotIndex);
        if (playerClientId == null) {
            msg("No player connected in slot " + slotIndex);
            return;
        }

        int pos = (Integer) posCombo.getSelectedItem();

        switch (selectedTarget){
            case VINE     -> server.removeFruitOnVineForClient(playerClientId, selectedIndex, pos);
            case PLATFORM -> server.removeFruitOnPlatformForClient(playerClientId, selectedIndex, pos);
            default       -> {}
        }
    }

    /** Loads crocodile variants into the variant combo box. */
    private void loadVariantsForCroc(){
        variantCombo.removeAllItems();
        variantCombo.addItem("RED");
        variantCombo.addItem("BLUE");
        variantCombo.setSelectedIndex(0);
    }

    /** Loads fruit variants into the variant combo box. */
    private void loadVariantsForFruit(){
        variantCombo.removeAllItems();
        variantCombo.addItem("BANANA");
        variantCombo.addItem("APPLE");
        variantCombo.addItem("ORANGE");
        variantCombo.setSelectedIndex(0);
    }

    /**
     * Resolves the selected variant name to its protocol code using Utils enums.
     * @return variant byte code
     */
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

    /**
     * Refreshes the player-slot combo based on current server state.
     * Adds slot numbers (1/2) that currently have an active player.
     */
    private void refreshClients(){
        clientCombo.removeAllItems();

        if (server.getPlayerClientIdForSlot(1) != null) {
            clientCombo.addItem(1);
        }
        if (server.getPlayerClientIdForSlot(2) != null) {
            clientCombo.addItem(2);
        }
    }

    /** Shows a modal message dialog. */
    private void msg(String m){ JOptionPane.showMessageDialog(frame, m); }

    /** Displays the admin window. */
    public void show(){ frame.setVisible(true); }
}
