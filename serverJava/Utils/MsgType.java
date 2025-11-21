package Utils;

public final class MsgType {
        public static final byte CLIENT_ACK      = 0x01;   // server->client
        public static final byte INIT_STATIC     = 0x02;   // server->client (LEGACY: payload fijo)
        public static final byte STATE_BUNDLE    = 0x10;   // server->client (TLV dentro)
        public static final byte TLV_STATE_HEADER   = 0x10;
        public static final byte TLV_PLAYER_CORR    = 0x11;
        public static final byte TLV_ENTITIES_CORR  = 0x12;
        public static final byte PLAYER_PROPOSED = 0x20;   
        public static final byte CROC_SPAWN      = 0x30;   // server->client (spawn crocodile)
        public static final byte FRUIT_SPAWN     = 0x40;   // server->client (spawn fruit)
        public static final byte REMOVE_FRUIT    = 0x41;  // server->client (remove fruit)
        public static final byte SPECTATOR_STATE = 0x50;   // server->spectator (player state update)
        public static final byte SPECTATE_REQUEST = 0x51;  // client->server (spectator choose player)

        public static final byte NOTIFY_DEATH_COLLISION = 0x60; // server->client (notify death by collision)
        public static final byte NOTIFY_VICTORY = 0x61; // server->client (notify victory)
        public static final byte NOTIFY_FRUIT_PICK = 0x62; // server->client (notify fruit pick)


        public static final byte PLAYER_RESPAWN  = 0x70;   // server->client (respawn player)
        public static final byte PLAYER_GAME_OVER  = 0x71;   // server->client (game over for player)
        public static final byte RESPAWN_VICTORY = 0x72; // server->client (increase enemy speed)
        public static final byte LIVES_UPDATE    = 0x73; // server->client (update lives)
        public static final byte SCORE_UPDATE    = 0x74; // server->client (update score)
        
        private MsgType(){}
    }
