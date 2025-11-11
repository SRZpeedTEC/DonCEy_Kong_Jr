package Utils;

// Inmutable, con getters implícitos: x(), y(), w(), h(), y equals/hashCode/toString
public record Rect(int x, int y, int w, int h) {
    public int left()   { return x; }
    public int right()  { return x + w; }
    public int top()    { return y; }
    public int bottom() { return y + h; }
}
