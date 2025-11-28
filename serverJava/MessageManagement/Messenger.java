package MessageManagement;

import serverJava.GameServer;
import java.io.*;

import Utils.Rect;
import Utils.MsgType;
import Messages.OutboundMessage;
import Messages.factories.CrocodileFactory;
import Messages.factories.FruitFactory;
import serverJava.ClientRole;

/**
 * High-level message sender responsible for building and dispatching
 * server→client protocol frames. This class encapsulates payload assembly,
 * header writing, and flushing for all outbound events (init state, spawns,
 * HUD updates, game lifecycle, spectator snapshots, etc.).
 *
 */
public class Messenger {

    /** Owning game server providing authoritative state snapshots. */
    private final GameServer server;
    /** Factory for crocodile-related outbound messages. */
    private final CrocodileFactory crocFactory = new CrocodileFactory();
    /** Factory for fruit-related outbound messages. */
    private final FruitFactory fruitFactory = new FruitFactory();

    /**
     * Creates a messenger bound to a given server instance.
     * @param server authoritative game server (non-null)
     */
    public Messenger(GameServer server){ this.server = server; }

    /**
     * Sends the legacy INIT_STATIC frame (server → client) with
     * the full static geometry and current dynamic lists.
     *
     * Payload layout:
     * playerRect(8B)
     * u16 nPlatforms, nPlatforms * rect(8B)
     * u16 nVines,     nVines     * rect(8B)
     * u16 nCrocs,     nCrocs     * rect(8B)
     * u16 nFruits,    nFruits    * rect(8B)
     *
     * @param destClientId destination client id
     * @param out stream to write to
     * @throws IOException on I/O error or disconnection
     */
    public void sendInitStaticLegacy(int destClientId, DataOutputStream out) throws IOException {
        int nP = server.platforms.size(), nV = server.vines.size(),
            nE = server.crocodiles.size(),   nF = server.fruits.size();

        final int rectBytes = 8;
        int payloadLen = 8
                       + 2 + nP*rectBytes
                       + 2 + nV*rectBytes
                       + 2 + nE*rectBytes
                       + 2 + nF*rectBytes;

        Proto.writeHeader(out, MsgType.INIT_STATIC, destClientId, 0, payloadLen);

        Proto.writeRect(out, server.player);
        Proto.writeU16(out, nP); for (Rect r: server.platforms)  Proto.writeRect(out, r);
        Proto.writeU16(out, nV); for (Rect r: server.vines)      Proto.writeRect(out, r);
        Proto.writeU16(out, nE); for (Rect r: server.crocodiles) Proto.writeRect(out, r);
        Proto.writeU16(out, nF); for (Rect r: server.fruits)     Proto.writeRect(out, r);

        out.flush();
    }

    /**
     * Sends a crocodile spawn event for a given session.
     * Payload structure is provided by {@link CrocodileFactory}.
     *
     * @param session destination session
     * @param variant crocodile variant code
     * @param x spawn X (pixels)
     * @param y spawn Y (pixels)
     * @throws IOException on I/O error
     */
    public void sendSpawnCroc(Session session, byte variant, int x, int y) throws IOException {
        DataOutputStream out = session.out();
        OutboundMessage message = crocFactory.spawn(variant, x, y);
        byte[] pl = message.payload();
        Proto.writeHeader(session.out(), message.type(), session.clientId(), 0, pl.length);
        if (pl.length > 0) session.out().write(pl);
        out.flush();
    }

    /**
     * Sends a fruit spawn event for a given session.
     * Payload structure is provided by {@link FruitFactory}.
     *
     * @param session destination session
     * @param variant fruit variant code
     * @param x spawn X (pixels)
     * @param y spawn Y (pixels)
     * @throws IOException on I/O error
     */
    public void sendSpawnFruit(Session session, byte variant, int x, int y) throws IOException {
        DataOutputStream out = session.out();
        OutboundMessage message = fruitFactory.spawn(variant, x, y);
        byte[] pl = message.payload();
        Proto.writeHeader(out, message.type(), session.clientId(), 0, pl.length);
        if (pl.length > 0) out.write(pl);
        out.flush();
    }

    /**
     * Sends a request to remove a fruit at a given position.
     *
     * Payload layout (4 bytes):
     * x: int16 (big-endian), y: int16 (big-endian)
     *
     * @param session destination session
     * @param x fruit X
     * @param y fruit Y
     * @throws IOException on I/O error
     */
    public void sendRemoveFruit(Session session, int x, int y) throws IOException {
        byte[] pl = new byte[]{
            (byte)(x>>8),(byte)x,
            (byte)(y>>8),(byte)y
        };
        Proto.writeHeader(session.out(), MsgType.REMOVE_FRUIT, session.clientId(), 0, pl.length);
        session.out().write(pl);
        session.out().flush();
    }

    /**
     * Notifies a client to respawn after death (no payload).
     *
     * @param session destination session
     * @throws IOException on I/O error
     */
    public void sendRespawnDeath(Session session) throws IOException {
        Proto.writeHeader(session.out(), MsgType.PLAYER_RESPAWN, session.clientId(), 0, 0);
        session.out().flush();
    }

