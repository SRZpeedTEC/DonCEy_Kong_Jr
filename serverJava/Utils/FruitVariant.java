
package Utils;

public enum FruitVariant {
    BANANA((byte)1), APPLE((byte)2), ORANGE((byte)3);
    public final byte code;
    FruitVariant(byte c){ this.code=c; }
}
