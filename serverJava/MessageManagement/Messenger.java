package MessageManagement;
import serverJava.GameServer;
import java.io.*;
import Utils.Rect;
import Utils.MsgType;
import Messages.OutboundMessage;
import Messages.factories.CrocodileFactory;
import Messages.factories.FruitFactory;


public class Messenger {
    private final GameServer server;
    private final CrocodileFactory crocFactory = new CrocodileFactory();
    private final FruitFactory fruitFactory = new FruitFactory();

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
        

        Proto.writeHeader(out, MsgType.INIT_STATIC, destClientId, 0, payloadLen);

        Proto.writeRect(out, server.player);
        Proto.writeU16(out, nP); for (Rect r: server.platforms) Proto.writeRect(out, r);
        Proto.writeU16(out, nV); for (Rect r: server.vines)     Proto.writeRect(out, r);
        Proto.writeU16(out, nE); for (Rect r: server.crocodiles)   Proto.writeRect(out, r);
        Proto.writeU16(out, nF); for (Rect r: server.fruits)    Proto.writeRect(out, r);

        out.flush();
    }

    public void sendSpawnCroc(Session sess, byte variant, int x, int y) throws IOException {
        byte[] pl = packVariantXY(variant, x, y);
        Proto.writeHeader(sess.out(), MsgType.CROC_SPAWN, sess.clientId(), 0, pl.length);
        sess.out().write(pl);
        sess.out().flush();
    }


    public void sendSpawnFruit(Session sess, byte variant, int x, int y) throws IOException {
        byte[] pl = packVariantXY(variant, x, y);
        Proto.writeHeader(sess.out(), MsgType.FRUIT_SPAWN, sess.clientId(), 0, pl.length);
        sess.out().write(pl);
        sess.out().flush();
    }

    private static byte[] packVariantXY(byte variant, int x, int y){
    return new byte[]{
        variant,
        (byte)(x >> 8), (byte)x,
        (byte)(y >> 8), (byte)y
    };
}

}
