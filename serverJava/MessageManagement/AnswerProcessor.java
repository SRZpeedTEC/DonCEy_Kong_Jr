package MessageManagement;
import java.io.*;
import java.util.HashMap;
import java.util.Map;

import Classes.Player.player;
import serverJava.GameServer;
import Utils.MsgType;

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
        // --- header (16 bytes, big-endian) ---
        byte version = in.readByte();
        byte type    = in.readByte();
        int  _res    = in.readUnsignedShort();
        int  fromId  = in.readInt();
        int  gameId  = in.readInt();
        int  len     = in.readInt();

        if (type == MsgType.PLAYER_PROPOSED) {
            // payload: tick u32, x i16, y i16, vx i16, vy i16, flags u8
            int   tick  = in.readInt();
            short x     = in.readShort();
            short y     = in.readShort();
            short vx    = in.readShort();
            short vy    = in.readShort();
            byte  flags = in.readByte();

            
            if (server.p1 != null){
                server.p1.x = x; server.p1.y = y; server.p1.vx = vx; server.p1.vy = vy;
            }
            int playerClientId = sess.clientId();
            server.broadcastPlayerStateToSpectators(playerClientId, x, y, vx, vy, flags);
    
        } else {
            // descarta payload de otros tipos por ahora
            if (len > 0) in.skipNBytes(len);
        }
    }
}
