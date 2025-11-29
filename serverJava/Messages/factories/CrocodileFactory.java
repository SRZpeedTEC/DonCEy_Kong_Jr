package Messages.factories;

import Messages.OutboundMessage;
import Messages.EntityMessageFactory;
import Utils.MsgType;

/**
 * Factory for creating crocodile-related outbound messages.
 * <p>
 * Payload layout for {@link #spawn(byte, int, int)}:
 */
public final class CrocodileFactory implements EntityMessageFactory {

    /**
     * Builds a spawn message for a crocodile entity.
     *
     * @param variant crocodile variant identifier (implementation-defined)
     * @param x world X coordinate in pixels (encoded as signed 16-bit big-endian)
     * @param y world Y coordinate in pixels (encoded as signed 16-bit big-endian)
     * @return an {@link OutboundMessage} with type {@link MsgType#CROC_SPAWN} and a 5-byte payload
     */
    @Override
    public OutboundMessage spawn(byte variant, int x, int y) {
        byte[] p = new byte[5];
        p[0] = variant;
        p[1] = (byte)(x >> 8); p[2] = (byte)x;
        p[3] = (byte)(y >> 8); p[4] = (byte)y;

        return new OutboundMessage() {
            public byte type()     { return MsgType.CROC_SPAWN; }
            public byte[] payload(){ return p; }
        };
    }
}
