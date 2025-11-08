import java.io.*;
import java.net.*;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicInteger;

public class GameServer {
    private static final byte VERSION = 1;
    private static final byte TYPE_MATRIX_STATE = 1;
    private static final byte TYPE_CLIENT_ACK   = 2;

    private final int port;
    private final ServerSocket serverSocket;
    private final ConcurrentHashMap<Integer, ClientHandler> clients = new ConcurrentHashMap<>();
    private final AtomicInteger idGen = new AtomicInteger(1000); // start IDs at 1000

    public GameServer(int port) throws IOException {
        this.port = port;
        this.serverSocket = new ServerSocket(this.port);
        System.out.println("Server listening on port " + port);
    }

    public void start() throws IOException {
        // Admin thread to target-send matrices and list clients
        Thread admin = new Thread(this::adminLoop, "admin-loop");
        admin.setDaemon(true);
        admin.start();

        while (true) {
            Socket s = serverSocket.accept();
            s.setTcpNoDelay(true);
            int id = idGen.getAndIncrement();
            ClientHandler handler = new ClientHandler(id, s, this);
            clients.put(id, handler);
            handler.start();
            System.out.println("Client connected, assigned id=" + id + " from " + s.getRemoteSocketAddress());
        }
    }

    private void adminLoop() {
        try (BufferedReader br = new BufferedReader(new InputStreamReader(System.in))) {
            String line;
            while ((line = br.readLine()) != null) {
                line = line.trim();
                if (line.equalsIgnoreCase("list")) {
                    if (clients.isEmpty()) System.out.println("(no clients)");
                    clients.forEach((id, h) ->
                        System.out.println("id=" + id + " remote=" + h.getRemote()));
                } else if (line.startsWith("send ")) {
                    try {
                        int id = Integer.parseInt(line.substring(5).trim());
                        sendSampleMatrixTo(id);
                    } catch (Exception e) {
                        System.out.println("Usage: send <clientId>");
                    }
                } else if (line.equalsIgnoreCase("help")) {
                    System.out.println("Commands: list | send <clientId> | help");
                }
            }
        } catch (IOException ignored) {}
    }

    private void sendSampleMatrixTo(int clientId) {
        ClientHandler h = clients.get(clientId);
        if (h == null) {
            System.out.println("No such client: " + clientId);
            return;
        }

        // Build a small 10x10 matrix with some 'traps' (value=2), empty=0, player=1 (example)
        int rows = 10, cols = 10;
        byte[] matrix = new byte[rows * cols];
        Random r = new Random();
        for (int i = 0; i < rows * cols; i++) {
            int val = 0;
            if (r.nextDouble() < 0.10) val = 2; // ~10% traps
            matrix[i] = (byte) val;
        }
        // Let's pretend player's tile at (0,0)
        matrix[0] = 1;

        try {
            h.sendMatrixState(clientId, /*gameId=*/clientId, rows, cols, matrix);
            System.out.println("Sent MATRIX_STATE to client " + clientId);
        } catch (IOException e) {
            System.out.println("Failed to send to client " + clientId + ": " + e.getMessage());
        }
    }

    void removeClient(int id) {
        clients.remove(id);
        System.out.println("Client " + id + " disconnected.");
    }

    public static void main(String[] args) throws Exception {
        int port = (args.length > 0) ? Integer.parseInt(args[0]) : 9090;
        new GameServer(port).start();
    }

    // --- client handler ---
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

            // (Optional) Send a quick ACK with assigned clientId so the client knows its id.
            sendAck();
        }

        String getRemote() {
            return socket.getRemoteSocketAddress().toString();
        }

        private void sendAck() throws IOException {
            // header: version, type=ACK, reserved=0, clientId, gameId=0, payloadLen=0
            out.writeByte(VERSION);
            out.writeByte(TYPE_CLIENT_ACK);
            out.writeShort(0);
            out.writeInt(clientId);
            out.writeInt(0);
            out.writeInt(0);
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
                while (true) {
                    // Read header (16 bytes)
                    byte version = in.readByte();
                    byte type = in.readByte();
                    int reserved = in.readUnsignedShort();
                    int fromClientId = in.readInt();  // source client specifies itself on outbound
                    int gameId = in.readInt();
                    int payloadLen = in.readInt();

                    if (version != VERSION) {
                        System.out.println("Client " + clientId + ": bad version " + version);
                        break;
                    }

                    byte[] payload = null;
                    if (payloadLen > 0) {
                        payload = in.readNBytes(payloadLen);
                        if (payload.length != payloadLen) break;
                    }

                    // Minimal handling: just log. You can route or update game state here.
                    System.out.println("Recv from client " + clientId + ": type=" + type + " len=" + payloadLen + " gameId=" + gameId);

                    // (Optional) echo back an ACK per message
                    // sendAck();

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
