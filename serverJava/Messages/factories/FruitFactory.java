package Messages.factories;

import Messages.OutboundMessage;
import Messages.EntityMessageFactory;
import Utils.MsgType;

// Factory class to create fruit-related messages
public final class FruitFactory implements EntityMessageFactory {
    @Override public OutboundMessage spawn(byte variant, int x, int y) {
        byte[] p = new byte[5];
        p[0] = variant;
        p[1] = (byte)(x >> 8); p[2] = (byte)x;
        p[3] = (byte)(y >> 8); p[4] = (byte)y;

        return new OutboundMessage() {
            public byte type()    { return MsgType.FRUIT_SPAWN; }
            public byte[] payload(){ return p; }
        };
    }
}