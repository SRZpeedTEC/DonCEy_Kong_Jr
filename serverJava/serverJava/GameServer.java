package serverJava;

import java.io.*;
import java.net.*;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicInteger;

import MessageManagement.Messenger;
import MessageManagement.Proto;
import MessageManagement.Session;
import MessageManagement.AnswerProcessor;
import Utils.Rect;
import Utils.MsgType; 
// import Utils.TlvType;

public class GameServer {

    

    // ---- Server fields ----
    private final int port;
    private final ServerSocket serverSocket;

    private final ConcurrentHashMap<Integer, ClientHandler> clients = new ConcurrentHashMap<>();
    private final AtomicInteger idGen = new AtomicInteger(1000);

    // ---- elementos del juego (estáticos por ahora) ----
    public Rect player = new Rect(16,192,16,16);
    public final List<Rect> enemies   = new ArrayList<>();
    public final List<Rect> fruits    = new ArrayList<>();
    public final List<Rect> platforms = new ArrayList<>();
    public final List<Rect> vines     = new ArrayList<>();
    public final List<Rect> waters    = new ArrayList<>();

    private void initLevel(){
        platforms.add(new Rect(0,   24, 256, 12));
        platforms.add(new Rect(32,  72,  56, 12));
        platforms.add(new Rect(152, 64,  88, 12));
        platforms.add(new Rect(184,136,  64, 12));
        platforms.add(new Rect(120,160,  96, 12));
        platforms.add(new Rect(72, 192, 120, 12));
        platforms.add(new Rect(200,176,  56, 12));
        platforms.add(new Rect( 64,208,  32, 12));
        platforms.add(new Rect(112,208,  32, 12));
        platforms.add(new Rect(160,208,  32, 12));
        vines.add(new Rect( 40, 40, 6, 168));
        vines.add(new Rect( 88, 56, 6, 152));
        vines.add(new Rect(128, 40, 6, 168));
        vines.add(new Rect(168, 48, 6, 160));
        vines.add(new Rect(200, 40, 6, 168));
        vines.add(new Rect(232, 40, 6, 168));
        waters.add(new Rect(0, 224, 256, 16));
    }

    public GameServer(int port) throws IOException {
        this.port = port;
        this.serverSocket = new ServerSocket(this.port);
        initLevel();
        System.out.println("Server listening on port " + port);
    }

    public void start() throws IOException {
        Thread admin = new Thread(this::adminLoop, "admin-loop");
        admin.setDaemon(true);
        admin.start();

        while (true) {
            Socket s = serverSocket.accept();
            s.setTcpNoDelay(true);
            int id = idGen.getAndIncrement();

            ClientHandler handle = new ClientHandler(id, s, this);
            clients.put(id, handle);
            handle.start();
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
                    clients.forEach((id, h) -> System.out.println("id=" + id + " remote=" + h.getRemote()));
                } else if (line.equalsIgnoreCase("help")) {
                    System.out.println("Commands: list | help");
                }
            }
        } catch (IOException ignored) {}
    }

    void removeClient(int id) {
        clients.remove(id);
        System.out.println("Client " + id + " disconnected.");
    }

    public static void main(String[] args) throws Exception {
        int port = (args.length > 0) ? Integer.parseInt(args[0]) : 9090;
        new GameServer(port).start();
    }

    // ===== Client handler =====
    public class ClientHandler extends Thread {
        private final int clientId;
        private final Socket socket;              // <- GUARDA el socket
        private final AnswerProcessor answerProcessor;
        private final Messenger messenger;
        private final Session session;
        private final DataInputStream in;

        public ClientHandler(int clientId, Socket socket, GameServer server) throws IOException {
            super("Client-" + clientId);
            this.clientId = clientId;
            this.socket   = socket;               // <- ASIGNA
            this.in   = new DataInputStream(new BufferedInputStream(socket.getInputStream()));
            DataOutputStream out = new DataOutputStream(new BufferedOutputStream(socket.getOutputStream()));
            this.session = new Session(clientId, out);
            this.answerProcessor   = new AnswerProcessor(server);
            this.messenger = new Messenger(server);

            // ACK
            Proto.writeHeader(out, MsgType.CLIENT_ACK, clientId, 0, 0);
            out.flush();

            
            messenger.sendInitStaticLegacy(clientId, out);
        }

        public String getRemote() {               // <- MÉTODO QUE TE FALTABA
            return socket.getRemoteSocketAddress().toString();
        }

        @Override public void run(){
            try {
                while (true) {
                    answerProcessor.processFrame(in, session);
                }
            } catch (Exception e){
                session.log("disconnect: " + e.getMessage());
            } finally {
                try { socket.close(); } catch (IOException ignore) {}
                removeClient(clientId);
            }
        }
    }
}
