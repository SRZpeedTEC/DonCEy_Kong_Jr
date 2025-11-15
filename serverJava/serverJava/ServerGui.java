package serverJava;

import Utils.Rect;

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

    public ServerGui(GameServer server) {
        this.server = server;

        frame = new JFrame("DonCEy Kong Jr - Trapper Control");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setLayout(new BorderLayout());

        // ---------- TOP PANEL: client selection + mode ----------
        JPanel top = new JPanel(new FlowLayout(FlowLayout.LEFT));

        top.add(new JLabel("Target client:"));

        clientCombo = new JComboBox<>();
        top.add(clientCombo);

        JButton refreshBtn = new JButton("Refresh clients");
        refreshBtn.addActionListener(e -> refreshClients());
        top.add(refreshBtn);

        // Mode selection: Crocodile vs Fruit
        crocRadio = new JRadioButton("Crocodile", true);
        fruitRadio = new JRadioButton("Fruit");

        ButtonGroup modeGroup = new ButtonGroup();
        modeGroup.add(crocRadio);
        modeGroup.add(fruitRadio);

        top.add(new JLabel("  Mode:"));
        top.add(crocRadio);
        top.add(fruitRadio);

        frame.add(top, BorderLayout.NORTH);

        // ---------- CENTER PANEL: vines & platforms buttons ----------
        JPanel center = new JPanel(new GridLayout(2, 1));

        // Vines panel
        JPanel vinesPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        vinesPanel.setBorder(new TitledBorder("Vines"));
        List<Rect> vines = server.getVines();
        for (int i = 0; i < vines.size(); i++) {
            final int idx = i;
            JButton b = new JButton("Vine " + i);
            b.addActionListener(e -> spawnOnVine(idx));
            vinesPanel.add(b);
        }

        // Platforms panel
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

        // Initial population of clients
        refreshClients();
    }

    // --- Helpers ---

    private void refreshClients() {
        clientCombo.removeAllItems();
        for (Integer id : server.getClientIdsSnapshot()) {
            clientCombo.addItem(id);
        }
    }

    private Integer getSelectedClientId() {
        return (Integer) clientCombo.getSelectedItem();
    }

    // Called when user clicks a "Vine N" button
    private void spawnOnVine(int vineIndex) {
        Integer clientId = getSelectedClientId();
        if (clientId == null) {
            System.out.println("GUI: no client selected");
            JOptionPane.showMessageDialog(frame, "No client selected", "Error", JOptionPane.ERROR_MESSAGE);
            return;
        }

        if (crocRadio.isSelected()) {
            server.spawnCrocOnVineForClient(clientId, vineIndex);
        } else {
            server.spawnFruitOnVineForClient(clientId, vineIndex);
        }
    }

    // Called when user clicks a "Plat N" button
    private void spawnOnPlatform(int platformIndex) {
        Integer clientId = getSelectedClientId();
        if (clientId == null) {
            System.out.println("GUI: no client selected");
            JOptionPane.showMessageDialog(frame, "No client selected", "Error", JOptionPane.ERROR_MESSAGE);
            return;
        }

        if (crocRadio.isSelected()) {
            server.spawnCrocOnPlatformForClient(clientId, platformIndex);
        } else {
            server.spawnFruitOnPlatformForClient(clientId, platformIndex);
        }
    }

    public void show() {
        frame.setVisible(true);
    }
}
