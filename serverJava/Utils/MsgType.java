package Utils;

/**
 * Message type codes shared by server and clients.
 * <p>
 * These values are part of the on-the-wire protocol and must remain stable.
 * Directions below indicate the typical sender → receiver flow.
 */
public final class MsgType {

    /** Server → client: connection acknowledgment (also used to convey role/slot info). */
    public static final byte CLIENT_ACK         = 0x01;

    /** Server → client: legacy static-map bootstrap (player start, platforms, vines, etc.). */
    public static final byte INIT_STATIC        = 0x02;

    /** Server → client: bundle container for state TLVs (alias of {@link #TLV_STATE_HEADER}). */
    public static final byte STATE_BUNDLE       = 0x10;

    /** Server → client: TLV header tag for a state bundle (same code as {@link #STATE_BUNDLE}). */
    public static final byte TLV_STATE_HEADER   = 0x10;

    /** Server → client: TLV carrying player correction (e.g., y snap, velocity fix). */
    public static final byte TLV_PLAYER_CORR    = 0x11;

    /** Server → client: TLV carrying entities correction (spawn/despawn sync, etc.). */
    public static final byte TLV_ENTITIES_CORR  = 0x12;

    /** Client → server: player's proposed state for this tick (x, y, vx, vy, flags). */
    public static final byte PLAYER_PROPOSED    = 0x20;

    /** Server → client: spawn a crocodile (variant, x, y). */
    public static final byte CROC_SPAWN         = 0x30;

    /** Server → client: spawn a fruit (variant, x, y). */
    public static final byte FRUIT_SPAWN        = 0x40;

    /** Server → client: remove a fruit at coordinates (x, y). */
    public static final byte REMOVE_FRUIT       = 0x41;

    /** Server → spectator: forward live player state (x, y, vx, vy, flags). */
    public static final byte SPECTATOR_STATE    = 0x50;

    /** Client → server: spectator asks to attach to a player slot (1 or 2). */
    public static final byte SPECTATE_REQUEST   = 0x51;

    /** Client → server: notify death due to collision (server drives lives/respawn). */
    public static final byte NOTIFY_DEATH_COLLISION = 0x60;

    /** Client → server: notify victory (server adjusts lives, round state, speed, etc.). */
    public static final byte NOTIFY_VICTORY     = 0x61;

    /** Client → server: notify fruit pick (payload carries fruit coordinates). */
    public static final byte NOTIFY_FRUIT_PICK  = 0x62;

    /** Server → client: respawn after death. */
    public static final byte PLAYER_RESPAWN     = 0x70;

    /** Server → client: game over for this player. */
    public static final byte PLAYER_GAME_OVER   = 0x71;

    /** Server → client: respawn sequence after a win (next round start). */
    public static final byte RESPAWN_VICTORY    = 0x72;

    /** Server → client: update remaining lives (HUD sync). */
    public static final byte LIVES_UPDATE       = 0x73;

    /** Server → client: update score (HUD sync). */
    public static final byte SCORE_UPDATE       = 0x74;

    /** Server → client: inform clients that crocodile speed increased. */
    public static final byte CROC_SPEED_INCREASE = 0x75;

    /** Client → server: player requests a full game restart. */
    public static final byte REQUEST_RESTART    = 0x76;

    /** Server → client: broadcast game restart to player and attached spectators. */
    public static final byte GAME_RESTART       = 0x77;

    private MsgType() {}
}
