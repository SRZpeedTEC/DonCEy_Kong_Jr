package MessageManagement;
import java.io.DataOutputStream;


public class Session {
    public final int clientId;
    private final DataOutputStream out;
    public final int maxPayload = 64*1024;

    public Session(int clientId, DataOutputStream out){
        this.clientId = clientId; this.out = out;
    }
    public int clientId(){ return clientId; }
    public DataOutputStream out(){ return out; }
    public void log(String s){ System.out.println("[Client "+clientId+"] "+s); }
}
