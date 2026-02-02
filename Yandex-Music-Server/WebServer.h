
#include <string>
#include <atomic>

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <mutex>
#include <map>

using namespace std;
class WebServer {
private:
    
    int server_port;
    SOCKET server_fd;  
    atomic<bool> is_running;
    string data_file_path;

   
    bool initWinsock();
    void cleanupWinsock();

    string readRequest(SOCKET client_socket);
    void sendResponse(SOCKET client_socket, const string& response);
    void handleClient(SOCKET client_socket);
    void handleApi(SOCKET client_socket, const string& path, const string& query);
    void handleMusicApi(SOCKET client_socket, const string& query);

    bool sendFile(SOCKET client_socket, const string& filepath);
    void send404Error(SOCKET  client_socket, const string& filename);
    void sendJSONResponse(SOCKET client_socket, const string& json);

    bool sendMusicData(SOCKET client_socket);
    string runPythonScript();

    map<string, string> parseQuery(const string& query);
   
    string getMimeType(const   string& filename);
    string urlDecode(const string& str);
    string readFileContent(const string& filename);
    bool fileExists(const string& filename);
    void createDirectoryIfNotExists(const string& dirname);

public:
    WebServer();
    ~WebServer();


    bool isRunning() const;
    bool start(int port, const string& data_file = "yamusic_data.json");
    void stop();

    static string executeCommand(const  string& cmd);
};
