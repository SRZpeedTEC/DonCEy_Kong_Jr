package MessageManagement;
import serverJava.GameServer;
import java.io.*;

import Utils.Rect;
import Utils.MsgType;
import Messages.OutboundMessage;
import Messages.factories.CrocodileFactory;
import Messages.factories.FruitFactory;
import serverJava.ClientRole;


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

    public void sendSpawnCroc(Session session, byte variant, int x, int y) throws IOException {
        DataOutputStream out = session.out();
        OutboundMessage message = crocFactory.spawn(variant, x, y);
        byte[] pl = message.payload();
        Proto.writeHeader(session.out(), message.type(), session.clientId(), 0, pl.length);
        if (pl.length > 0) session.out().write(pl);
        out.flush();

    }


    public void sendSpawnFruit(Session session, byte variant, int x, int y) throws IOException {
        DataOutputStream out = session.out();
        OutboundMessage message = fruitFactory.spawn(variant, x, y);
        byte[] pl = message.payload();
        Proto.writeHeader(out, message.type(), session.clientId(), 0, pl.length);
        if (pl.length > 0) out.write(pl);
        out.flush();
    }   

    public void sendRemoveFruit(Session session, int x, int y) throws IOException {
        byte[] pl = new byte[]{
            (byte)(x>>8),(byte)x,
            (byte)(y>>8),(byte)y
        };
        Proto.writeHeader(session.out(), MsgType.REMOVE_FRUIT, session.clientId(), 0, pl.length);
        session.out().write(pl);
        session.out().flush();
    }

    public void sendRespawnDeath(Session session) throws IOException {
        Proto.writeHeader(session.out(), MsgType.PLAYER_RESPAWN, session.clientId(), 0, 0);
        session.out().flush();
    }

    public void sendLivesUpdate(Session s, byte lives) throws IOException {
        DataOutputStream out = s.out();
        Proto.writeHeader(out, MsgType.LIVES_UPDATE, s.clientId(), 0, 1);
        out.writeByte(lives);
        out.flush();
    }

    public void sendScoreUpdate(Session s, int score) throws IOException {
        DataOutputStream out = s.out();
        Proto.writeHeader(out, MsgType.SCORE_UPDATE, s.clientId(), 0, 4);
        out.writeInt(score);
        out.flush();
    }

    public void sendGameOver(Session session) throws IOException {
        Proto.writeHeader(session.out(), MsgType.PLAYER_GAME_OVER, session.clientId(), 0, 0);
        session.out().flush();
    }

    public void sendRespawnWin(Session session) throws IOException {
        Proto.writeHeader(session.out(), MsgType.RESPAWN_VICTORY, session.clientId(), 0, 0);
        session.out().flush();
    }

    public void sendSpectatorState(Session s, short x, short y, short vx, short vy, byte flags) throws IOException 
    {

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        DataOutputStream out = new DataOutputStream(baos);

        // Payload layout: x, y, vx, vy as int16, flags as byte => 9 bytes
        out.writeShort(x);
        out.writeShort(y);
        out.writeShort(vx);
        out.writeShort(vy);
        out.writeByte(flags);

        byte[] payload = baos.toByteArray();

        // Header: dest = this session's clientId; gameId = 0 (or playerClientId if you want)
        Proto.writeHeader(s.out(),
                        MsgType.SPECTATOR_STATE,
                        s.clientId(),
                        0,
                        payload.length);

        s.out().write(payload);
        s.out().flush();
    }

    // --- CLIENT_ACK with role byte and optional slot info ---
    public void sendClientAck(Session s, ClientRole role) throws IOException {
        sendClientAck(s, role, 0, 0, false, false);
    }

    // Extended CLIENT_ACK with slot availability for spectators
    public void sendClientAck(Session s, ClientRole role, 
                             int player1SpecCount, int player2SpecCount,
                             boolean player1Active, boolean player2Active) throws IOException {
        byte roleByte = 0;
        if (role == ClientRole.PLAYER) {
            roleByte = 1;
        } else if (role == ClientRole.SPECTATOR) {
            roleByte = 2;
        }
        // roleByte = 0 means rejected

        // Payload format for spectators:
        // Byte 0: roleByte (0=rejected, 1=player, 2=spectator)
        // Byte 1: player1 spectator count (0-2) or 255 if inactive
        // Byte 2: player2 spectator count (0-2) or 255 if inactive
        int payloadLen = 3;

        // Header
        Proto.writeHeader(
                s.out(),
                MsgType.CLIENT_ACK,
                s.clientId(),
                0,
                payloadLen
        );

        // Payload
        s.out().writeByte(roleByte);
        s.out().writeByte(player1Active ? player1SpecCount : 255);
        s.out().writeByte(player2Active ? player2SpecCount : 255);
        s.out().flush();
    }

    public void sendCrocSpeedIncrease(Session session) throws IOException {
        Proto.writeHeader(session.out(), MsgType.CROC_SPEED_INCREASE, session.clientId(), 0, 0);
        session.out().flush();
    }

    public void sendGameRestart(Session session) throws IOException {
        Proto.writeHeader(session.out(), MsgType.GAME_RESTART, session.clientId(), 0, 0);
        session.out().flush();
    }

}