package Messages;

public interface EntityMessageFactory {
    
    OutboundMessage spawn(byte variant, int x, int y);
}