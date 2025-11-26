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

    // --- SPECTATE_REQUEST: adjuntar un espectador a un slot (1 o 2) ---
    private void handleSpectateRequest(byte[] payload, Session sess) {
        if (payload == null || payload.length < 1) {
            return;
        }

        int desiredSlot = payload[0] & 0xFF;  // 1 o 2
        int spectatorId = sess.clientId();

        boolean ok = server.attachSpectatorToSlot(spectatorId, desiredSlot);
        if (!ok) {
            System.out.println("Spectator " + spectatorId
                    + " failed to attach to slot " + desiredSlot + " (no player or full).");
            // Aquí podrías enviar un mensaje de error al cliente si quieres
        }
    }

    public void processFrame(DataInputStream in, Session sess) throws IOException {
        byte version = in.readByte();
        byte type    = in.readByte();
        int  _res    = in.readUnsignedShort();
        int  fromId  = in.readInt();
        int  gameId  = in.readInt();
        int  len     = in.readInt();

        // 1) Mensajes con payload estructurado conocido

        // --- PLAYER_PROPOSED (jugador envía su estado) ---
        if (type == MsgType.PLAYER_PROPOSED) {
            int   tick  = in.readInt();
            short x     = in.readShort();
            short y     = in.readShort();
            short vx    = in.readShort();
            short vy    = in.readShort();
            byte  flags = in.readByte();

            player p1 = server.getPlayerFromServer(sess.clientId());
            if (p1 != null) {
                p1.x  = x;
                p1.y  = y;
                p1.vx = vx;
                p1.vy = vy;
            }

            server.broadcastPlayerStateToSpectators(sess.clientId(), x, y, vx, vy, flags);
            return;
        }

        // --- STATE_BUNDLE (TLVs que luego podrás parsear) ---
        if (type == MsgType.STATE_BUNDLE) {
            byte[] buf = (len > 0) ? in.readNBytes(len) : new byte[0];
            TLVParser tlv = new TLVParser(buf);
            while (tlv.remaining() > 0) {
                TLVParser.TLV t = tlv.next();
                if (t == null) break;
                if (t.type == MsgType.TLV_ENTITIES_CORR) {
                    // futuro: parsear y mandar a espectadores
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
            player p = server.getPlayerFromServer(sess.clientId());
            if (p == null) return;

            p.increaseScore(400); // Aumenta la puntuación en 10 (o el valor que desees)

            messenger.sendScoreUpdate(sess, p.getScore()); // Envía la actualización de puntuación al cliente
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
            server.broadcastRespawnWinToGroup(sess.clientId());
            return;
        }

        // --- SPECTATE_REQUEST (nuevo) ---
        if (type == MsgType.SPECTATE_REQUEST) {
            byte[] payload = (len > 0) ? in.readNBytes(len) : new byte[0];
            handleSpectateRequest(payload, sess);
            return;
        }

        // 2) Otros tipos: de momento saltamos el payload
        if (len > 0) in.skipNBytes(len);
    }
}
