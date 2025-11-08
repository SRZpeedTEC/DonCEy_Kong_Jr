import java.io.*;
import java.net.*;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicInteger;

public class GameServer {
    // ---- Protocol ----
    private static final byte VERSION = 1;
    private static final byte TYPE_RECT_STATE    = 1; // server -> client
    private static final byte TYPE_CLIENT_ACK    = 2; // server -> client (assigned id)
    private static final byte TYPE_PLAYER_INPUT  = 3; // client -> server

    // ---- Server fields ----
    private final int port;
    private final ServerSocket serverSocket;

    // connected clients
    private final ConcurrentHashMap<Integer, ClientHandler> clients = new ConcurrentHashMap<>();
    private final AtomicInteger idGen = new AtomicInteger(1000);

    // per client game state (independent game state per instance)
    private final ConcurrentHashMap<Integer, GameState> gameStates = new ConcurrentHashMap<>();

    private static final int WORLD_WIDTH = 320;
    private static final int WORLD_HEIGHT = 240;

    private static final byte ELEMENT_PLAYER   = 1;
    private static final byte ELEMENT_ENEMY    = 2;
    private static final byte ELEMENT_FRUIT    = 3;
    private static final byte ELEMENT_PLATFORM = 4;

    public GameServer(int port) throws IOException {
        this.port = port;
        this.serverSocket = new ServerSocket(this.port);
        System.out.println("Server listening on port " + port);
    }

    private GameState initGameState() {
        GameState state = new GameState();

        // player
        state.addRect(ELEMENT_PLAYER, new Rect(24, 24, 16, 16));

        Random r = new Random();
        // create some enemies
        for (int i = 0; i < 3; i++) {
            int x = 40 + r.nextInt(WORLD_WIDTH - 80);
            int y = 40 + r.nextInt(WORLD_HEIGHT - 80);
            state.addRect(ELEMENT_ENEMY, new Rect(x, y, 16, 16));
        }

        // create some fruits
        for (int i = 0; i < 5; i++) {
            int x = 30 + r.nextInt(WORLD_WIDTH - 60);
            int y = 30 + r.nextInt(WORLD_HEIGHT - 60);
            state.addRect(ELEMENT_FRUIT, new Rect(x, y, 12, 12));
        }

        // create some platforms
        state.addRect(ELEMENT_PLATFORM, new Rect(0, 200, WORLD_WIDTH, 12));
        state.addRect(ELEMENT_PLATFORM, new Rect(50, 150, 120, 12));
        state.addRect(ELEMENT_PLATFORM, new Rect(200, 110, 90, 12));

        return state;
    }

    public void start() throws IOException {
        // simple admin CLI (list, send <id>)
        Thread admin = new Thread(this::adminLoop, "admin-loop");
        admin.setDaemon(true);
        admin.start();

        // accept loop
        while (true) {
            Socket s = serverSocket.accept();
            s.setTcpNoDelay(true);
            int id = idGen.getAndIncrement();

            // create per-client game state
            gameStates.put(id, initGameState());

            ClientHandler h = new ClientHandler(id, s, this);
            clients.put(id, h);
            h.start();
            System.out.println("Client connected, assigned id=" + id + " from " + s.getRemoteSocketAddress());
        }
    }

    private void adminLoop() {
        try (BufferedReader br = new BufferedReader(new InputStreamReader(System.in))) {
            String line;
            while ((line = br.readLine()) != null) {
                line = line.trim();
                if (line.equalsIgnoreCase("list")) {
                    if (clients.isEmpty()) { System.out.println("(no clients)"); continue; }
                    clients.forEach((id, h) -> {
                        System.out.println("id=" + id + " remote=" + h.getRemote());
                    });
                } else if (line.startsWith("send ")) {
                    try {
                        int id = Integer.parseInt(line.substring(5).trim());
                        GameState state = gameStates.get(id);
                        if (state == null) { System.out.println("No state for " + id); continue; }
                        ClientHandler h = clients.get(id);
                        if (h == null) { System.out.println("No such client " + id); continue; }
                        h.sendRectState(id, /*gameId*/ id, state);
                        System.out.println("Sent RECT_STATE to client " + id);
                    } catch (Exception e) {
                        System.out.println("Usage: send <clientId>");
                    }
                } else if (line.equalsIgnoreCase("help")) {
                    System.out.println("Commands: list | send <clientId> | help");
                }
            }
        } catch (IOException ignored) {}
    }

    void removeClient(int id) {
        clients.remove(id);
        gameStates.remove(id);
        System.out.println("Client " + id + " disconnected.");
    }

    // --- MAIN ---
    public static void main(String[] args) throws Exception {
        int port = (args.length > 0) ? Integer.parseInt(args[0]) : 9090;
        new GameServer(port).start();
    }

    // ===== Client handler with READ LOOP =====
    static class ClientHandler extends Thread {
        private final int clientId;
        private final Socket socket;
        private final GameServer server;
        private final DataInputStream in;
        private final DataOutputStream out;

        ClientHandler(int clientId, Socket socket, GameServer server) throws IOException {
            super("Client-" + clientId);
            this.clientId = clientId;
            this.socket = socket;
            this.server = server;
            this.in = new DataInputStream(new BufferedInputStream(socket.getInputStream()));
            this.out = new DataOutputStream(new BufferedOutputStream(socket.getOutputStream()));

            // On connect, send ACK with assigned clientId (payloadLen=0)
            sendAck();
            // Also send initial rectangles so the client renders something immediately (optional)
            GameState state = server.gameStates.get(clientId);
            if (state != null) sendRectState(clientId, clientId, state);
        }

