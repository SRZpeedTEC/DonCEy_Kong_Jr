package MessageManagement;

import java.io.*;
import serverJava.GameServer;
import Utils.MsgType;
import Classes.Player.player;

public class AnswerProcessor {

    public interface Handler {
        void handle(byte[] payload, Session sess) throws IOException;
    }

    private final GameServer server;
    private final Messenger messenger;

    public AnswerProcessor(GameServer server){
        this.server = server;
        this.messenger = new Messenger(server);
    }

    
    private void handleSpectateRequest(byte[] payload, Session sess) throws IOException {
        if (payload == null || payload.length < 1) {
            return;
        }

        int desiredSlot = payload[0] & 0xFF;  // 1 o 2
        int spectatorId = sess.clientId();

        boolean ok = server.attachSpectatorToSlot(spectatorId, desiredSlot);
        if (!ok) {
            System.out.println("Spectator " + spectatorId
                    + " failed to attach to slot " + desiredSlot + " (no player or full).");
            System.out.println("Disconnecting spectator " + spectatorId);
            
            // Force disconnect by throwing an exception 
            throw new IOException("Failed to attach to player slot " + desiredSlot + " - slot may be full or player not found");
        }
    }

    public void processFrame(DataInputStream in, Session sess) throws IOException {
        byte version = in.readByte();
        byte type    = in.readByte();
        int  _res    = in.readUnsignedShort();
        int  fromId  = in.readInt();
        int  gameId  = in.readInt();
        int  len     = in.readInt();

        //Mensajes con payload estructurado conocido
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

        // --- STATE_BUNDLE ---
        if (type == MsgType.STATE_BUNDLE) {
            byte[] buf = (len > 0) ? in.readNBytes(len) : new byte[0];
            TLVParser tlv = new TLVParser(buf);
            while (tlv.remaining() > 0) {
                TLVParser.TLV t = tlv.next();
                if (t == null) break;
                if (t.type == MsgType.TLV_ENTITIES_CORR) {
                    
                }
            }
            return;
        }

        // --- NOTIFY_DEATH_COLLISION ---
        if (type == MsgType.NOTIFY_DEATH_COLLISION) {
            player p = server.getPlayerFromServer(sess.clientId());
            if (p == null) return;

            if (p.getLives() > 0) p.decreaseLives();

            server.clearEntitiesForNewRound();
            byte lives = (byte) p.getLives();

            // HUD for player + spectators
            server.broadcastLivesUpdateToGroup(sess.clientId(), lives);

            if (p.getLives() > 0) {
                // respawn for everyone watching this player
                server.broadcastRespawnDeathToGroup(sess.clientId());
            } else {
                // game over for everyone watching this player
                server.broadcastGameOverToGroup(sess.clientId());
            }
            return;
        }

        if (type == MsgType.NOTIFY_FRUIT_PICK) {
            // Expect payload: 2 bytes x, 2 bytes y (coordinates of picked fruit)
            if (len < 4) return;
            
            byte[] payload = in.readNBytes(len);
            
            
            int fruitX = ((payload[0] & 0xFF) << 8) | (payload[1] & 0xFF);
            int fruitY = ((payload[2] & 0xFF) << 8) | (payload[3] & 0xFF);
            
            player p = server.getPlayerFromServer(sess.clientId());
            if (p == null) return;

            p.increaseScore(400);
            
            // Broadcast score update to player + spectators
            server.broadcastScoreUpdateToGroup(sess.clientId(), p.getScore());
            
            // Remove the fruit for player + spectators
            server.sendToPlayerGroup(sess.clientId(), h -> h.sendRemoveFruit(fruitX, fruitY));
            
            // Remove from server's fruit list
            server.fruits.removeIf(r -> r.x() == fruitX && r.y() == fruitY);
            server.fruitStates.removeIf(f -> f.x == fruitX && f.y == fruitY);
            
            return;
        }

        // --- NOTIFY_VICTORY ---
        if (type == MsgType.NOTIFY_VICTORY) {
            player p = server.getPlayerFromServer(sess.clientId());
            if (p == null) return;

            p.increaseLives();
            System.out.println("Player " + sess.clientId() + " won! Lives now: " + p.getLives());
            server.clearEntitiesForNewRound();

            byte lives = (byte) p.getLives();

            // HUD and respawn for everyone in the group
            server.broadcastLivesUpdateToGroup(sess.clientId(), lives);
            server.broadcastCrocSpeedIncreaseToGroup(sess.clientId());
            server.broadcastRespawnWinToGroup(sess.clientId());
            return;
        }

        // --- SPECTATE_REQUEST  ---
        if (type == MsgType.SPECTATE_REQUEST) {
            byte[] payload = (len > 0) ? in.readNBytes(len) : new byte[0];
            handleSpectateRequest(payload, sess);
            return;
        }

        // --- REQUEST_RESTART (player requests full game restart) ---
        if (type == MsgType.REQUEST_RESTART) {
            player p = server.getPlayerFromServer(sess.clientId());
            if (p == null) return;

            System.out.println("Player " + sess.clientId() + " requested game restart");

            // Reset player state
            p.setLives(3);
            p.setScore(0);

            // Clear all entities
            server.clearEntitiesForNewRound();

            // Reset crocodile speed on server 
            server.resetCrocodileSpeed();

            // Broadcast restart to player + spectators
            server.broadcastGameRestartToGroup(sess.clientId());

            // Send updated HUD
            server.broadcastLivesUpdateToGroup(sess.clientId(), (byte) 3);
            server.broadcastScoreUpdateToGroup(sess.clientId(), 0);

            return;
        }

        
        if (len > 0) in.skipNBytes(len);
    }
}