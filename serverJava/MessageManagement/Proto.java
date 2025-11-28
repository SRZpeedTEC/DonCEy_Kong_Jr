package MessageManagement;

import java.io.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import Utils.Rect;

/**
 * Binary protocol helpers for composing framed messages and simple TLV records.
 * <p>
 * Frame header layout (big-endian fields except where noted by {@link DataOutputStream} behavior):
 * <pre>
 * byte   version       (link version)
 * byte   type          (message type)
 * uint16 reserved      (0)
 * int32  destClientId  (recipient id; reuse as needed)
 * int32  gameId        (game/session id)
 * int32  payloadLen    (byte length of the payload that follows)
 * </pre>
 * TLV layout:
 * <pre>
 * uint8  type
 * uint16 length
 * byte[] value
 * </pre>
 */
public final class Proto {
    /**
     * Protocol version byte written into every frame header.
     */
    public static final byte VERSION = 1;

    private Proto() { /* no instances */ }

    /**
     * Writes a full frame header to the provided stream.
     *
     * @param out          destination stream (not null)
     * @param type         application-defined message type
     * @param destClientId recipient client id (semantics defined by caller)
     * @param gameId       logical game/session id
     * @param payloadLen   number of bytes that will follow this header
     * @throws IOException if the stream cannot be written
     */
    public static void writeHeader(DataOutputStream out, byte type, int destClientId, int gameId, int payloadLen) throws IOException {
        out.writeByte(VERSION);
        out.writeByte(type);
        out.writeShort(0);
        out.writeInt(destClientId);
        out.writeInt(gameId);
        out.writeInt(payloadLen);
    }

    /**
     * Writes an unsigned 16-bit integer (0..65535) in big-endian order.
     *
     * @param out target stream
     * @param v   value to write (only low 16 bits are used)
     * @throws IOException if the stream cannot be written
     */
    public static void writeU16(DataOutputStream out, int v) throws IOException {
        out.writeShort(v & 0xFFFF);
    }

    /**
     * Writes a rectangle as four unsigned 16-bit fields (x, y, w, h).
     *
     * @param out target stream
     * @param r   rectangle source (x,y,w,h must fit in 16-bit unsigned)
     * @throws IOException if the stream cannot be written
     */
    public static void writeRect(DataOutputStream out, Rect r) throws IOException {
        writeU16(out, r.x());
        writeU16(out, r.y());
        writeU16(out, r.w());
        writeU16(out, r.h());
    }

    /**
     * Writes a single TLV record: 1 byte type, 2 bytes length, then value bytes.
     *
     * @param out   target stream
     * @param type  TLV type (only low 8 bits are written)
     * @param value TLV value buffer (length must fit in 16 bits)
     * @throws IOException if writing fails
     */
    public static void writeTLV(DataOutputStream out, int type, byte[] value) throws IOException {
        out.writeByte(type & 0xFF);
        out.writeShort(value.length & 0xFFFF);
        out.write(value);
    }

    /**
     * Encodes a signed 32-bit integer as a 4-byte big-endian array.
     *
     * @param v value to encode
     * @return big-endian byte array of length 4
     */
    public static byte[] bbU32(int v){
        return ByteBuffer.allocate(4).order(ByteOrder.BIG_ENDIAN).putInt(v).array();
    }

    /**
     * Encodes a signed 16-bit integer as a 2-byte big-endian array.
     *
     * @param v value to encode
     * @return big-endian byte array of length 2
     */
    public static byte[] bbI16(short v){
        return ByteBuffer.allocate(2).order(ByteOrder.BIG_ENDIAN).putShort(v).array();
    }
}
