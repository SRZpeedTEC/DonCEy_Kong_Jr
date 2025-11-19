
package MessageManagement;
import java.io.*;
import java.util.HashMap;
import java.util.Map;
import serverJava.GameServer;
import Utils.MsgType;
import MessageManagement.TLVParser;

public class AnswerProcessor {
    public interface Handler {
        void handle(byte[] payload, Session ctx) throws IOException;
    }

    private final Map<Byte, Handler> handlers = new HashMap<>();
    private final GameServer server;
    private final Messenger messenger;

    public AnswerProcessor(GameServer server){
        this.server = server;
        this.messenger = new Messenger(server);

        // registra handlers por tipo de frame
        //handlers.put(MsgType.PLAYER_PROPOSED, this::onPlayerProposed);
        //handlers.put(MsgType.PING,            this::onPing);
        
    }

    
public void processFrame(DataInputStream in, Session sess) throws IOException {
    // Read CP header (16 bytes, big-endian)
    byte version = in.readByte();
    byte type    = in.readByte();
    int  _res    = in.readUnsignedShort();
    int  fromId  = in.readInt();
    int  gameId  = in.readInt();
    int  len     = in.readInt();

    if (type == MsgType.PLAYER_PROPOSED) {
        // Expected payload: tick(4), x(2), y(2), vx(2), vy(2), flags(1) = 13 bytes
        if (len < 13) {
            if (len > 0) in.skipNBytes(len);
            return;
        }

        int   tick  = in.readInt();
        short x     = in.readShort();
        short y     = in.readShort();
        short vx    = in.readShort();
        short vy    = in.readShort();
        byte  flags = in.readByte();

        int extra = len - 13;
        if (extra > 0) {
            in.skipNBytes(extra);
        }

        // Mirror player state into server model if present
        if (server.p1 != null) {
            server.p1.x  = x;
            server.p1.y  = y;
            server.p1.vx = vx;
            server.p1.vy = vy;
        }
        int playerClientId = sess.clientId();
        server.broadcastPlayerStateToSpectators(fromId, x, y, vx, vy, flags);

        return;
    }

    if (type == MsgType.STATE_BUNDLE) {
        if (len <= 0) return;

        byte[] buf = new byte[len];
        in.readFully(buf);

        TLVParser tlv = new TLVParser(buf);

        while (tlv.remaining() > 0) {
            TLVParser.TLV t = tlv.next();
            if (t == null) break;

            if (t.type == MsgType.TLV_ENTITIES_CORR) {
                System.out.println("[ENTITIES_CORR] len=" + t.length);

                // Print raw bytes in hex to verify content
                StringBuilder sb = new StringBuilder();
                for (int i = 0; i < t.length; i++) {
                    sb.append(String.format("%02X ", t.value[i]));
                }
                System.out.println(sb.toString());

                // Later this TLV will be parsed and broadcast to spectators
            }
        }
        return;
    }

    // For other message types, skip payload for now
    if (len > 0) {
        in.skipNBytes(len);
    }
}
}
