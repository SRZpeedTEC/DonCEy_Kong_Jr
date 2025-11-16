package Utils;

public final class MsgType {
        public static final byte CLIENT_ACK      = 0x01;   // server->client
        public static final byte INIT_STATIC     = 0x02;   // server->client (LEGACY: payload fijo)
        public static final byte STATE_BUNDLE    = 0x10;   // server->client (TLV dentro)
        public static final byte PLAYER_PROPOSED = 0x20;   
        public static final byte CROC_SPAWN      = 0x30;   // server->client (spawn crocodile)
        public static final byte FRUIT_SPAWN     = 0x40;   // server->client (spawn fruit)
        public static final byte REMOVE_FRUIT    = 0x41;  // server->client (remove fruit)
        private MsgType(){}
    }
