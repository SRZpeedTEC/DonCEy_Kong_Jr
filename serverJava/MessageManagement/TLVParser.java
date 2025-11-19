package MessageManagement;

public class TLVParser {
    private final byte[] buf;
    private int pos;

    public TLVParser(byte[] buf) {
        this.buf = buf;
        this.pos = 0;
    }

    public int remaining() {
        return buf.length - pos;
    }

    public TLV next() {
        if (remaining() < 4) return null;

        byte type = buf[pos];
        int len = ((buf[pos+1] & 0xFF) << 8) | (buf[pos+2] & 0xFF);
        int valueStart = pos + 3;

        if (remaining() < 3 + len) return null;

        byte[] value = new byte[len];
        System.arraycopy(buf, valueStart, value, 0, len);

        pos += 3 + len;

        return new TLV(type, len, value);
    }

    public static class TLV {
        public final byte type;
        public final int length;
        public final byte[] value;

        TLV(byte type, int length, byte[] value) {
            this.type = type;
            this.length = length;
            this.value = value;
        }
    }
}
