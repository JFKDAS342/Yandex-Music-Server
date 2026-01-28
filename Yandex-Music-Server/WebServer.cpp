#include "WebServer.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <filesystem>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

WebServer::WebServer()
    : server_port(0)
    , server_fd(INVALID_SOCKET)
    , is_running(false)
{
}

WebServer::~WebServer()
{
    stop();
}

// Просто возвращаем состояние сервера
bool WebServer::isRunning() const
{
    return is_running;
}

bool WebServer::initWinsock()
{
    WSADATA wsaData;
    return (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);
}

void WebServer::cleanupWinsock()
{
    WSACleanup();
}

bool WebServer::start(int port, const string& data_file)
{


    // Инициализируем Winsock
    if (!initWinsock()) {
        cerr << "Ошибка: Не удалось инициализировать Winsock" << endl;
        return false;
    }



    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd == INVALID_SOCKET) {
        cerr << "Ошибка: Не удалось создать сокет" << endl;
        cleanupWinsock();
        return false;
    }

    // Настраиваем адрес сервера
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;           // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY;   // Принимаем со всех интерфейсов
    server_addr.sin_port = htons(port);         // Порт

    // Привязываем сокет к порту
    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        cerr << "Ошибка: Не удалось привязать сокет к порту " << port << endl;
        cerr << "Возможно порт уже занят" << endl;
        closesocket(server_fd);
        cleanupWinsock();
        return false;
    }
    cout << " Сокет привязан к порту " << port << endl;


    if (listen(server_fd, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Ошибка: Не удалось начать прослушивание" << endl;
        closesocket(server_fd);
        cleanupWinsock();
        return false;
    }
    cout << " Сервер слушает порт " << port << endl;

    // Устанавливаем флаг запуска
    server_port = port;
    is_running = true;


    while (is_running) {
        // Принимаем новое соединение
        sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);

        SOCKET client_socket = accept(server_fd,
            (sockaddr*)&client_addr,
            &addr_len);

        if (client_socket == INVALID_SOCKET) {
            if (is_running) {
                cerr << "Ошибка принятия соединения" << endl;
            }
            continue;
        }

        // Получаем IP клиента
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

        cout << " Новое соединение от " << client_ip << endl;


        handleClient(client_socket);

        // Закрываем соединение с клиентом
        closesocket(client_socket);
        cout << "   Соединение закрыто" << endl;
    }

    return true;
}

// Остановка сервера
void WebServer::stop()
{
    if (is_running) {
        cout << "\nОСТАНОВКА СЕРВЕРА" << endl;
        is_running = false;

        // Закрываем сокет
        closesocket(server_fd);

        // Очищаем Winsock
        cleanupWinsock();

        cout << "Сервер остановлен" << endl;
    }
}

string WebServer::readRequest(SOCKET client_socket)
{
    char buffer[1024];
    string request;

    // Читаем данные из сокета
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';  // Конец строки
        request = buffer;
    }

    return request;
}

void WebServer::sendResponse(SOCKET client_socket, const string& response)
{
    send(client_socket, response.c_str(), response.length(), 0);
}

// Обработка клиента
void WebServer::handleClient(SOCKET client_socket)
{
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
            path = path.substr(0 , qpos);
        }

        if (path.rfind("/api/", 0) == 0) {
            handleApi(client_socket, path, query);
            return;
        }

        if (path == "/") {
            path = "/index.html";
        }

        if (!sendFile(client_socket, path)) {
            send404Error(client_socket, path);
        }
        return;
    }

    // если не GET
    string response =
        "HTTP/1.1 400 Bad Request\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<h1>400 Bad Request</h1>";

    sendResponse(client_socket, response);
}

map<string ,string>WebServer::parseQuery(const string& query) {
    map<string, string> params;
    stringstream ss(query);
    string pair;

    while (getline(ss, pair, '&')) {
        size_t eq = pair.find('=');
        if (eq != string::npos){
            string key = pair.substr(0, eq);
            string value = pair.substr(eq + 1);
            params[key] = value;
        }
    }
    return params;
}

void WebServer::handleApi(SOCKET client_socket, const string& path, const string& query)
{
    if (path == "/api/music") {
        handleMusicApi(client_socket, query);
        return;
    }

    // если API endpoint не найден
    sendResponse(
        client_socket,
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/plain\r\n\r\n"
        "API endpoint not found"
    );
}

void WebServer::handleMusicApi(SOCKET client_socket, const string& query) {
    auto params = parseQuery(query);

    if (!params.count("token")) {
        sendResponse(client_socket, "HTTP/1.1 400 Bad Request\r\n\r\nMissing token");
        return;
    }

    string token = params["token"];

    string command =
        "cmd /c \"\"C:\\Users\\ilyae\\AppData\\Local\\Programs\\Python\\Python39\\python.exe\" "
        "\"C:\\Users\\ilyae\\source\\repos\\Yandex-Music-Server\\Yandex-Music-Server\\fetcher_music.py\" "
        + token + "\"";

    FILE* pipe = _popen(command.c_str(), "r");

    if (!pipe) {
        sendResponse(client_socket, "HTTP/1.1 500 Internal Server Error\r\n\r\nPython error");
        return;
    }

    string result;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }

    _pclose(pipe);
    cout << "PYTHON OUTPUT:\n" << result << endl;
    sendJSONResponse(client_socket, result);
}




bool WebServer::sendFile(SOCKET client_socket, const string& filepath)
{
    // Убираем первый слеш если есть
    string filename = filepath;
    if (!filename.empty() && filename[0] == '/') {
        filename = filename.substr(1);
    }

    // Защита от "../"
    if (filename.find("..") != string::npos) {
        return false;
    }

    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cout << "   Файл не найден: " << filename << endl;
        return false;
    }

    cout << "Отправляем файл: " << filename << endl;

    // Читаем весь файл
    string content((istreambuf_iterator<char>(file)),
        istreambuf_iterator<char>());
    file.close();

    // Определяем тип файла по расширению
    string content_type = "text/plain";
    if (filename.find(".html") != string::npos ||
        filename.find(".htm") != string::npos) {
        content_type = "text/html";
    }
    else if (filename.find(".css") != string::npos) {
        content_type = "text/css";
    }
    else if (filename.find(".js") != string::npos) {
        content_type = "application/javascript";
    }
    else if (filename.find(".json") != string::npos) {
        content_type = "application/json";
    }
    else if (filename.find(".png") != string::npos) {
        content_type = "image/png";
    }
    else if (filename.find(".jpg") != string::npos ||
        filename.find(".jpeg") != string::npos) {
        content_type = "image/jpeg";
    }

    // Формируем и отправляем ответ
    string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: " + content_type + "\r\n"
        "Content-Length: " + to_string(content.length()) + "\r\n"
        "\r\n" + content;

    sendResponse(client_socket, response);
    return true;
}

void WebServer::send404Error(SOCKET client_socket, const string& filename) {
    string response = "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/html\r\n"
        "\r\n"
        "<html><body>"
        "<h1>404 - Страница не найдена</h1>"
        "<p>Файл " + filename + " не существует</p>"
        "<a href='/'>Вернуться на главную</a>"
        "</body></html>";
    sendResponse(client_socket, response);
}

void WebServer::sendJSONResponse(SOCKET client_socket, const string& json) {
    string response = "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + to_string(json.length()) + "\r\n"
        "\r\n" + json;
    sendResponse(client_socket, response);
}