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

/**
 * Core TCP game server. Manages connections, roles (player/spectator),
 * static level data, entity state replication, and group broadcasts.
 * <p>
 * Responsibilities:
 * <ul>
 *   <li>Accept sockets and negotiate role capacity (players/spectators).</li>
 *   <li>Create {@link ClientHandler} threads per client.</li>
 *   <li>Maintain per-player spectator groups and HUD sync (lives/score).</li>
 *   <li>Expose helpers to spawn/remove entities on platforms/vines.</li>
 *   <li>Broadcast live player state to attached spectators.</li>
 * </ul>
 */
public class GameServer {

    /** TCP port to listen on. */
    private final int port;
    private ServerSocket serverSocket;

    /** Maximum concurrent player clients. */
    private static final int MAX_PLAYERS = 2;
    /** Maximum spectators per player. */
    private static final int MAX_SPECTATORS_PER_PLAYER = 2;
    /** Global spectator cap (derived from per-player cap). */
    private static final int MAX_TOTAL_SPECTATORS = MAX_PLAYERS * MAX_SPECTATORS_PER_PLAYER;

    /** All connected clients (players and spectators). */
    private final ConcurrentHashMap<Integer, ClientHandler> clients = new ConcurrentHashMap<>();
    /** Monotonic client id generator. */
    private final AtomicInteger idGen = new AtomicInteger(1);

    /** Active players mapped by client id. */
    private final Map<Integer, ClientHandler> players = new ConcurrentHashMap<>();

    /** Spectator lists keyed by player client id. */
    private final Map<Integer, List<ClientHandler>> spectatorsByPlayer = new ConcurrentHashMap<>();

    /** Server-side player state cache keyed by client id. */
    private final ConcurrentHashMap<Integer, player> playerStates = new ConcurrentHashMap<>();

    // ---- Level & game state ----

    /** Static platforms for the level. */
    public final List<Rect> platforms = new ArrayList<>();
    /** Static vines for the level. */
    public final List<Rect> vines      = new ArrayList<>();
    /** Water areas if present. */
    public final List<Rect> waters     = new ArrayList<>();

    /** Legacy crocodile rect list (for INIT_STATIC compatibility). */
    public final List<Rect> crocodiles = new ArrayList<>();
    /** Legacy fruit rect list (for INIT_STATIC compatibility). */
    public final List<Rect> fruits     = new ArrayList<>();

    /** Live crocodile logical states (variant + position). */
    public final List<EntityState> crocodileStates = new ArrayList<>();
    /** Live fruit logical states (variant + position). */
    public final List<EntityState> fruitStates     = new ArrayList<>();

    /** Default sprite width for crocodiles. */
    static final int CROC_W  = 8;
    /** Default sprite width for fruits. */
    static final int FRUIT_W = 8;

    /** Player slot 1 client id (or null if free). */
    private Integer playerSlot1 = null;
    /** Player slot 2 client id (or null if free). */
    private Integer playerSlot2 = null;

    /** Player rectangle used in legacy INIT_STATIC payloads. */
    public Rect player = new Rect(0, 0, 0, 0);

    /**
     * Creates a new server bound to the given port and preloads static level geometry.
     * @param port TCP port to listen on
     */
    public GameServer(int port) {
        this.port = port;
        initLevel();
    }

    /**
     * Populates the static level geometry (platforms and vines).
     * Adjust here to change the board layout.
     */
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

        // vines
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

    /** @return immutable snapshot of client ids. */
    public List<Integer> getClientIdsSnapshot() {
        return new ArrayList<>(clients.keySet());
    }

    /** @return static platforms list. */
    public List<Rect> getPlatforms() { return platforms; }
    /** @return static vines list. */
    public List<Rect> getVines()     { return vines; }
    /** @return static waters list. */
    public List<Rect> getWaters()    { return waters; }
    /** @return legacy crocodile rects. */
    public List<Rect> getcrocodiles(){ return crocodiles; }
    /** @return legacy fruit rects. */
    public List<Rect> getFruits()    { return fruits; }

