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
        vines.add(new Rect( 20,  73,  2, 127));
        vines.add(new Rect( 44,  73,  2, 119));
        vines.add(new Rect( 68, 121,  2,  79));
        vines.add(new Rect(108,  73,  2, 103));
        vines.add(new Rect(140,  73,  2,  71));
        vines.add(new Rect(154,  81,  2,  95));
        vines.add(new Rect(188,  81,  2,  79));
        vines.add(new Rect(212,  49,  2, 127));
        vines.add(new Rect(236,  49,  2, 127));
        vines.add(new Rect(156,  33,  2,   7));
        vines.add(new Rect(108,  33,  2,  15));

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

    public void spawnCrocOnVineForClient(int clientId, int vineIndex, byte variant, int pos){
        Rect v = vines.get(vineIndex);
        int x = v.x() + v.w()/2;
        int y = quantizeCenterY(v, pos);
        ClientHandler h = clients.get(clientId);
        if (h != null) h.sendSpawnCroc(variant, x, y);
    }
    public void spawnFruitOnVineForClient(int clientId, int vineIndex, byte variant, int pos){
        Rect v = vines.get(vineIndex);
        int x = v.x() + v.w()/2;
        int y = quantizeCenterY(v, pos);
        ClientHandler h = clients.get(clientId);
        if (h != null) h.sendSpawnFruit(variant, x, y);
    }
    public void spawnCrocOnPlatformForClient(int clientId, int platIndex, byte variant, int pos){
        Rect p = platforms.get(platIndex);
        int x = quantizeCenterX(p, pos);
        int y = p.y() - 8; // ajusta si quieres el centro: p.y()+p.h()/2
        ClientHandler h = clients.get(clientId);
        if (h != null) h.sendSpawnCroc(variant, x, y);
    }
    public void spawnFruitOnPlatformForClient(int clientId, int platIndex, byte variant, int pos){
        Rect p = platforms.get(platIndex);
        int x = quantizeCenterX(p, pos);
        int y = p.y() - 8;
        ClientHandler h = clients.get(clientId);
        if (h != null) h.sendSpawnFruit(variant, x, y);
    }
    public void removeFruitOnVineForClient(int clientId, int vineIndex, int pos){
        Rect v = vines.get(vineIndex);
        int x = v.x() + v.w()/2;
        int y = quantizeCenterY(v, pos);
        ClientHandler h = clients.get(clientId);
        if (h != null) h.sendRemoveFruit(x, y);
    }
    public void removeFruitOnPlatformForClient(int clientId, int platIndex, int pos){
        Rect p = platforms.get(platIndex);
        int x = quantizeCenterX(p, pos);
        int y = p.y() - 8;
        ClientHandler h = clients.get(clientId);
        if (h != null) h.sendRemoveFruit(x, y);
    }



    private static final int N_FIXED = 5;

    private static int quantizeCenterY(Rect r, int pos /*1..5*/){
        double s = r.h() / (double)N_FIXED;
        return r.y() + (int)Math.round((pos - 0.5) * s);
    }
    private static int quantizeCenterX(Rect r, int pos /*1..5*/){
        double s = r.w() / (double)N_FIXED;
        return r.x() + (int)Math.round((pos - 0.5) * s);
    }


    public static void main(String[] args) throws Exception {
        int port = (args.length > 0) ? Integer.parseInt(args[0]) : 9090;
        new GameServer(port).start();
    }

    
}
