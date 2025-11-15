package MessageManagement;
import serverJava.GameServer;
import java.io.*;
import Utils.Rect;
import Utils.MsgType;

public class Messenger {
    private final GameServer server;

    public Messenger(GameServer server){ this.server = server; }

    // A) INIT_STATIC (LEGACY: igual al cliente C actual)
    public void sendInitStaticLegacy(int destClientId, DataOutputStream out) throws IOException {
        int nP = server.platforms.size(), nV = server.vines.size(),
            nE = server.crocodiles.size(),   nF = server.fruits.size();

        final int rectBytes = 8;
        int payloadLen = 8 // player
                       + 2 + nP*rectBytes
                       + 2 + nV*rectBytes
                       + 2 + nE*rectBytes
                       + 2 + nF*rectBytes;
        // Nota: si en C esperas también "water", añádelo aquí.

        Proto.writeHeader(out, MsgType.INIT_STATIC, destClientId, 0, payloadLen);

        Proto.writeRect(out, server.player);
        Proto.writeU16(out, nP); for (Rect r: server.platforms) Proto.writeRect(out, r);
        Proto.writeU16(out, nV); for (Rect r: server.vines)     Proto.writeRect(out, r);
        Proto.writeU16(out, nE); for (Rect r: server.crocodiles)   Proto.writeRect(out, r);
        Proto.writeU16(out, nF); for (Rect r: server.fruits)    Proto.writeRect(out, r);

        out.flush();
    }

    public void sendSpawnCroc(Session session, int x, int y) throws IOException {
        DataOutputStream out = session.out();  // or session.out, depending on your Session class

        // payload: int16 x, int16 y (big-endian, matches C side CP_TYPE_SPAWN_CROC)
        byte[] payload = new byte[4];
        payload[0] = (byte) (x >> 8);
        payload[1] = (byte) (x & 0xFF);
        payload[2] = (byte) (y >> 8);
        payload[3] = (byte) (y & 0xFF);

        // header: same helper you use for INIT_GEOM / STATE_BUNDLE
        Proto.writeHeader(
            out,
            MsgType.CROC_SPAWN,           // type
            session.clientId(),        // destination client id
            0,                            // gameId (0 for now)
            payload.length                // payload size
        );

        out.write(payload);
        out.flush();
    }

    public void sendSpawnFruit(Session session, int x, int y) throws IOException {
        DataOutputStream out = session.out();  // or session.out, depending on your Session class

        // payload: int16 x, int16 y (big-endian, matches C side CP_TYPE_SPAWN_FRUIT)
        byte[] payload = new byte[4];
        payload[0] = (byte) (x >> 8);
        payload[1] = (byte) (x & 0xFF);
        payload[2] = (byte) (y >> 8);
        payload[3] = (byte) (y & 0xFF);

        // header: same helper you use for INIT_GEOM / STATE_BUNDLE
        Proto.writeHeader(
            out,
            MsgType.FRUIT_SPAWN,           // type
            session.clientId(),        // destination client id
            0,                            // gameId (0 for now)
            payload.length                // payload size
        );

        out.write(payload);
        out.flush();
    }

}
