package MessageManagement;

import java.io.DataOutputStream;

/**
 * Lightweight session descriptor for a connected client.
 * Holds the client identifier and the output stream used to send messages.
 */
public class Session {
    /**
     * Unique identifier of the connected client.
     */
    public final int clientId;

    private final DataOutputStream out;

    /**
     * Maximum supported payload size (bytes) for safety checks upstream.
     */
    public final int maxPayload = 64 * 1024;

    /**
     * Creates a new session wrapper.
     *
     * @param clientId client identifier assigned by the server
     * @param out      output stream bound to the client's socket
     */
    public Session(int clientId, DataOutputStream out){
        this.clientId = clientId;
        this.out = out;
    }

    /**
     * @return the client identifier.
     */
    public int clientId(){ return clientId; }

    /**
     * @return the output stream for sending data to this client.
     */
    public DataOutputStream out(){ return out; }

    /**
     * Writes a prefixed message to standard output for debugging purposes.
     *
     * @param s message to log
     */
    public void log(String s){ System.out.println("[Client " + clientId + "] " + s); }
}
