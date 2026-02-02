#define _CRT_SECURE_NO_WARNINGS
#include "WebServer.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

/* ================== URL DECODE ================== */
string WebServer::urlDecode(const string& str) {
    string result;
    char ch;
    int ii;

    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '%') {
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            result += ch;
            i += 2;
        }
        else if (str[i] == '+') {
            result += ' ';
        }
        else {
            result += str[i];
        }
    }
    return result;
}

/* ================== CONSTRUCTOR ================== */

WebServer::WebServer()
    : server_port(0)
    , server_fd(INVALID_SOCKET)
    , is_running(false)
{
}

WebServer::~WebServer() {
    stop();
}

/* ================== WINSOCK ================== */

bool WebServer::initWinsock() {
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
}

void WebServer::cleanupWinsock() {
    WSACleanup();
}

/* ================== SERVER ================== */

bool WebServer::start(int port, const string&) {

    if (!initWinsock()) {
        cerr << "Winsock init error" << endl;
        return false;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd == INVALID_SOCKET) {
        cerr << "Socket error" << endl;
        cleanupWinsock();
        return false;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        cerr << "Bind error" << endl;
        closesocket(server_fd);
        cleanupWinsock();
        return false;
    }

    cout << "Сокет привязан к порту " << port << endl;

    if (listen(server_fd, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Listen error" << endl;
        closesocket(server_fd);
        cleanupWinsock();
        return false;
    }

    cout << "Сервер слушает порт " << port << endl;
    is_running = true;

    while (is_running) {
        sockaddr_in client_addr{};
        int addr_len = sizeof(client_addr);

        SOCKET client_socket = accept(server_fd, (sockaddr*)&client_addr, &addr_len);
        if (client_socket == INVALID_SOCKET) continue;

        handleClient(client_socket);
        closesocket(client_socket);
    }

    return true;
}

void WebServer::stop() {
    if (is_running) {
        is_running = false;
        closesocket(server_fd);
        cleanupWinsock();
    }
}

/* ================== REQUEST ================== */

string WebServer::readRequest(SOCKET client_socket) {
    char buffer[4096];
    int bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) return "";
    buffer[bytes] = '\0';
    return string(buffer);
}

void WebServer::sendResponse(SOCKET client_socket, const string& response) {
    send(client_socket, response.c_str(), response.size(), 0);
}

/* ================== CLIENT ================== */

void WebServer::handleClient(SOCKET client_socket) {
    string request = readRequest(client_socket);
    if (request.empty()) return;

    cout << "Запрос: " << request.substr(0, request.find('\n')) << endl;

    if (request.rfind("GET ", 0) == 0) {
        size_t start = request.find(' ') + 1;
        size_t end = request.find(' ', start);
        string path = request.substr(start, end - start);

        string query;
        size_t qpos = path.find('?');
        if (qpos != string::npos) {
            query = path.substr(qpos + 1);
            path = path.substr(0, qpos);
        }

        if (path.rfind("/api/", 0) == 0) {
            handleApi(client_socket, path, query);
            return;
        }

        if (path == "/") path = "/index.html";

        if (!sendFile(client_socket, path)) {
            send404Error(client_socket, path);
        }
    }
}

/* ================== QUERY ================== */

map<string, string> WebServer::parseQuery(const string& query) {
    map<string, string> params;
    stringstream ss(query);
    string pair;

    while (getline(ss, pair, '&')) {
        size_t eq = pair.find('=');
        if (eq != string::npos) {
            params[pair.substr(0, eq)] = pair.substr(eq + 1);
        }
    }
    return params;
}

/* ================== API ================== */

void WebServer::handleApi(SOCKET client_socket, const string& path, const string& query) {
    if (path == "/api/music") {
        handleMusicApi(client_socket, query);
        return;
    }

    sendResponse(client_socket,
        "HTTP/1.1 404 Not Found\r\n\r\nAPI not found");
}

void WebServer::handleMusicApi(SOCKET client_socket, const string& query) {
    auto params = parseQuery(query);

    if (!params.count("token") || !params.count("action")) {
        sendResponse(client_socket,
            "HTTP/1.1 400 Bad Request\r\n\r\nMissing token or action");
        return;
    }

    string token = urlDecode(params["token"]);
    string action = params["action"];

    cout << "TOKEN: [" << token << "]" << endl;

    string cmd =
        "cmd /c \""
        "\"C:\\Users\\ilyae\\AppData\\Local\\Programs\\Python\\Python39\\python.exe\" "
        "\"C:\\Users\\ilyae\\source\\repos\\Yandex-Music-Server\\Yandex-Music-Server\\fetcher_music.py\" "
        "\"" + token + "\" "
        "\"" + action + "\""
        "\"";

    cout << "CMD EXEC: " << cmd << endl;

    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) {
        sendResponse(client_socket,
            "HTTP/1.1 500 Internal Server Error\r\n\r\nPython error");
        return;
    }

    string result;
    char buffer[512];

    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }

    _pclose(pipe);

    result.erase(remove(result.begin(), result.end(), '\r'), result.end());
    result.erase(remove(result.begin(), result.end(), '\n'), result.end());

    cout << "JSON OUT:\n" << result << endl;

    sendJSONResponse(client_socket, result);
}

/* ================== FILE ================== */

bool WebServer::sendFile(SOCKET client_socket, const string& filepath) {
    string filename = filepath[0] == '/' ? filepath.substr(1) : filepath;
    if (filename.find("..") != string::npos) return false;

    ifstream file(filename, ios::binary);
    if (!file.is_open()) return false;

    string content((istreambuf_iterator<char>(file)), {});
    file.close();

    string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + to_string(content.size()) + "\r\n\r\n" +
        content;

    sendResponse(client_socket, response);
    return true;
}

void WebServer::send404Error(SOCKET client_socket, const string&) {
    sendResponse(client_socket,
        "HTTP/1.1 404 Not Found\r\n\r\n404");
}

void WebServer::sendJSONResponse(SOCKET client_socket, const string& json) {
    string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Content-Length: " + to_string(json.size()) + "\r\n"
        "Connection: close\r\n\r\n" +
        json;

    sendResponse(client_socket, response);
}