        String getRemote() { return socket.getRemoteSocketAddress().toString(); }

        private void sendAck() throws IOException {
            out.writeByte(VERSION);
            out.writeByte(TYPE_CLIENT_ACK);
            out.writeShort(0);          // reserved
            out.writeInt(clientId);     // dest client id (this client)
            out.writeInt(0);            // gameId (0 for now)
            out.writeInt(0);            // payloadLen
            out.flush();
        }

        void sendRectState(int destClientId, int gameId, GameState state) throws IOException {
            byte[] payload = state.serialize();

            out.writeByte(VERSION);
            out.writeByte(TYPE_RECT_STATE);
            out.writeShort(0);                // reserved
            out.writeInt(destClientId);       // destination client id (this client)
            out.writeInt(gameId);
            out.writeInt(payload.length);

            out.write(payload);

            out.flush();
        }

        @Override
        public void run() {
            try {
                // ===== READ LOOP =====
                while (true) {
                    // --- 16-byte header (big-endian via DataInputStream) ---
                    byte version = in.readByte();
                    byte type    = in.readByte();
                    int reserved = in.readUnsignedShort();
                    int fromClientId = in.readInt(); // for client->server, this is the SENDER
                    int gameId  = in.readInt();
                    int payloadLen = in.readInt();

                    if (version != VERSION) {
                        System.out.println("Client " + clientId + ": bad version " + version);
                        break;
                    }

                    byte[] payload = null;
                    if (payloadLen > 0) {
                        payload = in.readNBytes(payloadLen);
                        if (payload.length != payloadLen) {
                            System.out.println("Client " + clientId + ": short payload");
                            break;
                        }
                    }

                    // --- Handle message types ---
                    if (type == TYPE_PLAYER_INPUT) {
                        // payload: action(1) + dx(2) + dy(2) big-endian = 5 bytes
                        if (payloadLen != 5) {
                            System.out.println("Client " + clientId + ": bad PLAYER_INPUT size " + payloadLen);
                            continue;
                        }
                        int action = payload[0] & 0xFF;
                        int dx = ((payload[1] & 0xFF) << 8) | (payload[2] & 0xFF);
                        if (dx > 32767) dx -= 65536;
                        int dy = ((payload[3] & 0xFF) << 8) | (payload[4] & 0xFF);
                        if (dy > 32767) dy -= 65536;

                        GameState state = server.gameStates.computeIfAbsent(clientId, k -> server.initGameState());

                        Rect player = state.getFirstRect(ELEMENT_PLAYER);
                        if (player == null) {
                            player = new Rect(24, 24, 16, 16);
                            state.addRect(ELEMENT_PLAYER, player);
                        }

                        int nx = player.x;
                        int ny = player.y;
                        int speed = 8;
                        switch (action) {
                            case 'L': nx -= speed; System.out.println("input receive L"); break;
                            case 'R': nx += speed; System.out.println("input receive R"); break;
                            case 'U': ny -= speed; System.out.println("input receive U"); break;
                            case 'D': ny += speed; System.out.println("input receive D"); break;
                            case 'J': ny -= speed * 2; System.out.println("input receive J"); break; // jump higher
                            default: /* ignore unknown */ break;
                        }

                        nx += dx;
                        ny += dy;

                        player.x = clamp(nx, 0, WORLD_WIDTH - player.width);
                        player.y = clamp(ny, 0, WORLD_HEIGHT - player.height);

                        // send updated rectangles back ONLY to this client
                        sendRectState(clientId, gameId == 0 ? clientId : gameId, state);
                        continue;
                    }

                    // (Optional) future types can be handled here.
                    System.out.println("Client " + clientId + " -> type=" + type + " payloadLen=" + payloadLen + " gameId=" + gameId);
                }
            } catch (EOFException eof) {
                // client closed
            } catch (IOException io) {
                System.out.println("Client " + clientId + " error: " + io.getMessage());
            } finally {
                try { socket.close(); } catch (IOException ignored) {}
                server.removeClient(clientId);
            }
        }
    }

    private static int clamp(int value, int min, int max) {
        if (value < min) return min;
        return Math.min(value, max);
    }

    private static class Rect {
        int x, y, width, height;

        Rect(int x, int y, int width, int height) {
            this.x = x;
            this.y = y;
            this.width = width;
            this.height = height;
        }
    }

    private static class GameState {
        private final Map<Byte, List<Rect>> groups = new LinkedHashMap<>();

        void addRect(byte elementType, Rect rect) {
            groups.computeIfAbsent(elementType, k -> new ArrayList<>()).add(rect);
        }

        Rect getFirstRect(byte elementType) {
            List<Rect> rects = groups.get(elementType);
            if (rects == null || rects.isEmpty()) return null;
            return rects.get(0);
        }

        byte[] serialize() throws IOException {
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            DataOutputStream dout = new DataOutputStream(baos);

            dout.writeShort(groups.size());
            for (Map.Entry<Byte, List<Rect>> entry : groups.entrySet()) {
                byte elementType = entry.getKey();
                List<Rect> rects = entry.getValue();
                dout.writeByte(elementType & 0xFF);
                dout.writeShort(rects.size());
                for (Rect rect : rects) {
                    dout.writeShort(rect.x);
                    dout.writeShort(rect.y);
                    dout.writeShort(rect.width);
                    dout.writeShort(rect.height);
                }
            }

            dout.flush();
            return baos.toByteArray();
        }
    }
}
