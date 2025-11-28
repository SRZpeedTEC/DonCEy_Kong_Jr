package Messages;

/**
 * Contract for factories that construct outbound messages for spawnable entities.
 */
public interface EntityMessageFactory {

    /**
     * Creates an {@link OutboundMessage} that instructs clients to spawn an entity
     * of the given variant at the specified world position.
     *
     * @param variant entity variant identifier (implementation-defined)
     * @param x world X coordinate in pixels
     * @param y world Y coordinate in pixels
     * @return a ready-to-send {@link OutboundMessage}
     */
    OutboundMessage spawn(byte variant, int x, int y);
}
