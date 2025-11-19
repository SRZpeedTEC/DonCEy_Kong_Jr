package MessageManagement;

import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.IOException;

import serverJava.GameServer;
import Utils.MsgType;

public class AnswerProcessor {

    private final GameServer server;

    public AnswerProcessor(GameServer server) {
        this.server = server;
    }

    public void processFrame(DataInputStream in, Session session) throws IOException {
        // --- leer header según Proto.writeHeader ---
        int version = in.readUnsignedByte();
        if (version != Proto.VERSION) {
            session.log("Bad protocol version: " + version);
            // descarta todo lo que quede disponible en el socket para evitar des-sync
            int avail = in.available();
            if (avail > 0) in.skipBytes(avail);
            return;
        }

        int type        = in.readUnsignedByte();   // MsgType.*
        int reserved    = in.readUnsignedShort();  // sin uso
        int destClient  = in.readInt();            // no lo usamos aquí
        int gameId      = in.readInt();            // idem
        int payloadLen  = in.readInt();

        byte[] payload = null;
        if (payloadLen > 0) {
            payload = in.readNBytes(payloadLen);
            if (payload.length != payloadLen) {
                session.log("Short read on payload: expected " + payloadLen +
                            " got " + payload.length);
                return;
            }
        }

        // --- despachar según tipo ---
        switch (type) {
            case MsgType.STATE_BUNDLE -> handleStateBundle(payload, session);
            case MsgType.PLAYER_PROPOSED -> handlePlayerProposed(payload, session);
            default -> {
                // tipos desconocidos: por ahora solo ignorar el payload
                if (payloadLen > 0) {
                    session.log("Ignoring unknown msg type: " + type +
                                " with " + payloadLen + " bytes");
                }
            }
        }
    }

    private void handleStateBundle(byte[] payload, Session session) throws IOException {
        if (payload == null || payload.length == 0) {
            return;
        }

    }

    
    private void handlePlayerProposed(byte[] payload, Session session) throws IOException {
        if (payload == null || payload.length < 9) {
            // x,y,vx,vy = 4 * int16 (8 bytes) + flags (1 byte) = 9 mínimo
            session.log("PLAYER_PROPOSED payload too short: " + (payload == null ? 0 : payload.length));
            return;
        }

        DataInputStream din = new DataInputStream(new ByteArrayInputStream(payload));

        short tick  = in.readShort();
        short x    = din.readShort();
        short y    = din.readShort();
        short vx   = din.readShort();
        short vy   = din.readShort();
        byte flags = din.readByte();


        int playerClientId = session.clientId();
        server.broadcastPlayerStateToSpectators(playerClientId, x, y, vx, vy, flags);
    }
}
