package serverJava;

/**
 * Minimal immutable snapshot for spawned dynamic entities managed by the server,
 * used for resynchronizing late joiners (e.g., spectators) and bookkeeping.
 */
public class EntityState {
    /** Variant/protocol code of the entity (e.g., croc/fruit type). */
    public byte variant;
    /** X coordinate (world pixels, top-left or entity anchor as defined by client). */
    public int x;
    /** Y coordinate (world pixels, top-left or entity anchor as defined by client). */
    public int y;

    /**
     * Constructs a new server-side entity snapshot.
     *
     * @param v protocol variant byte
     * @param x x position in world space (pixels)
     * @param y y position in world space (pixels)
     */
    public EntityState(byte v, int x, int y) {
        this.variant = v; this.x = x; this.y = y;
    }
}
