package serverJava;

import java.io.*;
import java.net.*;

import MessageManagement.Messenger;
import MessageManagement.Proto;
import MessageManagement.Session;
import MessageManagement.AnswerProcessor;
import Utils.MsgType;
import serverJava.ClientRole;
import Utils.Rect;
import Classes.Player.player;



public class ClientHandler extends Thread {

    private final int clientId;
    private final Socket socket;
    private final GameServer server;

    private final DataInputStream in;
    private final Session session;
    private final AnswerProcessor answerProcessor;
    private final Messenger messenger;

    private ClientRole role;
    private Integer observedPlayerId;

    public ClientHandler(int clientId, Socket socket, GameServer server, ClientRole role, Integer observedPlayerId) throws IOException {
        super("Client-" + clientId);
        this.clientId = clientId;
        this.socket   = socket;
        this.server   = server;   // store reference to GameServer
        this.role     = role;
        this.observedPlayerId = observedPlayerId;

        this.in = new DataInputStream(new BufferedInputStream(socket.getInputStream()));
        DataOutputStream out = new DataOutputStream(new BufferedOutputStream(socket.getOutputStream()));

        this.session         = new Session(clientId, out);
        this.answerProcessor = new AnswerProcessor(server);
        this.messenger       = new Messenger(server);

        // --- ACK al conectar ---
        Proto.writeHeader(out, MsgType.CLIENT_ACK, clientId, 0, 0);
        out.flush();

        // --- Mapa estático inicial ---
        messenger.sendInitStaticLegacy(clientId, out);
    }
    
    public ClientRole getRole() {
        return role;
    }

    public Integer getObservedPlayerId() {
        return observedPlayerId;
    }

    public void sendSpectatorState(short x, short y, short vx, short vy, byte flags) {
        try {
            messenger.sendSpectatorState(session, x, y, vx, vy, flags);
            session.log("Sent SPECTATOR_STATE to client " + clientId +
                        " (x=" + x + ", y=" + y + ", vx=" + vx + ", vy=" + vy +
                        ", flags=" + flags + ")");
        } catch (IOException e) {
            session.log("Error sending SPECTATOR_STATE: " + e.getMessage());
        }
    }

    public String getRemote() {
        return socket.getRemoteSocketAddress().toString();
    }

    public void sendSpawnCroc(byte variant, int x, int y) {
        try {
            messenger.sendSpawnCroc(session, variant, x, y);
            session.log("Sent CROC_SPAWN(v="+variant+") to client " + clientId + " at (" + x + "," + y + ")");
        } catch (IOException e) {
            session.log("Error sending CROC_SPAWN: " + e.getMessage());
        }
    }

    public void sendSpawnFruit(byte variant, int x, int y) {
        try {
            messenger.sendSpawnFruit(session, variant, x, y);
            session.log("Sent FRUIT_SPAWN(v="+variant+") to client " + clientId + " at (" + x + "," + y + ")");
        } catch (IOException e) {
            session.log("Error sending FRUIT_SPAWN: " + e.getMessage());
        }
    }

    public void sendRemoveFruit(int x, int y){
        try {
            messenger.sendRemoveFruit(session, x, y);
            session.log("Sent REMOVE_FRUIT to " + clientId + " at ("+x+","+y+")");
        } catch(IOException e){
            session.log("Error REMOVE_FRUIT: " + e.getMessage());
        }
    }


    @Override
    public void run() {
        try {
            while (true) {
                // Your existing message loop
                answerProcessor.processFrame(in, session);
                // For example, AnswerProcessor could decode "spawn crocodile at (x,y)"
                // and ask GameServer (via server) to broadcast that to players.
            }
        } catch (Exception e) {
            session.log("disconnect: " + e.getMessage());
        } finally {
            try { socket.close(); } catch (IOException ignore) {}

            // <<< this is the important line: call through the server reference
            server.removeClient(clientId);
        }
    }
}