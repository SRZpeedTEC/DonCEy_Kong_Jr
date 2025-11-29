package serverJava;

/**
 * Logical connection role of a client.
 */
public enum ClientRole {
    /** Active player controlling gameplay. */
    PLAYER,
    /** Read-only viewer attached to a player's feed. */
    SPECTATOR
}
