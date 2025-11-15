package serverJava;

import java.io.*;
import java.net.*;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicInteger;

import Utils.Rect;
import Classes.Player.player;   // <- this is the class that AnswerProcessor uses

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

    // Enemies & fruits were referenced in Messenger
    public final List<Rect> enemies    = new ArrayList<>();
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
        
        /* 
        
        vines.add(new Rect(20, 73, 2, 127));

        vines.add(new Rect( 40, 40, 6, 168));
        vines.add(new Rect( 88, 56, 6, 152));
        vines.add(new Rect(128, 40, 6, 168));
        vines.add(new Rect(168, 48, 6, 160));
        vines.add(new Rect(200, 40, 6, 168));
        vines.add(new Rect(232, 40, 6, 168));
        vines.add(new Rect(0, 40, 6, 168));

        waters.add(new Rect(0, 224, 256, 16));
        /* */

        // If you want an initial visible position for the player rect:
        // (example values — use whatever your game logic expects)
        player = new Rect(16, 200, 16, 16);
    }

    // Optional getters (you can keep them or remove them; Messenger uses the public fields)
    public List<Rect> getPlatforms() { return platforms; }
    public List<Rect> getVines()     { return vines; }
    public List<Rect> getWaters()    { return waters; }
    public List<Rect> getEnemies()   { return enemies; }
    public List<Rect> getFruits()    { return fruits; }

    private void adminLoop() {
        try (BufferedReader br = new BufferedReader(new InputStreamReader(System.in))) {
            String line;
            while ((line = br.readLine()) != null) {
                line = line.trim();
                if (line.equalsIgnoreCase("list")) {
                    if (clients.isEmpty()) { System.out.println("(no clients)"); continue; }
                    clients.forEach((id, h) ->
                        System.out.println("id=" + id + " remote=" + h.getRemote()));
                }
                else if (line.startsWith("croc ")) {
                    try {
                        String[] parts = line.split("\\s+");
                        if (parts.length != 4) {
                            System.out.println("Usage: croc <clientId> <x> <y>");
                            continue;
                        }

                        int id = Integer.parseInt(parts[1]);
                        int x  = Integer.parseInt(parts[2]);
                        int y  = Integer.parseInt(parts[3]);

                        ClientHandler h = clients.get(id);
                        if (h == null) {
                            System.out.println("No such client: " + id);
                            continue;
                        }

                        h.sendSpawnCroc(x, y);
                        System.out.println("Sent CROC_SPAWN to client " + id + " at (" + x + "," + y + ")");
                    } catch (Exception e) {
                        System.out.println("Usage: croc <clientId> <x> <y>");
                    }
                }
                else if (line.equalsIgnoreCase("help")) {
                    System.out.println("Commands: list | croc <clientId> <x> <y> | help");
                }
            }
        } catch (IOException ignored) {}
    }

    public void start() throws IOException {
        serverSocket = new ServerSocket(port);
        System.out.println("Server listening on port " + port);

        // start admin console in background
        Thread admin = new Thread(this::adminLoop, "admin-loop");
        admin.setDaemon(true);
        admin.start();


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

    public static void main(String[] args) throws Exception {
        int port = (args.length > 0) ? Integer.parseInt(args[0]) : 9090;
        new GameServer(port).start();
    }


}
