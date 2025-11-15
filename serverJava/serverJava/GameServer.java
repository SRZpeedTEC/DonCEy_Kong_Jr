package serverJava;

import java.io.*;
import java.net.*;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicInteger;

import Utils.Rect;
import Classes.Player.player;

public class GameServer {

    private final int port;
    private ServerSocket serverSocket;

    // Connected clients
    private final ConcurrentHashMap<Integer, ClientHandler> clients = new ConcurrentHashMap<>();
    private final AtomicInteger idGen = new AtomicInteger(1);

    // ---- Level & game state ----

    // These were being used directly as server.platforms, server.vines, etc.
    // Make them public so MessageManagement.* can access them.
    public final List<Rect> platforms = new ArrayList<>();
    public final List<Rect> vines      = new ArrayList<>();
    public final List<Rect> waters     = new ArrayList<>();

    // crocodiles & fruits were referenced in Messenger
    public final List<Rect> crocodiles    = new ArrayList<>();
    public final List<Rect> fruits     = new ArrayList<>();

    // Player rectangle used in Messenger.sendInitStaticLegacy
    public Rect player = new Rect(0, 0, 0, 0);

    // p1 is the logical player object used in AnswerProcessor
    // AnswerProcessor sets p1.x, p1.y, p1.vx, p1.vy
    public player p1 = null;

    public GameServer(int port) {
        this.port = port;
        initLevel();
    }

    // ---- Your initial game board ----
    private void initLevel() {
        // grass platforms
        platforms.add(new Rect(  1, 215,  71, 8));
        platforms.add(new Rect( 97, 199,  31, 8));
        platforms.add(new Rect(137, 207,  23, 8));
        platforms.add(new Rect(169, 199,  31, 8));
        platforms.add(new Rect(209, 191,  31, 8));

        // wood platforms
        platforms.add(new Rect( 49, 152,  47, 8));
        platforms.add(new Rect( 49, 112,  31, 8));
        platforms.add(new Rect(193, 136,  63, 8));
        platforms.add(new Rect(145,  72,  62 , 8));
        platforms.add(new Rect(  1,  64, 151, 8));

        //vines
        vines.add(new Rect(20, 73, 2, 127));
        
        
        
        vines.add(new Rect(20, 73, 2, 127));

        vines.add(new Rect( 40, 40, 6, 168));
        vines.add(new Rect( 88, 56, 6, 152));
        vines.add(new Rect(128, 40, 6, 168));
        vines.add(new Rect(168, 48, 6, 160));
        vines.add(new Rect(200, 40, 6, 168));
        vines.add(new Rect(232, 40, 6, 168));
        vines.add(new Rect(0, 40, 6, 168));

        waters.add(new Rect(0, 224, 256, 16));
        

        // If you want an initial visible position for the player rect:
        // (example values — use whatever your game logic expects)
        player = new Rect(16, 200, 16, 16);
    }

    // Optional getters (you can keep them or remove them; Messenger uses the public fields)
    public List<Rect> getPlatforms() { return platforms; }
    public List<Rect> getVines()     { return vines; }
    public List<Rect> getWaters()    { return waters; }
    public List<Rect> getcrocodiles()   { return crocodiles; }
    public List<Rect> getFruits()    { return fruits; }
    public List<Integer> getClientIdsSnapshot() {
        return new ArrayList<>(clients.keySet());
    }

    private void adminLoop() {
        try (BufferedReader br = new BufferedReader(new InputStreamReader(System.in))) {
            String line;
            while ((line = br.readLine()) != null) {
                line = line.trim();
                if (line.equalsIgnoreCase("list")) {
                    if (clients.isEmpty()) { System.out.println("(no clients)"); continue; }
                    clients.forEach((id, h) -> System.out.println("id=" + id + " remote=" + h.getRemote()));
                }
                else if (line.startsWith("croc ")) {
                    // croc <clientId> <variant> <x> <y>
                    try {
                        String[] p = line.split("\\s+");
                        if (p.length != 5) { System.out.println("Usage: croc <clientId> <RED|BLUE|1|2> <x> <y>"); continue; }
                        int id = Integer.parseInt(p[1]);
                        byte variant = parseCrocVariant(p[2]);
                        int x = Integer.parseInt(p[3]);
                        int y = Integer.parseInt(p[4]);
                        ClientHandler h = clients.get(id);
                        if (h == null) { System.out.println("No such client: " + id); continue; }
                        h.sendSpawnCroc(variant, x, y);
                        System.out.printf("Sent CROC_SPAWN v=%d to client %d at (%d,%d)%n", variant, id, x, y);
                    } catch (Exception e) {
                        System.out.println("Usage: croc <clientId> <RED|BLUE|1|2> <x> <y>");
                    }
                }
                else if (line.startsWith("fruit ")) {
                    // fruit <clientId> <variant> <x> <y>
                    try {
                        String[] p = line.split("\\s+");
                        if (p.length != 5) { System.out.println("Usage: fruit <clientId> <BANANA|APPLE|ORANGE|1|2|3> <x> <y>"); continue; }
                        int id = Integer.parseInt(p[1]);
                        byte variant = parseFruitVariant(p[2]);
                        int x = Integer.parseInt(p[3]);
                        int y = Integer.parseInt(p[4]);
                        ClientHandler h = clients.get(id);
                        if (h == null) { System.out.println("No such client: " + id); continue; }
                        h.sendSpawnFruit(variant, x, y);
                        System.out.printf("Sent FRUIT_SPAWN v=%d to client %d at (%d,%d)%n", variant, id, x, y);
                    } catch (Exception e) {
                        System.out.println("Usage: fruit <clientId> <BANANA|APPLE|ORANGE|1|2|3> <x> <y>");
                    }
                }
                else if (line.equalsIgnoreCase("help")) {
                    System.out.println("Commands:");
                    System.out.println("  list");
                    System.out.println("  croc  <clientId> <RED|BLUE|1|2> <x> <y>");
                    System.out.println("  fruit <clientId> <BANANA|APPLE|ORANGE|1|2|3> <x> <y>");
                    System.out.println("  help");
                }
            }
        } catch (IOException ignored) {}
    }

