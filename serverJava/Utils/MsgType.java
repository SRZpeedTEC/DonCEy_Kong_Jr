package Utils;

public final class MsgType {
        public static final byte CLIENT_ACK      = 0x01;   // server->client
        public static final byte INIT_STATIC     = 0x02;   // server->client (LEGACY: payload fijo)
        public static final byte STATE_BUNDLE    = 0x10;   // server->client (TLV dentro)
        public static final byte PLAYER_PROPOSED = 0x20;   // client->server (inputs/estado propuesto)
        public static final byte PING            = 0x7E;
        public static final byte PONG            = 0x7F;
        private MsgType(){}
    }
