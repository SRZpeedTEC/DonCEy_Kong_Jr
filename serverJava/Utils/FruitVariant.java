package Utils;

/**
 * Fruit sprite/logic variant codes used on the wire and client rendering.
 * Values are stable protocol bytes; do not reorder without updating both
 * server and clients.
 */
public enum FruitVariant {
    /** Banana fruit (protocol code {@code 1}). */
    BANANA((byte) 1),
    /** Apple fruit (protocol code {@code 2}). */
    APPLE((byte) 2),
    /** Orange fruit (protocol code {@code 3}). */
    ORANGE((byte) 3);

    /** Protocol byte associated with this variant. */
    public final byte code;

    /**
     * Creates a fruit variant with a fixed protocol code.
     * @param c protocol byte (stable over the network)
     */
    FruitVariant(byte c) { this.code = c; }
}
