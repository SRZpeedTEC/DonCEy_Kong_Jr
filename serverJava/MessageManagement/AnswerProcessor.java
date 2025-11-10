package MessageManagement;
import java.io.*;
import java.util.HashMap;
import java.util.Map;
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

    public void processFrame(DataInputStream in, Session ctx) throws IOException {
        // Lee cabecera (16 bytes)
        byte version = in.readByte();
        byte type    = in.readByte();
        int  _res    = in.readUnsignedShort();
        int  fromId  = in.readInt();
        int  gameId  = in.readInt();
        int  len     = in.readInt();

        if (version != Proto.VERSION) throw new IOException("Bad version");
        if (len < 0 || len > ctx.maxPayload) throw new IOException("Bad len");

        byte[] payload = (len>0)? in.readNBytes(len) : new byte[0];

        Handler h = handlers.get(type);
        if (h != null) h.handle(payload, ctx);
        else ctx.log("Unknown type="+type+" len="+len);
    }

    // ---- Handlers ----

    /* 
    // PLAYER_PROPOSED: (tick:u32)(x:i16)(y:i16)(vx:i16)(vy:i16)(flags:u8)
    private void onPlayerProposed(byte[] p, Session ctx) throws IOException {
        if (p.length < 4+2+2+2+2+1) { ctx.log("short PLAYER_PROPOSED"); return; }
        DataInputStream din = new DataInputStream(new ByteArrayInputStream(p));
        int   tick = din.readInt();
        short x    = din.readShort();
        short y    = din.readShort();
        short vx   = din.readShort();
        short vy   = din.readShort();
        int flags  = din.readUnsignedByte();

        // 1) aplica inputs/estado propuesto a tu modelo
        Player pl = server.getPlayer(ctx.clientId);
        server.integrateProposed(pl, x, y, vx, vy, flags);

        // 2) colisiones autoritativas (plataformas, lianas, enemigos, frutas)
        server.resolveCollisions(pl, tick);

        // 3) envía corrección autoritativa (TLV) de vuelta
        messenger.sendStateCorrectedTLV(ctx.clientId, ctx.out(), pl, tick);
    }

    private void onPing(byte[] p, Session ctx) throws IOException {
        // espejo simple
        Proto.writeHeader(ctx.out(), MsgType.PONG, ctx.clientId, 0, p.length);
        if (p.length>0) ctx.out().write(p);
        ctx.out().flush();
    } */
}
