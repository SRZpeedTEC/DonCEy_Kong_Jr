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
            nE = server.enemies.size(),   nF = server.fruits.size();

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
        Proto.writeU16(out, nE); for (Rect r: server.enemies)   Proto.writeRect(out, r);
        Proto.writeU16(out, nF); for (Rect r: server.fruits)    Proto.writeRect(out, r);

        out.flush();
    }

    /* 
    // B) STATE_BUNDLE (TLV dentro) – autoritativo
    public void sendStateCorrectedTLV(int destClientId, DataOutputStream out, Player p, int tick) throws IOException {
        // TLV 1: STATE_HEADER (tick u32)
        ByteArrayOutputStream buf = new ByteArrayOutputStream();
        DataOutputStream pout = new DataOutputStream(buf);
        Proto.writeTLV(pout, TlvType.STATE_HEADER, Proto.bbU32(tick));

        // TLV 2: PLAYER_CORR
        ByteArrayOutputStream v2 = new ByteArrayOutputStream();
        DataOutputStream dv2 = new DataOutputStream(v2);
        // grounded (u8), platId (u16), yCorr (i16), vyCorr (i16)
        dv2.writeByte(p.grounded ? 1 : 0);
        Proto.writeU16(dv2, p.onPlatId < 0 ? 0xFFFF : p.onPlatId);
        dv2.write(Proto.bbI16((short)p.y));
        dv2.write(Proto.bbI16((short)p.vy));
        dv2.flush();
        Proto.writeTLV(pout, TlvType.PLAYER_CORR, v2.toByteArray());

        pout.flush();
        byte[] payload = buf.toByteArray();

        Proto.writeHeader(out, MsgType.STATE_BUNDLE, destClientId, 0, payload.length);
        out.write(payload);
        out.flush();

    } */

}
