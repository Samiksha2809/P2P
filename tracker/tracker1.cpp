#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace std;

#define PORT 8000  

map<string, string> users;  

void loadTrackerInfo(const string &filename) {
    ifstream file(filename);
    string line;
    while (getline(file, line)) {
        cout << "Tracker info: " << line << endl;
    }
}

void createUser(const string &user_id, const string &passwd, int client_socket) {
    if (users.find(user_id) == users.end()) {
        users[user_id] = passwd;
        string response = "User created successfully: " + user_id;
        send(client_socket, response.c_str(), response.size(), 0);
    } else {
        std::string response = "User already exists!";
        send(client_socket, response.c_str(), response.size(), 0);
    }
}

void loginUser(const string &user_id, const string &passwd, int client_socket) {
    if (users.find(user_id) != users.end() && users[user_id] == passwd) {
        string response = "Login successful: " + user_id;
        send(client_socket, response.c_str(), response.size(), 0);
    } else {
        string response = "Invalid user_id or password!";
        send(client_socket, response.c_str(), response.size(), 0);
    }
}

void handleClientCommands(int client_socket) {
    char buffer[1024] = {0};
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(client_socket, buffer, 1024);
        if (valread == 0) {
            // Client disconnected
            std::cout << "Client disconnected" << std::endl;
            break;
        }

        string command(buffer);
        istringstream iss(command);
        string cmd;
        iss >> cmd;

        if (cmd == "create_user") {
            string user_id, passwd;
            iss >> user_id >> passwd;
            createUser(user_id, passwd, client_socket);
        } else if (cmd == "login") {
            string user_id, passwd;
            iss >> user_id >> passwd;
            loginUser(user_id, passwd, client_socket);
        } else if (cmd == "quit") {
            string response = "Tracker shutting down...";
            send(client_socket, response.c_str(), response.size(), 0);
            close(client_socket);
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: ./tracker tracker_info.txt tracker_no" << std::endl;
        return 1;
    }

    string tracker_info = argv[1];
    int tracker_no = stoi(argv[2]);

    loadTrackerInfo(tracker_info);
    cout << "Tracker " << tracker_no << " is running..." << endl;

  
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        cout << "Socket failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        cout << "Bind failed" << endl;
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        cout << "Listen failed" << endl;
        exit(EXIT_FAILURE);
    }

    while (true) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            std::cerr << "Accept failed" << endl;
            exit(EXIT_FAILURE);
        }
        cout << "Client connected" << endl;
        handleClientCommands(new_socket);
    }

    return 0;
}