    /**
     * Simple stdin admin loop supporting basic commands
     * (list, croc, fruit, help). Runs on a daemon thread.
     */
    private void adminLoop() {
        try (BufferedReader br = new BufferedReader(new InputStreamReader(System.in))) {
            String line;
            while ((line = br.readLine()) != null) {
                line = line.trim();
                if (line.equalsIgnoreCase("list")) {
                    if (clients.isEmpty()) { System.out.println("(no clients)"); continue; }
                    clients.forEach((id, h) -> System.out.println("id=" + id + " remote=" + h.getRemote()));
                } else if (line.startsWith("croc ")) {
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
                } else if (line.startsWith("fruit ")) {
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
                } else if (line.equalsIgnoreCase("help")) {
                    System.out.println("Commands:");
                    System.out.println("  list");
                    System.out.println("  croc  <clientId> <RED|BLUE|1|2> <x> <y>");
                    System.out.println("  fruit <clientId> <BANANA|APPLE|ORANGE|1|2|3> <x> <y>");
                    System.out.println("  help");
                }
            }
        } catch (IOException ignored) {}
    }

    /**
     * Parses crocodile variant token.
     * @param token textual token (e.g., "RED", "BLUE", "1", "2")
     * @return variant byte; defaults to 0 on error
     */
    private static byte parseCrocVariant(String token){
        token = token.toUpperCase();
        switch (token){
            case "RED":  return Utils.CrocVariant.RED.code;
            case "BLUE": return Utils.CrocVariant.BLUE.code;
            default:
                try { return (byte)Integer.parseInt(token); } catch(Exception e){ return 0; }
        }
    }

    /**
     * Parses fruit variant token.
     * @param token textual token (e.g., "BANANA", "APPLE", "ORANGE", "1", "2", "3")
     * @return variant byte; defaults to 0 on error
     */
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

    /**
     * Clears all dynamic entities (rects and states). Use when starting a new round.
     */
    public synchronized void clearEntitiesForNewRound() {
        crocodiles.clear();
        fruits.clear();
        crocodileStates.clear();
        fruitStates.clear();
    }

    /**
     * Removes crocodiles that have left the visible area (y &gt; 240).
     * Intended for periodic cleanup on a background thread.
     */
    public synchronized void cleanupOffScreenCrocodiles() {
        crocodileStates.removeIf(c -> c.y > 240);
        crocodiles.removeIf(r -> r.y() > 240);
    }

    /**
     * Heuristic: choose the player with the fewest spectators (&lt; 2).
     * @return player client id or {@code null} if none available
     */
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

    /**
     * Returns or creates a {@link player} state object for the given client id.
     * @param clientId client id
     * @return server-side player state
     */
    public player getPlayerFromServer(int clientId) {
        return playerStates.computeIfAbsent(clientId, id -> {
            player p = new player(0, 0);
            p.setLives(3);
            p.setScore(0);
            return p;
        });
    }

