package MessageManagement;

import java.io.*;
import java.util.*;
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


    public void processFrame(DataInputStream in, Session sess) throws IOException {
        byte version = in.readByte();
        byte type    = in.readByte();
        int  _res    = in.readUnsignedShort();
        int  fromId  = in.readInt();
        int  gameId  = in.readInt();
        int  len     = in.readInt();

        // 1) Tipos con payload estructurado conocido
        if (type == MsgType.PLAYER_PROPOSED) {
            int   tick  = in.readInt();
            short x     = in.readShort();
            short y     = in.readShort();
            short vx    = in.readShort();
            short vy    = in.readShort();
            byte  flags = in.readByte();

            player p1 = server.getPlayerFromServer(sess.clientId());
            if (p1 != null) { p1.x = x; p1.y = y; p1.vx = vx; p1.vy = vy; }
            server.broadcastPlayerStateToSpectators(sess.clientId(), x, y, vx, vy, flags);
            return;
        }

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

       if (type == MsgType.NOTIFY_DEATH_COLLISION){
            player p = server.getPlayerFromServer(sess.clientId());
            if (p == null) return;

            if (p.getLives() > 0) p.setLives(p.getLives() - 1);

            if (p.getLives() > 0) {
                messenger.sendRespawnDeath(sess);   // -> CP_TYPE_RESPAWN_DEATH_COLLISION
            } else {
                messenger.sendGameOver(sess);       // -> CP_TYPE_GAME_OVER
                // aquí puedes marcar estado de game over del lado server si llevas scoreboard
            }
       }

        if (type == MsgType.NOTIFY_VICTORY){
            player p = server.getPlayerFromServer(sess.clientId());
            if (p == null) return;

            p.increaseLife();               // +1 vida (según tu regla)
            messenger.sendRespawnWin(sess); // -> CP_TYPE_RESPAWN_WIN
         }


        
        if (len > 0) in.skipNBytes(len);
    }

 }
