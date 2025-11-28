package Messages.factories;

import Messages.OutboundMessage;
import Messages.EntityMessageFactory;
import Utils.MsgType;

// Factory class to create crocodile-related messages
public final class CrocodileFactory implements EntityMessageFactory {
    @Override public OutboundMessage spawn(byte variant, int x, int y) {
        // payload: [variant:1][x:i16][y:i16] (big-endian)
        byte[] p = new byte[5];
        p[0] = variant;
        p[1] = (byte)(x >> 8); p[2] = (byte)x;
        p[3] = (byte)(y >> 8); p[4] = (byte)y;

        return new OutboundMessage() {
            public byte type()    { return MsgType.CROC_SPAWN; }
            public byte[] payload(){ return p; }
        };
    }
}
