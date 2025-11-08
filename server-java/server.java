import java.io.*;
import java.net.*;


public class JavaServer {
    private int port = 1234;

    public JavaServer(int port) {this.port = port; }
    public boolean running = true;


    // establish server socket and listen for connections
    public void start() throws IOException {

        ExecutorService pool = Executors.newCachedThreadPool(); // thread pool for handling clients

        try(ServerSocket DonCEyServer = new ServerSocket(port)) {
            System.out.println("Server started on port " + port);
            while (running) {
                Socket clientSocket = DonCEyServer.accept();
                pool.submit(new ClientHandler(clientSocket));

            }
        } finally {
            pool.shutdown();
        }
    }


    private void ClientHandler(Socket clientSocket) {
        try(clientSocket) {
            BufferedReader Request = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
            PrintWriter Response = new PrintWriter(clientSocket.getOutputStream(), true);

            System.out.println("Client connected: " + clientSocket.getRemoteSocketAddress());

            String line = Request.readLine(); // espera "HELLO\n"


            if ("HELLO".equals(line)) {
                Response.write("WELCOME\n");
                Response.flush();
            } else {
                Response.write("ERR\n");
                Response.flush();
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }




}

