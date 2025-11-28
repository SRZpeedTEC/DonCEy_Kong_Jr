package serverJava;

import java.io.*;
import java.net.*;

import MessageManagement.Messenger;
import MessageManagement.Session;
import MessageManagement.AnswerProcessor;
import Classes.Player.player;

/**
 * Handles a single client connection: reads incoming frames, delegates parsing/side-effects,
 * and exposes convenience methods to push outbound messages to the client.
 */
public class ClientHandler extends Thread {

    private final int clientId;
    private final Socket socket;
    private final GameServer server;

    private final DataInputStream in;
    private final Session session;
    private final AnswerProcessor answerProcessor;
    private final Messenger messenger;
    private player player;

    private ClientRole role;
    private Integer observedPlayerId;

    /**
     * Creates a handler bound to a specific client connection.
     *
     * @param clientId          unique identifier assigned by the server
     * @param socket            connected socket to the remote client
     * @param server            owning {@link GameServer}
     * @param role              initial role for this client (PLAYER or SPECTATOR)
     * @param observedPlayerId  if SPECTATOR, the player id being observed; otherwise may be {@code null}
     * @throws IOException if I/O streams cannot be created or initial messages cannot be sent
     */
    public ClientHandler(int clientId, Socket socket, GameServer server, ClientRole role, Integer observedPlayerId) throws IOException {
        super("Client-" + clientId);
        this.clientId = clientId;
        this.socket   = socket;
        this.server   = server;
        this.player   = server.getPlayerFromServer(clientId);
        this.role     = role;
        this.observedPlayerId = observedPlayerId;

        this.in = new DataInputStream(new BufferedInputStream(socket.getInputStream()));
        DataOutputStream out = new DataOutputStream(new BufferedOutputStream(socket.getOutputStream()));

        this.session         = new Session(clientId, out);
        this.answerProcessor = new AnswerProcessor(server);
        this.messenger       = new Messenger(server);

        // Send initial static map (legacy INIT_STATIC)
        messenger.sendInitStaticLegacy(clientId, out);
    }

    /**
     * @return current role for this client.
     */
    public ClientRole getRole() {
        return role;
    }

    /**
     * @return player id currently observed by this client (for spectators), or {@code null}.
     */
    public Integer getObservedPlayerId() {
        return observedPlayerId;
    }

    /**
     * @return the server-side {@link player} model associated with this client (if any).
     */
    public player getPlayer() {
        return player;
    }

    /**
     * @return the unique client id assigned by the server.
     */
    public int getClientId() {
        return clientId;
    }

    /**
     * Updates the observed player id for this client (spectator context).
     *
     * @param pid player id to observe; may be {@code null} to clear.
     */
    public synchronized void setObservedPlayerId(Integer pid) {
        this.observedPlayerId = pid;
    }

    /**
     * Sends a SPECTATOR_STATE update (position, velocity, flags) to this client.
     *
     * @param x     world X position (i16)
     * @param y     world Y position (i16)
     * @param vx    horizontal velocity (i16)
     * @param vy    vertical velocity (i16)
     * @param flags bitfield describing player state
     */
    public void sendSpectatorState(short x, short y, short vx, short vy, byte flags) {
        try {
            messenger.sendSpectatorState(session, x, y, vx, vy, flags);
        } catch (IOException e) {
            session.log("Error sending SPECTATOR_STATE: " + e.getMessage());
        }
    }

    /**
     * Notifies the client that crocodile speed increased.
     */
    public void sendCrocSpeedIncrease() {
        try {
            messenger.sendCrocSpeedIncrease(session);
        } catch (IOException e) {
            session.log("Error CROC_SPEED_INCREASE: " + e.getMessage());
        }
    }

    /**
     * @return remote endpoint string (host:port).
     */
    public String getRemote() {
        return socket.getRemoteSocketAddress().toString();
    }

    /**
     * Sends a crocodile spawn command.
     *
     * @param variant crocodile variant id
     * @param x       world X
     * @param y       world Y
     */
    public void sendSpawnCroc(byte variant, int x, int y) {
        try {
            messenger.sendSpawnCroc(session, variant, x, y);
            session.log("Sent CROC_SPAWN(v="+variant+") to client " + clientId + " at (" + x + "," + y + ")");
        } catch (IOException e) {
            session.log("Error sending CROC_SPAWN: " + e.getMessage());
        }
    }

    /**
     * Sends a fruit spawn command.
     *
     * @param variant fruit variant id
     * @param x       world X
     * @param y       world Y
     */
    public void sendSpawnFruit(byte variant, int x, int y) {
        try {
            messenger.sendSpawnFruit(session, variant, x, y);
            session.log("Sent FRUIT_SPAWN(v="+variant+") to client " + clientId + " at (" + x + "," + y + ")");
        } catch (IOException e) {
            session.log("Error sending FRUIT_SPAWN: " + e.getMessage());
        }
    }

    /**
     * Sends a remove-fruit command for the fruit at (x,y).
     *
     * @param x fruit X
     * @param y fruit Y
     */
    public void sendRemoveFruit(int x, int y){
        try {
            messenger.sendRemoveFruit(session, x, y);
            session.log("Sent REMOVE_FRUIT to " + clientId + " at ("+x+","+y+")");
        } catch(IOException e){
            session.log("Error REMOVE_FRUIT: " + e.getMessage());
        }
    }

    /**
     * Informs the client to respawn after death.
     */
    public void sendRespawnDeath() {
        try { messenger.sendRespawnDeath(session); }
        catch (IOException e){ session.log("Error RESPawnDeath: "+e.getMessage()); }
    }

    /**
     * Informs the client to respawn after victory.
     */
    public void sendRespawnWin() {
        try { messenger.sendRespawnWin(session); }
        catch (IOException e){ session.log("Error RESPawnWin: "+e.getMessage()); }
    }

    /**
     * Informs the client that the game is over.
     */
    public void sendGameOver() {
        try { messenger.sendGameOver(session); }
        catch (IOException e){ session.log("Error GameOver: "+e.getMessage()); }
    }

    /**
     * Sends a HUD lives update.
     *
     * @param lives remaining lives
     */
    public void sendLivesUpdate(byte lives) {
        try {
            messenger.sendLivesUpdate(session, lives);
        } catch (IOException e) {
            session.log("Error LIVES_UPDATE: " + e.getMessage());
        }
    }

    /**
     * Sends a HUD score update.
     *
     * @param score new score value
     */
    public void sendScoreUpdate(int score) {
        try {
            messenger.sendScoreUpdate(session, score);
        } catch (IOException e) {
            session.log("Error SCORE_UPDATE: " + e.getMessage());
        }
    }

    /**
     * Requests a full game restart on the client.
     */
    public void sendGameRestart() {
        try {
            messenger.sendGameRestart(session);
        } catch (IOException e) {
            session.log("Error GAME_RESTART: " + e.getMessage());
        }
    }

    /**
     * Main receive loop: blocks reading frames from the client and delegates to {@link AnswerProcessor}.
     * Terminates on I/O error or protocol exception; guarantees cleanup and server deregistration.
     */
    @Override
    public void run() {
        try {
            while (true) {
                answerProcessor.processFrame(in, session);
            }
        } catch (Exception e) {
            session.log("disconnect: " + e.getMessage());
        } finally {
            try { socket.close(); } catch (IOException ignore) {}
            server.removeClient(clientId);
        }
    }
}
