package Messages;

public interface OutboundMessage {
    byte type();        // Utils.MsgType.*
    byte[] payload();   // bytes listos (big-endian)
}
