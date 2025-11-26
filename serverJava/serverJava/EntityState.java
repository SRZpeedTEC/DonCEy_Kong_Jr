package serverJava;

public class EntityState {
    public byte variant;
    public int x, y;
    public EntityState(byte v, int x, int y){
        this.variant=v; this.x=x; this.y=y;
    }
}
