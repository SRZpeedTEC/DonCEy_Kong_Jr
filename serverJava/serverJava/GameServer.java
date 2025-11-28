package serverJava;

import java.io.*;
import java.net.*;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.function.Consumer;

import Utils.Rect;
import Classes.Player.player;
import serverJava.EntityState;
import MessageManagement.Proto;
import Utils.MsgType;

public class GameServer {

    private final int port;
    private ServerSocket serverSocket;

    private static final int MAX_PLAYERS = 2;
    private static final int MAX_SPECTATORS_PER_PLAYER = 2;
    private static final int MAX_TOTAL_SPECTATORS = MAX_PLAYERS * MAX_SPECTATORS_PER_PLAYER; // 4


    // Connected clients
    private final ConcurrentHashMap<Integer, ClientHandler> clients = new ConcurrentHashMap<>();
    private final AtomicInteger idGen = new AtomicInteger(1);

    // Map playerId -> player handler
    private final Map<Integer, ClientHandler> players = new ConcurrentHashMap<>();

    // Map playerId -> list of spectator observers
    private final Map<Integer, List<ClientHandler>> spectatorsByPlayer = new ConcurrentHashMap<>();

    private final ConcurrentHashMap<Integer, player> playerStates = new ConcurrentHashMap<>();

    // ---- Level & game state ----

    // These were being used directly as server.platforms, server.vines, etc.
    // Make them public so MessageManagement.* can access them.
    public final List<Rect> platforms = new ArrayList<>();
    public final List<Rect> vines      = new ArrayList<>();
    public final List<Rect> waters     = new ArrayList<>();

    // legacy (para INIT_STATIC y compatibilidad con Messenger)
    public final List<Rect> crocodiles = new ArrayList<>();
    public final List<Rect> fruits     = new ArrayList<>();

  
    public final List<EntityState> crocodileStates = new ArrayList<>();
    public final List<EntityState> fruitStates     = new ArrayList<>();


    static final int CROC_W  = 8;
    static final int FRUIT_W = 8;


    private Integer playerSlot1 = null;
    private Integer playerSlot2 = null;

    // Player rectangle used in Messenger.sendInitStaticLegacy
    public Rect player = new Rect(0, 0, 0, 0);


    public GameServer(int port) {
        this.port = port;
        initLevel();
    }

    // ---- initial game board ----
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

        vines.add(new Rect( 68, 121,  2, 31));
        vines.add(new Rect( 68, 160,  2, 40));

        vines.add(new Rect(108,  73,  2, 103));
        vines.add(new Rect(140,  73,  2,  71));
        vines.add(new Rect(164,  81,  2,  95));
        vines.add(new Rect(188,  81,  2,  79));

        vines.add(new Rect(212,  49,  2, 87));
        vines.add(new Rect(212, 144,  2, 32));

        vines.add(new Rect(236,  49,  2, 87));
        vines.add(new Rect(236, 144,  2, 32));

