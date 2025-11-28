package MessageManagement;

/**
 * Minimal TLV (Type-Length-Value) buffer iterator for parsing inbound payloads.
 * Buffer layout (big-endian length):
 * The parser is forward-only: each call to {@link #next()} advances the cursor.
 */
public class TLVParser {
    private final byte[] buf;
    private int pos;

    /**
     * Creates a parser over the provided byte buffer.
     *
     * @param buf source buffer containing TLV records (not modified)
     */
    public TLVParser(byte[] buf) {
        this.buf = buf;
        this.pos = 0;
    }

    /**
     * @return remaining unread bytes in the buffer.
     */
    public int remaining() {
        return buf.length - pos;
    }

    /**
     * Decodes the next TLV record if enough bytes remain.
     *
     * @return a {@link TLV} instance, or {@code null} if the buffer does not
     *         contain a full record (including value bytes)
     */
    public TLV next() {
        if (remaining() < 4) return null;

        byte type = buf[pos];
        int len = ((buf[pos + 1] & 0xFF) << 8) | (buf[pos + 2] & 0xFF);
        int valueStart = pos + 3;

        if (remaining() < 3 + len) return null;

        byte[] value = new byte[len];
        System.arraycopy(buf, valueStart, value, 0, len);

        pos += 3 + len;

        return new TLV(type, len, value);
    }

    /**
     * Immutable view of a single TLV record.
     */
    public static class TLV {
        /** Application-defined record type (0..255). */
        public final byte type;
        /** Value length in bytes. */
        public final int length;
        /** Value bytes (defensive-copied from source buffer). */
        public final byte[] value;

        TLV(byte type, int length, byte[] value) {
            this.type = type;
            this.length = length;
            this.value = value;
        }
    }
}
