package serverJava;

import Classes.Player.player;

/**
 * Observer for server-side player state updates.
 * Implementations can forward the latest player snapshot to external
 * consumers (e.g., spectators, logs, or monitoring tools).
 */
public interface GameObserver {
    /**
     * Notifies an observer about a player's most recent state.
     *
     * @param playerId unique client identifier of the player
     * @param state    current mutable {@link player} snapshot owned by the server
     */
    void onPlayerState(int playerId, player state);
}
