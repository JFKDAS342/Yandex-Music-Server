#include <iostream>
#include "../include/WebServer.h"

using namespace std;

int main()
{
    WebServer server;
    setlocale(LC_ALL, "ru");

    try
    {
        if (server.start(8080)) {

            cin.get();

            // Останавливаем сервер после нажатия Enter
            server.stop();
        }
        else {
            cout << "Не удалось запустить сервер" << endl;
        }
    }
    catch (const exception& e)
    {
        cerr << "Ошибка: " << e.what() << endl;
        cout << "Не удалось запустить сервер." << endl;
    }

    return 0;
}