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

    public ServerGui(GameServer server) {
        this.server = server;

        frame = new JFrame("DonCEy Kong Jr - Trapper Control");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setLayout(new BorderLayout());

        // ---- Top: client selection + refresh ----
        JPanel top = new JPanel(new FlowLayout(FlowLayout.LEFT));
        top.add(new JLabel("Target client:"));

        clientCombo = new JComboBox<>();
        top.add(clientCombo);

        JButton refreshBtn = new JButton("Refresh clients");
        refreshBtn.addActionListener(e -> refreshClients());
        top.add(refreshBtn);

        frame.add(top, BorderLayout.NORTH);

        // ---- Center: buttons for vines + platforms ----
        JPanel center = new JPanel(new GridLayout(2, 1));

        // Vines panel
        JPanel vinesPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        vinesPanel.setBorder(new TitledBorder("Vines"));
        List<Rect> vines = server.getVines();
        for (int i = 0; i < vines.size(); i++) {
            int idx = i;
            JButton b = new JButton("Vine " + i);
            b.addActionListener(e -> spawnOnVine(idx));
            vinesPanel.add(b);
        }

        // Platforms panel
        JPanel platformsPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        platformsPanel.setBorder(new TitledBorder("Platforms"));
        List<Rect> plats = server.getPlatforms();
        for (int i = 0; i < plats.size(); i++) {
            int idx = i;
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

    private void refreshClients() {
        clientCombo.removeAllItems();
        for (Integer id : server.getClientIdsSnapshot()) {
            clientCombo.addItem(id);
        }
    }

    private Integer getSelectedClientId() {
        return (Integer) clientCombo.getSelectedItem();
    }

    private void spawnOnVine(int vineIndex) {
        Integer clientId = getSelectedClientId();
        if (clientId == null) {
            System.out.println("GUI: no client selected");
            JOptionPane.showMessageDialog(frame, "No client selected", "Error", JOptionPane.ERROR_MESSAGE);
            return;
        }
        server.spawnCrocOnVineForClient(clientId, vineIndex);
    }

    private void spawnOnPlatform(int platformIndex) {
        Integer clientId = getSelectedClientId();
        if (clientId == null) {
            System.out.println("GUI: no client selected");
            JOptionPane.showMessageDialog(frame, "No client selected", "Error", JOptionPane.ERROR_MESSAGE);
            return;
        }
        server.spawnCrocOnPlatformForClient(clientId, platformIndex);
    }

    public void show() {
        frame.setVisible(true);
    }
}
