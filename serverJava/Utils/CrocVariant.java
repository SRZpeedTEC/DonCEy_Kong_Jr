package Utils;

public enum CrocVariant {
    RED((byte)1), BLUE((byte)2);
    public final byte code;
    CrocVariant(byte c){ this.code=c; }
}
