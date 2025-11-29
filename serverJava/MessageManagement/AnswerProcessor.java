package MessageManagement;

import java.io.*;
import serverJava.GameServer;
import Utils.MsgType;
import Classes.Player.player;

/**
 * Parses and processes inbound frames from a client and triggers the
 * corresponding server-side actions and broadcasts. This class acts as the
 * entry point for handling player proposals, collisions, fruit events,
 * victory, spectating, and restart requests.
 */
public class AnswerProcessor {

    /**
     * Functional interface for handlers that operate on a raw payload.
     */
    public interface Handler {
        /**
         * Handles a message payload for a given session.
         * @param payload Raw payload bytes (may be empty).
         * @param sess    Source session.
         * @throws IOException If an I/O error occurs during handling.
         */
        void handle(byte[] payload, Session sess) throws IOException;
    }

    private final GameServer server;
    private final Messenger messenger;

    /**
     * Creates a new processor bound to a server instance.
     * @param server Authoritative server instance (non-null).
     */
    public AnswerProcessor(GameServer server){
        this.server = server;
        this.messenger = new Messenger(server);
    }

    /**
     * Handles a spectator attach request. Expects a one-byte payload indicating
     * the desired player slot (1 or 2). If attachment fails, an {@link IOException}
     * is thrown to force disconnection.
     *
     * @param payload Request payload (must contain at least one byte).
     * @param sess    Requesting session.
     * @throws IOException If slot is unavailable or an I/O error occurs.
     */
    private void handleSpectateRequest(byte[] payload, Session sess) throws IOException {
        if (payload == null || payload.length < 1) {
            return;
        }

        int desiredSlot = payload[0] & 0xFF;
        int spectatorId = sess.clientId();

        boolean ok = server.attachSpectatorToSlot(spectatorId, desiredSlot);
        if (!ok) {
            System.out.println("Spectator " + spectatorId
                    + " failed to attach to slot " + desiredSlot + " (no player or full).");
            System.out.println("Disconnecting spectator " + spectatorId);
            throw new IOException("Failed to attach to player slot " + desiredSlot + " - slot may be full or player not found");
        }
    }

    /**
     * Reads a single inbound frame (already on the frame boundary) and executes
     * the corresponding action. Unknown types are skipped by consuming the payload.
     *
     * <p>Frame header layout (in read order):</p>
     * <pre>
     * version: byte
     * type   : byte
     * _res   : u16
     * fromId : s32
     * gameId : s32
     * len    : s32  (payload length)
     * </pre>
     *
     * @param in   Stream to read the frame from.
     * @param sess Source session.
     * @throws IOException If an I/O error occurs while reading or handling.
     */
    public void processFrame(DataInputStream in, Session sess) throws IOException {
        byte version = in.readByte();
        byte type    = in.readByte();
        int  _res    = in.readUnsignedShort();
        int  fromId  = in.readInt();
        int  gameId  = in.readInt();
        int  len     = in.readInt();

        if (type == MsgType.PLAYER_PROPOSED) {
            int   tick  = in.readInt();
            short x     = in.readShort();
            short y     = in.readShort();
            short vx    = in.readShort();
            short vy    = in.readShort();
            byte  flags = in.readByte();

            // Read remaining bytes as entities TLV
            int entitiesLen = len - 13;
            byte[] entitiesTlv = null;
            if (entitiesLen > 0) {
                entitiesTlv = in.readNBytes(entitiesLen);
            }

            player p1 = server.getPlayerFromServer(sess.clientId());
            if (p1 != null) {
                p1.x  = x;
                p1.y  = y;
                p1.vx = vx;
                p1.vy = vy;
            }

            // Broadcast to spectators WITH entities TLV
            server.broadcastPlayerStateToSpectators(sess.clientId(), x, y, vx, vy, flags, entitiesTlv);
            return;
        }

        if (type == MsgType.STATE_BUNDLE) {
            byte[] buf = (len > 0) ? in.readNBytes(len) : new byte[0];
            TLVParser tlv = new TLVParser(buf);
            while (tlv.remaining() > 0) {
                TLVParser.TLV t = tlv.next();
                if (t == null) break;
                if (t.type == MsgType.TLV_ENTITIES_CORR) {
                    // Reserved: entities correction handling 
                }
            }
            return;
        }

        if (type == MsgType.NOTIFY_DEATH_COLLISION) {
            player p = server.getPlayerFromServer(sess.clientId());
            if (p == null) return;

            if (p.getLives() > 0) p.decreaseLives();

            server.clearEntitiesForNewRound();
            byte lives = (byte) p.getLives();

            server.broadcastLivesUpdateToGroup(sess.clientId(), lives);

            if (p.getLives() > 0) {
                server.broadcastRespawnDeathToGroup(sess.clientId());
            } else {
                server.broadcastGameOverToGroup(sess.clientId());
            }
            return;
        }

        if (type == MsgType.NOTIFY_FRUIT_PICK) {
            if (len < 4) return;

            byte[] payload = in.readNBytes(len);

            int fruitX = ((payload[0] & 0xFF) << 8) | (payload[1] & 0xFF);
            int fruitY = ((payload[2] & 0xFF) << 8) | (payload[3] & 0xFF);

            player p = server.getPlayerFromServer(sess.clientId());
            if (p == null) return;

            p.increaseScore(400);

            server.broadcastScoreUpdateToGroup(sess.clientId(), p.getScore());
            server.sendToPlayerGroup(sess.clientId(), h -> h.sendRemoveFruit(fruitX, fruitY));

            server.fruits.removeIf(r -> r.x() == fruitX && r.y() == fruitY);
            server.fruitStates.removeIf(f -> f.x == fruitX && f.y == fruitY);

            return;
        }

        if (type == MsgType.NOTIFY_VICTORY) {
            player p = server.getPlayerFromServer(sess.clientId());
            if (p == null) return;

            p.increaseLives();
            System.out.println("Player " + sess.clientId() + " won! Lives now: " + p.getLives());
            server.clearEntitiesForNewRound();

            byte lives = (byte) p.getLives();

            server.broadcastLivesUpdateToGroup(sess.clientId(), lives);
            server.broadcastCrocSpeedIncreaseToGroup(sess.clientId());
            server.broadcastRespawnWinToGroup(sess.clientId());
            return;
        }

        if (type == MsgType.SPECTATE_REQUEST) {
            byte[] payload = (len > 0) ? in.readNBytes(len) : new byte[0];
            handleSpectateRequest(payload, sess);
            return;
        }

        if (type == MsgType.REQUEST_RESTART) {
            player p = server.getPlayerFromServer(sess.clientId());
            if (p == null) return;

            System.out.println("Player " + sess.clientId() + " requested game restart");

            p.setLives(3);
            p.setScore(0);

            server.clearEntitiesForNewRound();
            server.resetCrocodileSpeed();

            server.broadcastGameRestartToGroup(sess.clientId());
            server.broadcastLivesUpdateToGroup(sess.clientId(), (byte) 3);
            server.broadcastScoreUpdateToGroup(sess.clientId(), 0);

            return;
        }

        if (len > 0) in.skipNBytes(len);
    }
}
