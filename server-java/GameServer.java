import java.io.*;
import java.net.*;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicInteger;

public class GameServer {
    // ---- Protocol ----
    private static final byte VERSION = 1;
    private static final byte TYPE_MATRIX_STATE  = 1; // server -> client
    private static final byte TYPE_CLIENT_ACK    = 2; // server -> client (assigned id)
    private static final byte TYPE_PLAYER_INPUT  = 3; // client -> server

    // ---- Server fields ----
    private final int port;
    private final ServerSocket serverSocket;

    // connected clients
    private final ConcurrentHashMap<Integer, ClientHandler> clients = new ConcurrentHashMap<>();
    private final AtomicInteger idGen = new AtomicInteger(1000);

    // one matrix per client (independent game state per instance)
    private final ConcurrentHashMap<Integer, byte[]> matrices = new ConcurrentHashMap<>();
    private final int rows = 10, cols = 10; // adjust to your game grid size

    public GameServer(int port) throws IOException {
        this.port = port;
        this.serverSocket = new ServerSocket(this.port);
        System.out.println("Server listening on port " + port);
    }

    private byte[] initMatrix() {
        byte[] m = new byte[rows * cols];
        // player starts at (0,0) = 1
        m[0] = 1;
        // sprinkle some traps as demo (value 2)
        Random r = new Random();
        for (int i = 1; i < m.length; i++) if (r.nextDouble() < 0.08) m[i] = 2;
        return m;
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

            // create per-client matrix
            matrices.put(id, initMatrix());

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
                        byte[] m = matrices.get(id);
                        if (m == null) { System.out.println("No matrix for " + id); continue; }
                        ClientHandler h = clients.get(id);
                        if (h == null) { System.out.println("No such client " + id); continue; }
                        h.sendMatrixState(id, /*gameId*/ id, rows, cols, m);
                        System.out.println("Sent MATRIX_STATE to client " + id);
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
        matrices.remove(id);
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
            // Also send initial matrix so the client renders something immediately (optional)
            byte[] m = server.matrices.get(clientId);
            if (m != null) sendMatrixState(clientId, clientId, server.rows, server.cols, m);
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

        void sendMatrixState(int destClientId, int gameId, int rows, int cols, byte[] matrix) throws IOException {
            if (matrix.length != rows * cols) throw new IllegalArgumentException("rows*cols mismatch");
            int payloadLen = 2 + 2 + matrix.length;

            out.writeByte(VERSION);
            out.writeByte(TYPE_MATRIX_STATE);
            out.writeShort(0);                // reserved
            out.writeInt(destClientId);       // destination client id (this client)
            out.writeInt(gameId);
            out.writeInt(payloadLen);

            out.writeShort(rows);
            out.writeShort(cols);
            out.write(matrix);

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

                        // Update ONLY this client's matrix
                        byte[] m = server.matrices.get(clientId);
                        if (m == null) {
                            m = server.initMatrix();
                            server.matrices.put(clientId, m);
                        }

                        // find player tile (1)
                        int px = 0, py = 0;
                        boolean found = false;
                        for (int r = 0; r < server.rows && !found; r++) {
                            for (int c = 0; c < server.cols; c++) {
                                if (m[r * server.cols + c] == 1) { py = r; px = c; found = true; break; }
                            }
                        }
                        int nx = px, ny = py;
                        switch (action) {
                            case 'L': nx = Math.max(0, px - 1); System.out.println("input receive L");break;
                            case 'R': nx = Math.min(server.cols - 1, px + 1);System.out.println("input receive R"); break;
                            case 'U': ny = Math.max(0, py - 1);System.out.println("input receive U"); break;
                            case 'D': ny = Math.min(server.rows - 1, py + 1); System.out.println("input receive D");break;
                            case 'J': ny = Math.max(0, py - 2); System.out.println("input receive J");break; // simple jump up
                            default: /* ignore unknown */ break;
                        }
                        // apply extra delta if provided
                        nx = Math.max(0, Math.min(server.cols - 1, nx + dx));
                        ny = Math.max(0, Math.min(server.rows - 1, ny + dy));

                        // write back
                        m[py * server.cols + px] = 0;
                        m[ny * server.cols + nx] = 1;

                        // send updated matrix back ONLY to this client
                        sendMatrixState(clientId, gameId == 0 ? clientId : gameId, server.rows, server.cols, m);
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
}
