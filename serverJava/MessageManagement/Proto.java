package MessageManagement;
import java.io.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import Utils.Rect;

public final class Proto {
    public static final byte VERSION = 1;

    public static void writeHeader(DataOutputStream out, byte type, int destClientId, int gameId, int payloadLen) throws IOException {
        out.writeByte(VERSION);
        out.writeByte(type);
        out.writeShort(0);            // reserved
        out.writeInt(destClientId);   // o fromId/dest, como uses
        out.writeInt(gameId);
        out.writeInt(payloadLen);
    }

    public static void writeU16(DataOutputStream out, int v) throws IOException { out.writeShort(v & 0xFFFF); }

    public static void writeRect(DataOutputStream out, Rect r) throws IOException {
        writeU16(out, r.x()); writeU16(out, r.y()); writeU16(out, r.w()); writeU16(out, r.h());
    }

    // --- TLV helpers ---
    public static void writeTLV(DataOutputStream out, int type, byte[] value) throws IOException {
        out.writeByte(type & 0xFF);
        out.writeShort(value.length & 0xFFFF);
        out.write(value);
    }

    public static byte[] bbU32(int v){
        return ByteBuffer.allocate(4).order(ByteOrder.BIG_ENDIAN).putInt(v).array();
    }
    public static byte[] bbI16(short v){
        return ByteBuffer.allocate(2).order(ByteOrder.BIG_ENDIAN).putShort(v).array();
    }
}
