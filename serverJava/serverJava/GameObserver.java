package serverJava;

import Classes.Player.player;

public interface GameObserver {
    void onPlayerState(int playerId, player state);
}