    // Parsers auxiliares:
    private static byte parseCrocVariant(String token){
        token = token.toUpperCase();
        switch (token){
            case "RED":  return Utils.CrocVariant.RED.code;
            case "BLUE": return Utils.CrocVariant.BLUE.code;
            default:
                try { return (byte)Integer.parseInt(token); } catch(Exception e){ return 0; }
        }
    }
    private static byte parseFruitVariant(String token){
        token = token.toUpperCase();
        switch (token){
            case "BANANA": return Utils.FruitVariant.BANANA.code;
            case "APPLE":  return Utils.FruitVariant.APPLE.code;
            case "ORANGE": return Utils.FruitVariant.ORANGE.code;
            default:
                try { return (byte)Integer.parseInt(token); } catch(Exception e){ return 0; }
        }
    }


    public void start() throws IOException {
        serverSocket = new ServerSocket(port);
        System.out.println("Server listening on port " + port);

        // Start admin console (if you still use text commands)
        Thread admin = new Thread(this::adminLoop, "admin-loop");
        admin.setDaemon(true);
        admin.start();

        // Start GUI on the Swing event thread
        javax.swing.SwingUtilities.invokeLater(() -> {
            new ServerGui(this).show();
        });

        while (true) {
            Socket socket = serverSocket.accept();
            socket.setTcpNoDelay(true);

            int clientId = idGen.getAndIncrement();
            ClientHandler handler = new ClientHandler(clientId, socket, this);
            clients.put(clientId, handler);
            handler.start();

            System.out.println("Client connected, id=" + clientId + " from " + socket.getRemoteSocketAddress());
        }
    }

    // Called by ClientHandler when a client disconnects
    public void removeClient(int id) {
        clients.remove(id);
        System.out.println("Client " + id + " disconnected.");
    }

        // Spawn a croc at the center of the given vine for a specific client
    public void spawnCrocOnVineForClient(int clientId, int vineIndex, byte variant) {
        ClientHandler h = clients.get(clientId);
        if (h == null) {
            System.out.println("GUI: no client " + clientId);
            return;
        }
        if (vineIndex < 0 || vineIndex >= vines.size()) {
            System.out.println("GUI: invalid vine index " + vineIndex);
            return;
        }
        Rect r = vines.get(vineIndex);
        int x = r.x() + r.w() / 2;
        int y = r.y() + r.h() / 2;
        h.sendSpawnCroc(variant, x, y);   
    }

    // Optional: spawn croc on a platform instead of vine
    public void spawnCrocOnPlatformForClient(int clientId, int platformIndex, byte variant) {
        ClientHandler h = clients.get(clientId);
        if (h == null) {
            System.out.println("GUI: no client " + clientId);
            return;
        }
        if (platformIndex < 0 || platformIndex >= platforms.size()) {
            System.out.println("GUI: invalid platform index " + platformIndex);
            return;
        }
        Rect r = platforms.get(platformIndex);
        int x = r.x() + r.w() / 2;
        int y = r.y() + r.h() / 2;
        h.sendSpawnCroc(variant, x, y);
    }

    // Spawn a fruit at the center of the given vine for a specific client
    public void spawnFruitOnVineForClient(int clientId, int vineIndex, byte variant) {
        ClientHandler h = clients.get(clientId);
        if (h == null) {
            System.out.println("GUI: no client " + clientId);
            return;
        }
        if (vineIndex < 0 || vineIndex >= vines.size()) {
            System.out.println("GUI: invalid vine index " + vineIndex);
            return;
        }
        Rect r = vines.get(vineIndex);
        int x = r.x() + r.w() / 2;
        int y = r.y() + r.h() / 2;

        // TODO: change this to your real method name if needed
        h.sendSpawnFruit(variant, x, y);
    }

    // Spawn a fruit at the center of the given platform for a specific client
    public void spawnFruitOnPlatformForClient(int clientId, int platformIndex, byte variant) {
        ClientHandler h = clients.get(clientId);
        if (h == null) {
            System.out.println("GUI: no client " + clientId);
            return;
        }
        if (platformIndex < 0 || platformIndex >= platforms.size()) {
            System.out.println("GUI: invalid platform index " + platformIndex);
            return;
        }
        Rect r = platforms.get(platformIndex);
        int x = r.x() + r.w() / 2;
        int y = r.y() + r.h() / 2;

        // TODO: change this to your real method name if needed
        h.sendSpawnFruit(variant, x, y);
    }

    public static void main(String[] args) throws Exception {
        int port = (args.length > 0) ? Integer.parseInt(args[0]) : 9090;
        new GameServer(port).start();
    }

    
}