    /**
     * Sends the current lives counter to a client HUD.
     *
     * @param s destination session
     * @param lives remaining lives (0..n)
     * @throws IOException on I/O error
     */
    public void sendLivesUpdate(Session s, byte lives) throws IOException {
        DataOutputStream out = s.out();
        Proto.writeHeader(out, MsgType.LIVES_UPDATE, s.clientId(), 0, 1);
        out.writeByte(lives);
        out.flush();
    }

    /**
     * Sends the current score to a client HUD.
     *
     * Payload layout: 4 bytes (int32).
     *
     * @param s destination session
     * @param score score value
     * @throws IOException on I/O error
     */
    public void sendScoreUpdate(Session s, int score) throws IOException {
        DataOutputStream out = s.out();
        Proto.writeHeader(out, MsgType.SCORE_UPDATE, s.clientId(), 0, 4);
        out.writeInt(score);
        out.flush();
    }

    /**
     * Notifies a client of game over (no payload).
     *
     * @param session destination session
     * @throws IOException on I/O error
     */
    public void sendGameOver(Session session) throws IOException {
        Proto.writeHeader(session.out(), MsgType.PLAYER_GAME_OVER, session.clientId(), 0, 0);
        session.out().flush();
    }

    /**
     * Notifies a client of victory respawn (no payload).
     *
     * @param session destination session
     * @throws IOException on I/O error
     */
    public void sendRespawnWin(Session session) throws IOException {
        Proto.writeHeader(session.out(), MsgType.RESPAWN_VICTORY, session.clientId(), 0, 0);
        session.out().flush();
    }

    /**
     * Sends a compact spectator snapshot (position, velocity, flags).
     *
     * Payload layout (9 bytes total):
     * x  : int16
     * y  : int16
     * vx : int16
     * vy : int16
     * f  : byte  (bitfield: 0x01 grounded, 0x02 justDied, 0x04 onVine, 0x08 betweenVines, etc.)
     *
     * @param s destination spectator session
     * @param x player x (int16)
     * @param y player y (int16)
     * @param vx player vx (int16)
     * @param vy player vy (int16)
     * @param flags packed state flags
     * @throws IOException on I/O error
     */
    public void sendSpectatorState(Session s, short x, short y, short vx, short vy, byte flags) throws IOException {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        DataOutputStream out = new DataOutputStream(baos);

        out.writeShort(x);
        out.writeShort(y);
        out.writeShort(vx);
        out.writeShort(vy);
        out.writeByte(flags);

        byte[] payload = baos.toByteArray();

        Proto.writeHeader(s.out(), MsgType.SPECTATOR_STATE, s.clientId(), 0, payload.length);
        s.out().write(payload);
        s.out().flush();
    }

    /**
     * Sends a basic client acknowledgment indicating accepted role.
     * Equivalent to calling the extended overload with default slot info.
     *
     * @param s session
     * @param role accepted role (PLAYER / SPECTATOR)
     * @throws IOException on I/O error
     */
    public void sendClientAck(Session s, ClientRole role) throws IOException {
        sendClientAck(s, role, 0, 0, false, false);
    }

    /**
     * Sends client acknowledgment with spectator slot availability per player.
     *
     * Payload layout (3 bytes):
     * [0] roleByte: 0=rejected, 1=player, 2=spectator
     * [1] player1SpectatorCount 
     * [2] player2SpectatorCount 
     *
     * @param s session
     * @param role accepted role
     * @param player1SpecCount spectators on player 1 (0..2)
     * @param player2SpecCount spectators on player 2 (0..2)
     * @param player1Active whether player 1 slot is active
     * @param player2Active whether player 2 slot is active
     * @throws IOException on I/O error
     */
    public void sendClientAck(Session s, ClientRole role,
                              int player1SpecCount, int player2SpecCount,
                              boolean player1Active, boolean player2Active) throws IOException {
        byte roleByte = 0;
        if (role == ClientRole.PLAYER) {
            roleByte = 1;
        } else if (role == ClientRole.SPECTATOR) {
            roleByte = 2;
        }

        int payloadLen = 3;

        Proto.writeHeader(
                s.out(),
                MsgType.CLIENT_ACK,
                s.clientId(),
                0,
                payloadLen
        );

        s.out().writeByte(roleByte);
        s.out().writeByte(player1Active ? player1SpecCount : 255);
        s.out().writeByte(player2Active ? player2SpecCount : 255);
        s.out().flush();
    }

    /**
     * Notifies clients to increase crocodile speed (no payload).
     *
     * @param session destination session
     * @throws IOException on I/O error
     */
    public void sendCrocSpeedIncrease(Session session) throws IOException {
        Proto.writeHeader(session.out(), MsgType.CROC_SPEED_INCREASE, session.clientId(), 0, 0);
        session.out().flush();
    }

    /**
     * Broadcasts a game restart event (no payload).
     *
     * @param session destination session
     * @throws IOException on I/O error
     */
    public void sendGameRestart(Session session) throws IOException {
        Proto.writeHeader(session.out(), MsgType.GAME_RESTART, session.clientId(), 0, 0);
        session.out().flush();
    }
}
