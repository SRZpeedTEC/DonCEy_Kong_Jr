import java.io.*;
import java.net.*;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicInteger;

public class GameServer {
    // ---- Protocol ----
    private static final byte VERSION = 1;
    private static final byte TYPE_CLIENT_ACK   = 2; // server -> client
    private static final byte TYPE_PLAYER_INPUT = 3; // client -> server
    private static final byte TYPE_INIT_STATIC = 4;


    // ---- Server fields ----
    private final int port;
    private final ServerSocket serverSocket;

    private final ConcurrentHashMap<Integer, ClientHandler> clients = new ConcurrentHashMap<>();
    private final AtomicInteger idGen = new AtomicInteger(1000);

    // ---- elements in game ----
    public static final class Rect {
        final int x,y,w,h;
        Rect(int x,int y,int w,int h){ this.x=x; this.y=y; this.w=w; this.h=h; }
    }

    public Rect player = new Rect(16,192,16,16);
    public final List<Rect> enemies   = new ArrayList<>();
    public final List<Rect> fruits    = new ArrayList<>();
    public final List<Rect> platforms = new ArrayList<>();
    public final List<Rect> vines     = new ArrayList<>();
    public final List<Rect> waters    = new ArrayList<>();

    private void writeU16(DataOutputStream out, int v) throws IOException {
        out.writeShort(v & 0xFFFF);
    }
    private void writeRect(DataOutputStream out, Rect r) throws IOException {
        writeU16(out, r.x); writeU16(out, r.y); writeU16(out, r.w); writeU16(out, r.h);
    }

    private void initLevel(){
        
         // Plataformas (VERDE)
        platforms.add(new Rect(0,   24, 256, 12));  // superior larga
        platforms.add(new Rect(32,  72,  56, 12));  // escalón izq
        platforms.add(new Rect(152, 64,  88, 12));  // plataforma derecha alta
        platforms.add(new Rect(184,136,  64, 12));  // derecha media
        platforms.add(new Rect(120,160,  96, 12));  // centro media-baja
        platforms.add(new Rect(72, 192, 120, 12));  // larga inferior
        platforms.add(new Rect(200,176,  56, 12));  // baja derecha
        // "islas" frente al agua (opcional)
        platforms.add(new Rect( 64,208,  32, 12));
        platforms.add(new Rect(112,208,  32, 12));
        platforms.add(new Rect(160,208,  32, 12));

        // Lianas (AMARILLO) – finas (w=6)
        vines.add(new Rect( 40, 40, 6, 168));
        vines.add(new Rect( 88, 56, 6, 152));
        vines.add(new Rect(128, 40, 6, 168));
        vines.add(new Rect(168, 48, 6, 160));
        vines.add(new Rect(200, 40, 6, 168));
        vines.add(new Rect(232, 40, 6, 168));

        // Mar (CELESTE)
        waters.add(new Rect(0, 224, 256, 16));
    }

    public GameServer(int port) throws IOException {
        this.port = port;
        this.serverSocket = new ServerSocket(this.port);
        initLevel(); // nivel listo desde el arranque
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

            sendAck();
            sendInitGeom(clientId, out); // usa datos ya inicializados en server
        }

        String getRemote() { return socket.getRemoteSocketAddress().toString(); }

        private void sendAck() throws IOException {
            out.writeByte(VERSION);
            out.writeByte(TYPE_CLIENT_ACK);
            out.writeShort(0);
            out.writeInt(clientId);
            out.writeInt(0);      // gameId
            out.writeInt(0);      // payloadLen
            out.flush();
        }

        // send GameGeometry (todo via 'server.')
        private void sendInitGeom(int destClientId, DataOutputStream out) throws IOException {
            int nP = server.platforms.size(), nV = server.vines.size(),
                nE = server.enemies.size(),   nF = server.fruits.size();
            int rectBytes = 8;
            int payloadLen = (4*2)
                           + 2 + nP*rectBytes
                           + 2 + nV*rectBytes
                           + 2 + nE*rectBytes
                           + 2 + nF*rectBytes;

            out.writeByte(VERSION);
            out.writeByte(TYPE_INIT_STATIC);
            out.writeShort(0);
            out.writeInt(destClientId);
            out.writeInt(0);
            out.writeInt(payloadLen);

            server.writeRect(out, server.player);

            server.writeU16(out, nP); for (Rect r: server.platforms) server.writeRect(out, r);
            server.writeU16(out, nV); for (Rect r: server.vines)     server.writeRect(out, r);
            server.writeU16(out, nE); for (Rect r: server.enemies)   server.writeRect(out, r);
            server.writeU16(out, nF); for (Rect r: server.fruits)    server.writeRect(out, r);

            out.flush();
        }

        @Override public void run(){
            try{
                while (true){
                    byte version = in.readByte();
                    byte type    = in.readByte();
                    int  _res    = in.readUnsignedShort();
                    int  fromId  = in.readInt();
                    int  gameId  = in.readInt();
                    int  len     = in.readInt();
                    byte[] payload = (len>0)? in.readNBytes(len): null;
                    if (version!=VERSION) break;

                    if (type==TYPE_PLAYER_INPUT){
                        System.out.println("INPUT from "+fromId+" len="+len);
                        // TODO: actualizar server.player / entidades y enviar diffs/STATE
                    } else {
                        System.out.println("msg type="+type+" len="+len);
                    }
                }
            } catch (EOFException ignore) {
            } catch (IOException e){
                System.out.println("Client "+clientId+" error: "+e.getMessage());
            } finally {
                try{ socket.close(); }catch(IOException ignore){}
                server.removeClient(clientId);
            }
        }
    }
}
