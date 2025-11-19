package MessageManagement;

import java.io.*;
import java.util.HashMap;
import java.util.Map;

import serverJava.GameServer;
import Utils.MsgType;
import MessageManagement.TLVParser;

public class AnswerProcessor {
    // Simple handler interface (kept for possible future use)
    public interface Handler {
        void handle(byte[] payload, Session ctx) throws IOException;
    }

    private final Map<Byte, Handler> handlers = new HashMap<>();
    private final GameServer server;
    private final Messenger messenger;

    public AnswerProcessor(GameServer server){
        this.server = server;
        this.messenger = new Messenger(server);

        // you can register extra handlers here if you want
        // handlers.put(MsgType.PLAYER_PROPOSED, this::onPlayerProposed);
        // handlers.put(MsgType.PING,            this::onPing);
    }

    public void processFrame(DataInputStream in, Session sess) throws IOException {
        // read common frame header (16 bytes, big-endian)
        byte version = in.readByte();
        byte type    = in.readByte();
        int  _res    = in.readUnsignedShort();
        int  fromId  = in.readInt();
        int  gameId  = in.readInt();
        int  len     = in.readInt();

        // handle player proposed frame from C (C -> Java)
        if (type == MsgType.PLAYER_PROPOSED) {
            // payload layout: tick u32, x i16, y i16, vx i16, vy i16, flags u8
            int   tick  = in.readInt();
            short x     = in.readShort();
            short y     = in.readShort();
            short vx    = in.readShort();
            short vy    = in.readShort();
            byte  flags = in.readByte();

            // mirror state into server.p1 if that object exists
            if (server.p1 != null){
                server.p1.x  = x;
                server.p1.y  = y;
                server.p1.vx = vx;
                server.p1.vy = vy;
            }

            // notify spectators about the updated player state + flags
            int playerClientId = sess.clientId();
            server.broadcastPlayerStateToSpectators(playerClientId, x, y, vx, vy, flags);

            return;
        }

        // handle state bundle sent from server to clients (used for TLVs)
        if (type == MsgType.STATE_BUNDLE) {
            if (len <= 0) {
                return;
            }

            // read the full TLV buffer
            byte[] buf = new byte[len];
            in.readFully(buf);

            // parse TLVs inside the bundle
            TLVParser tlv = new TLVParser(buf);
            while (tlv.remaining() > 0) {
                TLVParser.TLV t = tlv.next();
                if (t == null) break;

                // debug-print the ENTITIES_CORR TLV that comes from C
                if (t.type == MsgType.TLV_ENTITIES_CORR) {
                    System.out.println("[ENTITIES_CORR] len=" + t.length);

                    StringBuilder sb = new StringBuilder();
                    for (int i = 0; i < t.length; i++) {
                        sb.append(String.format("%02X ", t.value[i]));
                    }
                    System.out.println(sb.toString());

                    // later you can decode t.value into entity objects
                    // and broadcast them to spectators
                }
            }
            return;
        }

        // for other message types, skip payload for now
        if (len > 0) {
            in.skipNBytes(len);
        }
    }
}