package Utils;

/**
 * Crocodile sprite/logic variant codes used on the wire and client rendering.
 * Values are stable protocol bytes; do not reorder without updating both
 * server and clients.
 */
public enum CrocVariant {
    /** Red crocodile variant (protocol code {@code 1}). */
    RED((byte) 1),
    /** Blue crocodile variant (protocol code {@code 2}). */
    BLUE((byte) 2);

    /** Protocol byte associated with this variant. */
    public final byte code;

    /**
     * Creates a croc variant with a fixed protocol code.
     * @param c protocol byte (stable over the network)
     */
    CrocVariant(byte c) { this.code = c; }
}