        vines.add(new Rect(156,  33,  2,   7));
        vines.add(new Rect(108,  33,  2,  15));

        
    }

    // Optional getters 
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

    public synchronized void clearEntitiesForNewRound() {
        crocodiles.clear();
        fruits.clear();
        crocodileStates.clear();
        fruitStates.clear();
    }

    public synchronized void cleanupOffScreenCrocodiles() {
        // Remove crocodiles that are below the screen (y > 240)
        crocodileStates.removeIf(c -> c.y > 240);
        crocodiles.removeIf(r -> r.y() > 240);
    }


    private Integer choosePlayerForSpectator() {
        Integer best = null;
        int bestCount = Integer.MAX_VALUE;

        for (Map.Entry<Integer, ClientHandler> e : players.entrySet()) {
            int pid = e.getKey();
            List<ClientHandler> specs = spectatorsByPlayer.get(pid);
            int count = (specs == null) ? 0 : specs.size();
            if (count < 2 && count < bestCount) {
                bestCount = count;
                best = pid;
            }
        }
        return best;
    }

    public player getPlayerFromServer(int clientId) {
        return playerStates.computeIfAbsent(clientId, id -> {
                player p = new player(0, 0);
                p.setLives(3);
                p.setScore(0);
                return p;
        });
    }

    public synchronized boolean attachSpectatorToSlot(int spectatorClientId, int slotIndex) {
        ClientHandler spectator = clients.get(spectatorClientId);
        if (spectator == null) {
            return false; // no such client
        }
        if (spectator.getRole() != ClientRole.SPECTATOR) {
            return false; // not a spectator
        }

        Integer targetPlayerId = null;
        if (slotIndex == 1) {
            targetPlayerId = playerSlot1;
        } else if (slotIndex == 2) {
            targetPlayerId = playerSlot2;
        } else {
            return false; // invalid slot index
        }

        if (targetPlayerId == null) {
            // no hay player en ese slot
            return false;
        }

        List<ClientHandler> specs = spectatorsByPlayer
                .computeIfAbsent(targetPlayerId,
                        k -> Collections.synchronizedList(new ArrayList<>()));

        if (specs.size() >= 2) {
            // slot full
            return false;
        }

        specs.add(spectator);
        spectator.setObservedPlayerId(targetPlayerId);
       
        for (EntityState c : crocodileStates) {
            spectator.sendSpawnCroc(c.variant, c.x, c.y);
        }
        for (EntityState f : fruitStates) {
            spectator.sendSpawnFruit(f.variant, f.x, f.y);
        }

        player pState = getPlayerFromServer(targetPlayerId); 
        byte lives = (byte) pState.getLives();
        spectator.sendLivesUpdate(lives);
        int score = pState.getScore();
        spectator.sendScoreUpdate(score);


        System.out.println("Spectator " + spectatorClientId
                + " attached to player " + targetPlayerId
                + " (slot " + slotIndex + ")");
        return true;
    }

    public void broadcastRespawnDeathToGroup(int playerId){
        sendToPlayerGroup(playerId, ClientHandler::sendRespawnDeath);
    }

    public void broadcastRespawnWinToGroup(int playerId){
        sendToPlayerGroup(playerId, ClientHandler::sendRespawnWin);
    }

    public void broadcastGameOverToGroup(int playerId){
        sendToPlayerGroup(playerId, ClientHandler::sendGameOver);
    }
    public void broadcastLivesUpdateToGroup(int playerId, byte lives) {
        sendToPlayerGroup(playerId, h -> h.sendLivesUpdate(lives));
    }

    public void broadcastScoreUpdateToGroup(int playerId, int score) {
        sendToPlayerGroup(playerId, h -> h.sendScoreUpdate(score));
    }

    public void broadcastCrocSpeedIncreaseToGroup(int playerId) {
        sendToPlayerGroup(playerId, ClientHandler::sendCrocSpeedIncrease);
    }

    public void broadcastGameRestartToGroup(int playerId) {
        sendToPlayerGroup(playerId, ClientHandler::sendGameRestart);
    }

    public void resetCrocodileSpeed() {
        // This resets the server-side crocodile speed tracking
        // The actual speed is managed client-side, so we just need to ensure
        // the server state is consistent
        System.out.println("Crocodile speed reset to default");
    }


    public void start() throws IOException {
        serverSocket = new ServerSocket(port);
        System.out.println("Server listening on port " + port);

        // Start admin console 
        Thread admin = new Thread(this::adminLoop, "admin-loop");
        admin.setDaemon(true);
        admin.start();

        // Start GUI on the Swing event thread
        javax.swing.SwingUtilities.invokeLater(() -> {
            new ServerGui(this).show();
        });

        Thread cleanupThread = new Thread(() -> {
            while (true) {
                try {
                    Thread.sleep(1000); // Clean up every second
                    cleanupOffScreenCrocodiles();
                } catch (InterruptedException e) {
                    break;
                }
            }
        }, "cleanup-thread");
        cleanupThread.setDaemon(true);
        cleanupThread.start();

        while (true) {
            Socket socket = serverSocket.accept();
            socket.setTcpNoDelay(true);

            int clientId = idGen.getAndIncrement();

            // Read requested role byte
            int requestedRoleRaw;
            try {
                requestedRoleRaw = socket.getInputStream().read();
            } catch (IOException e) {
                System.out.println("Failed to read requested role from client: " + e.getMessage());
                try { socket.close(); } catch (IOException ignore) {}
                continue;
            }
            if (requestedRoleRaw == -1) {
                System.out.println("Client disconnected before sending requested role");
                try { socket.close(); } catch (IOException ignore) {}
                continue;
            }

            boolean wantsPlayer    = (requestedRoleRaw == 1);
            boolean wantsSpectator = (requestedRoleRaw == 2);

            if (!wantsPlayer && !wantsSpectator) {
                System.out.println("Client " + clientId + " sent invalid requested role: " + requestedRoleRaw);
                try { socket.close(); } catch (IOException ignore) {}
                continue;
            }

            ClientRole role = null;
            Integer observedPlayerId = null;

            // Check capacity before creating ClientHandler
            synchronized (this) {
                if (wantsPlayer) {
                    // client asks to be player
                    Integer freeSlot = null;
                    if (playerSlot1 == null) {
                        freeSlot = 1;
                    } else if (playerSlot2 == null) {
                        freeSlot = 2;
                    }

                    if (freeSlot != null) {
                        role = ClientRole.PLAYER;
                        observedPlayerId = null;

                        // Reserve the slot immediately
                        if (freeSlot == 1) {
                            playerSlot1 = clientId;
                        } else {
                            playerSlot2 = clientId;
                        }

                        System.out.println("Client " + clientId
                                + " requested PLAYER -> assigned PLAYER in slot " + freeSlot);
                    } else {
                        
                        System.out.println("Client " + clientId
                                + " requested PLAYER but no slot free. Closing.");
                    }
                } else if (wantsSpectator) {
                    // client asks to be spectator
                    if (players.isEmpty()) {
                        // there must be at least one player
                        System.out.println("Client " + clientId
                                + " requested SPECTATOR but no players connected. Rejecting.");
                    } else {
                        // Count ALL connected spectators (from clients map)
                        long totalSpectators = clients.values().stream()
                                .filter(h -> h.getRole() == ClientRole.SPECTATOR)
                                .count();

                        if (totalSpectators >= MAX_TOTAL_SPECTATORS) {
                            System.out.println("Client " + clientId
                                    + " requested SPECTATOR but max spectators reached ("
                                    + totalSpectators + "/" + MAX_TOTAL_SPECTATORS + "). Rejecting.");
                        } else {
                            // Additional check: At least one player slot must have room
                            boolean hasAvailableSlot = false;
                            
                            for (ClientHandler playerHandler : players.values()) {
                                int playerId = playerHandler.getClientId();
                                List<ClientHandler> specs = spectatorsByPlayer.get(playerId);
                                int currentSpecs = (specs == null) ? 0 : specs.size();
                                
                                if (currentSpecs < MAX_SPECTATORS_PER_PLAYER) {
                                    hasAvailableSlot = true;
                                    break;
                                }
                            }
                            
                            if (!hasAvailableSlot) {
                                System.out.println("Client " + clientId
                                        + " requested SPECTATOR but all player slots are full. Rejecting.");
                            } else {
                                role = ClientRole.SPECTATOR;
                                observedPlayerId = null;
                                System.out.println("Client " + clientId
                                        + " requested SPECTATOR -> assigned SPECTATOR (pending selection) ("
                                        + totalSpectators + "/" + MAX_TOTAL_SPECTATORS + ")");
                            }
                        }
                    }
                }
            }

            // Send CLIENT_ACK before creating ClientHandler
            try {
                DataOutputStream out = new DataOutputStream(
                        new BufferedOutputStream(socket.getOutputStream()));

                byte roleByte = (role != null) 
                    ? (role == ClientRole.PLAYER ? (byte)1 : (byte)2) 
                    : (byte)0;

                Proto.writeHeader(out, MsgType.CLIENT_ACK, clientId, 0, 1);
                out.writeByte(roleByte);
                out.flush();

                if (role == null) {
                    // Rejected - close socket
                    socket.close();
                    continue;
                }

            } catch (IOException e) {
                System.out.println("Failed to send CLIENT_ACK: " + e.getMessage());
                try { socket.close(); } catch (IOException ignore) {}
                
                // If we reserved a player slot, free it
                synchronized (this) {
                    if (role == ClientRole.PLAYER) {
                        if (Objects.equals(playerSlot1, clientId)) playerSlot1 = null;
                        if (Objects.equals(playerSlot2, clientId)) playerSlot2 = null;
                    }
                }
                continue;
            }

            // Now create the ClientHandler 
            ClientHandler handler;
            try {
                handler = new ClientHandler(clientId, socket, this, role, observedPlayerId);
            } catch (IOException e) {
                System.out.println("Failed to create ClientHandler: " + e.getMessage());
                try { socket.close(); } catch (IOException ignore) {}
                
                // Free reserved slot
                synchronized (this) {
                    if (role == ClientRole.PLAYER) {
                        if (Objects.equals(playerSlot1, clientId)) playerSlot1 = null;
                        if (Objects.equals(playerSlot2, clientId)) playerSlot2 = null;
                    }
                }
                continue;
            }

            clients.put(clientId, handler);

            if (role == ClientRole.PLAYER) {
                players.put(clientId, handler);
                spectatorsByPlayer.putIfAbsent(clientId,
                        Collections.synchronizedList(new ArrayList<>()));
            }

            handler.start();
            System.out.println("Client connected, id=" + clientId + " from " + socket.getRemoteSocketAddress());
        }

        

    }

    // Called by ClientHandler when a client disconnects
    public synchronized void removeClient(int clientId) {
        ClientHandler handler = clients.remove(clientId);
        if (handler == null) {
            return; // already removed
        }

        System.out.println("Client " + clientId + " disconnected");

        if (handler.getRole() == ClientRole.PLAYER) {
            // quit this player
            players.remove(clientId);

            
            if (Objects.equals(playerSlot1, clientId)) {
                playerSlot1 = null;
                System.out.println("Freed player slot 1");
            }
            if (Objects.equals(playerSlot2, clientId)) {
                playerSlot2 = null;
                System.out.println("Freed player slot 2");
            }

            
            var specs = spectatorsByPlayer.remove(clientId);
            if (specs != null) {
                for (ClientHandler s : specs) {
                    System.out.println("Detaching spectator " + s.getClientId()
                            + " from player " + clientId);
                    try {
                        s.setObservedPlayerId(null);
                    } catch (Exception ignore) {}
                }
            }

        } else { // SPECTATOR
            
            for (var entry : spectatorsByPlayer.entrySet()) {
                List<ClientHandler> list = entry.getValue();
                if (list != null) {
                    list.remove(handler);
                }
            }
        }
    }

    public Integer getPlayerClientIdForSlot(int slotIndex) {
        return switch (slotIndex) {
            case 1 -> playerSlot1;
            case 2 -> playerSlot2;
            default -> null;
        };
    }


    public void sendToPlayerGroup(int playerId, Consumer<ClientHandler> action) {
    //main player
        ClientHandler player = clients.get(playerId);
        if (player != null) {
            action.accept(player);
        }

        //spectators
        List<ClientHandler> specs = spectatorsByPlayer.get(playerId);
        if (specs != null) {
            for (ClientHandler s : specs) {
                action.accept(s);
            }
        }
    }

    // Broadcast player state to all spectators observing this player
    //param playerClientId The client ID of the player whose state is being broadcasted
    //returns void
    public void broadcastPlayerStateToSpectators(int playerClientId, short x, short y, short vx, short vy, byte flags) {
        List<ClientHandler> specs = spectatorsByPlayer.get(playerClientId);
        if (specs == null || specs.isEmpty()) return;

        for (ClientHandler spectator : specs) {
            spectator.sendSpectatorState(x, y, vx, vy, flags);
        }
    }

    // Helper methods to spawn/remove entities on vines/platforms for a specific client
    // These methods calculate the actual (x,y) coordinates based on vine/platform index and position
    // then send the appropriate spawn/remove message to the specified client


    public void spawnCrocOnVineForClient(int clientId, int vineIndex, byte variant, int pos){
        Rect v = vines.get(vineIndex);
        int x = centerXOn(v, CROC_W);
        int y = quantizeCenterY(v, pos);
        sendToPlayerGroup(clientId, h -> h.sendSpawnCroc(variant, x, y));
        crocodiles.add(new Rect(x, y, 8, 8));
        crocodileStates.add(new EntityState(variant, x, y));

    }
    public void spawnFruitOnVineForClient(int clientId, int vineIndex, byte variant, int pos){
        Rect v = vines.get(vineIndex);
        int x = centerXOn(v, FRUIT_W);
        int y = quantizeCenterY(v, pos);
        sendToPlayerGroup(clientId, h -> h.sendSpawnFruit(variant, x, y));
        fruits.add(new Rect(x, y, 8, 8));
        fruitStates.add(new EntityState(variant, x, y));

    }
    public void spawnCrocOnPlatformForClient(int clientId, int platIndex, byte variant, int pos){
        Rect p = platforms.get(platIndex);
        int x = quantizeCenterX(p, pos);
        int y = p.y() - 8; 
        sendToPlayerGroup(clientId, h -> h.sendSpawnCroc(variant, x, y));
        crocodiles.add(new Rect(x, y, 8, 8));
        crocodileStates.add(new EntityState(variant, x, y));

    }
    public void spawnFruitOnPlatformForClient(int clientId, int platIndex, byte variant, int pos){
        Rect p = platforms.get(platIndex);
        int x = quantizeCenterX(p, pos);
        int y = p.y() - 8;
        sendToPlayerGroup(clientId, h -> h.sendSpawnFruit(variant, x, y));
        fruits.add(new Rect(x, y, 8, 8));
        fruitStates.add(new EntityState(variant, x, y));
    }
    public void removeFruitOnVineForClient(int clientId, int vineIndex, int pos){
        Rect v = vines.get(vineIndex);
        int x = v.x() + v.w()/2;
        int y = quantizeCenterY(v, pos);

        sendToPlayerGroup(clientId, h -> h.sendRemoveFruit(x, y));
        fruits.removeIf(r -> r.x() == x && r.y() == y);
        fruitStates.removeIf(f -> f.x == x && f.y == y);
    }
    public void removeFruitOnPlatformForClient(int clientId, int platIndex, int pos){
        Rect p = platforms.get(platIndex);
        int x = quantizeCenterX(p, pos);
        int y = p.y() - 8;

        sendToPlayerGroup(clientId, h -> h.sendRemoveFruit(x, y));

        fruits.removeIf(r -> r.x() == x && r.y() == y);
        fruitStates.removeIf(f -> f.x == x && f.y == y);
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

    static int centerXOn(Rect vine, int entityW) {
        return vine.x() + (vine.w() - entityW) / 2;
    }


    public static void main(String[] args) throws Exception {
        int port = (args.length > 0) ? Integer.parseInt(args[0]) : 9090;
        new GameServer(port).start();
    }

    
}