    /**
     * Attaches a spectator client to a specific player slot if possible.
     * Loads current entities and HUD to the spectator upon success.
     *
     * @param spectatorClientId spectator client id
     * @param slotIndex         1 or 2
     * @return {@code true} if attached; {@code false} otherwise
     */
    public synchronized boolean attachSpectatorToSlot(int spectatorClientId, int slotIndex) {
        ClientHandler spectator = clients.get(spectatorClientId);
        if (spectator == null) return false;
        if (spectator.getRole() != ClientRole.SPECTATOR) return false;

        Integer targetPlayerId = null;
        if (slotIndex == 1) {
            targetPlayerId = playerSlot1;
        } else if (slotIndex == 2) {
            targetPlayerId = playerSlot2;
        } else {
            return false;
        }

        if (targetPlayerId == null) return false;

        List<ClientHandler> specs = spectatorsByPlayer
                .computeIfAbsent(targetPlayerId,
                        k -> Collections.synchronizedList(new ArrayList<>()));

        if (specs.size() >= 2) return false;

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

    /** Broadcasts a respawn-after-death event to the player and their spectators. */
    public void broadcastRespawnDeathToGroup(int playerId){
        sendToPlayerGroup(playerId, ClientHandler::sendRespawnDeath);
    }

    /** Broadcasts a respawn-after-victory event to the player and their spectators. */
    public void broadcastRespawnWinToGroup(int playerId){
        sendToPlayerGroup(playerId, ClientHandler::sendRespawnWin);
    }

    /** Broadcasts a game-over event to the player and their spectators. */
    public void broadcastGameOverToGroup(int playerId){
        sendToPlayerGroup(playerId, ClientHandler::sendGameOver);
    }

    /**
     * Broadcasts a HUD lives update to the player and their spectators.
     * @param playerId player client id
     * @param lives    remaining lives
     */
    public void broadcastLivesUpdateToGroup(int playerId, byte lives) {
        sendToPlayerGroup(playerId, h -> h.sendLivesUpdate(lives));
    }

    /**
     * Broadcasts a HUD score update to the player and their spectators.
     * @param playerId player client id
     * @param score    new score value
     */
    public void broadcastScoreUpdateToGroup(int playerId, int score) {
        sendToPlayerGroup(playerId, h -> h.sendScoreUpdate(score));
    }

    /** Broadcasts a crocodile speed increase event to the player and their spectators. */
    public void broadcastCrocSpeedIncreaseToGroup(int playerId) {
        sendToPlayerGroup(playerId, ClientHandler::sendCrocSpeedIncrease);
    }

    /** Broadcasts a full game restart to the player and their spectators. */
    public void broadcastGameRestartToGroup(int playerId) {
        sendToPlayerGroup(playerId, ClientHandler::sendGameRestart);
    }

    /**
     * Resets server-side crocodile speed tracking (client performs actual speed logic).
     * Intended as a semantic sync point.
     */
    public void resetCrocodileSpeed() {
        System.out.println("Crocodile speed reset to default");
    }

    /**
     * Main accept loop. Negotiates role capacity, sends CLIENT_ACK,
     * constructs {@link ClientHandler}, and starts its thread.
     *
     * @throws IOException if the server socket fails
     */
    public void start() throws IOException {
        serverSocket = new ServerSocket(port);
        System.out.println("Server listening on port " + port);

        Thread admin = new Thread(this::adminLoop, "admin-loop");
        admin.setDaemon(true);
        admin.start();

        javax.swing.SwingUtilities.invokeLater(() -> {
            new ServerGui(this).show();
        });

        Thread cleanupThread = new Thread(() -> {
            while (true) {
                try {
                    Thread.sleep(1000);
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

            synchronized (this) {
                if (wantsPlayer) {
                    Integer freeSlot = null;
                    if (playerSlot1 == null) {
                        freeSlot = 1;
                    } else if (playerSlot2 == null) {
                        freeSlot = 2;
                    }

                    if (freeSlot != null) {
                        role = ClientRole.PLAYER;
                        observedPlayerId = null;

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
                    if (players.isEmpty()) {
                        System.out.println("Client " + clientId
                                + " requested SPECTATOR but no players connected. Rejecting.");
                    } else {
                        long totalSpectators = clients.values().stream()
                                .filter(h -> h.getRole() == ClientRole.SPECTATOR)
                                .count();

                        if (totalSpectators >= MAX_TOTAL_SPECTATORS) {
                            System.out.println("Client " + clientId
                                    + " requested SPECTATOR but max spectators reached ("
                                    + totalSpectators + "/" + MAX_TOTAL_SPECTATORS + "). Rejecting.");
                        } else {
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

            try {
                DataOutputStream out = new DataOutputStream(
                        new BufferedOutputStream(socket.getOutputStream()));

                byte roleByte = (role != null)
                        ? (role == ClientRole.PLAYER ? (byte)1 : (byte)2)
                        : (byte)0;

                int player1SpecCount = 0;
                int player2SpecCount = 0;
                boolean player1Active = false;
                boolean player2Active = false;

                synchronized (this) {
                    if (playerSlot1 != null && clients.containsKey(playerSlot1)) {
                        player1Active = true;
                        List<ClientHandler> specs1 = spectatorsByPlayer.get(playerSlot1);
                        player1SpecCount = (specs1 == null) ? 0 : specs1.size();
                    }
                    if (playerSlot2 != null && clients.containsKey(playerSlot2)) {
                        player2Active = true;
                        List<ClientHandler> specs2 = spectatorsByPlayer.get(playerSlot2);
                        player2SpecCount = (specs2 == null) ? 0 : specs2.size();
                    }
                }

                Proto.writeHeader(out, MsgType.CLIENT_ACK, clientId, 0, 3);
                out.writeByte(roleByte);
                out.writeByte(player1Active ? player1SpecCount : 255);
                out.writeByte(player2Active ? player2SpecCount : 255);
                out.flush();

                if (role == null) {
                    socket.close();
                    continue;
                }

            } catch (IOException e) {
                System.out.println("Failed to send CLIENT_ACK: " + e.getMessage());
                try { socket.close(); } catch (IOException ignore) {}

                synchronized (this) {
                    if (role == ClientRole.PLAYER) {
                        if (Objects.equals(playerSlot1, clientId)) playerSlot1 = null;
                        if (Objects.equals(playerSlot2, clientId)) playerSlot2 = null;
                    }
                }
                continue;
            }

            ClientHandler handler;
            try {
                handler = new ClientHandler(clientId, socket, this, role, observedPlayerId);
            } catch (IOException e) {
                System.out.println("Failed to create ClientHandler: " + e.getMessage());
                try { socket.close(); } catch (IOException ignore) {}

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

    /**
     * Deregisters a client and frees any occupied player slot.
     * Also detaches spectators observing a disconnected player.
     * @param clientId disconnected client id
     */
    public synchronized void removeClient(int clientId) {
        ClientHandler handler = clients.remove(clientId);
        if (handler == null) return;

        System.out.println("Client " + clientId + " disconnected");

        if (handler.getRole() == ClientRole.PLAYER) {
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

        } else {
            for (var entry : spectatorsByPlayer.entrySet()) {
                List<ClientHandler> list = entry.getValue();
                if (list != null) {
                    list.remove(handler);
                }
            }
        }
    }

    /**
     * @param slotIndex 1 or 2
     * @return client id in the given player slot or {@code null}
     */
    public Integer getPlayerClientIdForSlot(int slotIndex) {
        return switch (slotIndex) {
            case 1 -> playerSlot1;
            case 2 -> playerSlot2;
            default -> null;
        };
    }

    /**
     * Applies the given action to a player and all spectators watching them.
     * @param playerId player client id
     * @param action   side-effect to execute on each handler
     */
    public void sendToPlayerGroup(int playerId, Consumer<ClientHandler> action) {
        ClientHandler player = clients.get(playerId);
        if (player != null) {
            action.accept(player);
        }

        List<ClientHandler> specs = spectatorsByPlayer.get(playerId);
        if (specs != null) {
            for (ClientHandler s : specs) {
                action.accept(s);
            }
        }
    }

    /**
     * Sends the latest player state to all spectators attached to that player.
     *
     * @param playerClientId player client id
     * @param x              x position (i16)
     * @param y              y position (i16)
     * @param vx             x velocity (i16)
     * @param vy             y velocity (i16)
     * @param flags          state flags
     */
    public void broadcastPlayerStateToSpectators(int playerClientId, short x, short y, short vx, short vy, byte flags) {
        List<ClientHandler> specs = spectatorsByPlayer.get(playerClientId);
        if (specs == null || specs.isEmpty()) return;

        for (ClientHandler spectator : specs) {
            spectator.sendSpectatorState(x, y, vx, vy, flags);
        }
    }

    // ---- Entity spawn/remove helpers (vine/platform quantization) ----

    /**
     * Spawns a crocodile centered on the given vine at a quantized position.
     * Broadcasts to the player's group and records server state.
     */
    public void spawnCrocOnVineForClient(int clientId, int vineIndex, byte variant, int pos){
        Rect v = vines.get(vineIndex);
        int x = centerXOn(v, CROC_W);
        int y = quantizeCenterY(v, pos);
        sendToPlayerGroup(clientId, h -> h.sendSpawnCroc(variant, x, y));
        crocodiles.add(new Rect(x, y, 8, 8));
        crocodileStates.add(new EntityState(variant, x, y));
    }

    /**
     * Spawns a fruit centered on the given vine at a quantized position.
     * Broadcasts to the player's group and records server state.
     */
    public void spawnFruitOnVineForClient(int clientId, int vineIndex, byte variant, int pos){
        Rect v = vines.get(vineIndex);
        int x = centerXOn(v, FRUIT_W);
        int y = quantizeCenterY(v, pos);
        sendToPlayerGroup(clientId, h -> h.sendSpawnFruit(variant, x, y));
        fruits.add(new Rect(x, y, 8, 8));
        fruitStates.add(new EntityState(variant, x, y));
    }

    /**
     * Spawns a crocodile on top of the given platform at a quantized X position.
     */
    public void spawnCrocOnPlatformForClient(int clientId, int platIndex, byte variant, int pos){
        Rect p = platforms.get(platIndex);
        int x = quantizeCenterX(p, pos);
        int y = p.y() - 8;
        sendToPlayerGroup(clientId, h -> h.sendSpawnCroc(variant, x, y));
        crocodiles.add(new Rect(x, y, 8, 8));
        crocodileStates.add(new EntityState(variant, x, y));
    }

    /**
     * Spawns a fruit on top of the given platform at a quantized X position.
     */
    public void spawnFruitOnPlatformForClient(int clientId, int platIndex, byte variant, int pos){
        Rect p = platforms.get(platIndex);
        int x = quantizeCenterX(p, pos);
        int y = p.y() - 8;
        sendToPlayerGroup(clientId, h -> h.sendSpawnFruit(variant, x, y));
        fruits.add(new Rect(x, y, 8, 8));
        fruitStates.add(new EntityState(variant, x, y));
    }

    /**
     * Removes a fruit from a quantized vine position and broadcasts the removal.
     */
    public void removeFruitOnVineForClient(int clientId, int vineIndex, int pos){
        Rect v = vines.get(vineIndex);
        int x = v.x() + v.w()/2;
        int y = quantizeCenterY(v, pos);

        sendToPlayerGroup(clientId, h -> h.sendRemoveFruit(x, y));
        fruits.removeIf(r -> r.x() == x && r.y() == y);
        fruitStates.removeIf(f -> f.x == x && f.y == y);
    }

    /**
     * Removes a fruit from a quantized platform position and broadcasts the removal.
     */
    public void removeFruitOnPlatformForClient(int clientId, int platIndex, int pos){
        Rect p = platforms.get(platIndex);
        int x = quantizeCenterX(p, pos);
        int y = p.y() - 8;

        sendToPlayerGroup(clientId, h -> h.sendRemoveFruit(x, y));

        fruits.removeIf(r -> r.x() == x && r.y() == y);
        fruitStates.removeIf(f -> f.x == x && f.y == y);
    }

    /** Number of fixed segments used by quantizers (1..5). */
    private static final int N_FIXED = 5;

    /**
     * Maps a discrete position 1..5 to a centered Y inside the rect height.
     */
    private static int quantizeCenterY(Rect r, int pos){
        double s = r.h() / (double)N_FIXED;
        return r.y() + (int)Math.round((pos - 0.5) * s);
    }

    /**
     * Maps a discrete position 1..5 to a centered X inside the rect width.
     */
    private static int quantizeCenterX(Rect r, int pos){
        double s = r.w() / (double)N_FIXED;
        return r.x() + (int)Math.round((pos - 0.5) * s);
    }

    /**
     * Centers an entity of width {@code entityW} on a vertical vine rect.
     */
    static int centerXOn(Rect vine, int entityW) {
        return vine.x() + (vine.w() - entityW) / 2;
    }

    /**
     * Entry point. Starts the server on the given port or 9090 by default.
     * @param args optional port argument
     * @throws Exception if the server fails to start
     */
    public static void main(String[] args) throws Exception {
        int port = (args.length > 0) ? Integer.parseInt(args[0]) : 9090;
        new GameServer(port).start();
    }
}
