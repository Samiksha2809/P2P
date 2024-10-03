#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8000  // Same port as tracker

void loadTrackerInfo(const std::string &filename) {
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        std::cout << "Tracker info: " << line << std::endl;
    }
}

void go_to_tracker(int client_socket) {
    std::string command;
    while (true) {
        std::cout << "Enter command: ";
        std::getline(std::cin, command);
        
        if (command == "quit") {
            send(client_socket, command.c_str(), command.size(), 0);
            std::cout << "Client shutting down..." << std::endl;
            break;
        }

        // Send command to tracker
        send(client_socket, command.c_str(), command.size(), 0);

        // Receive response from tracker
        char buffer[1024] = {0};
        int valread = read(client_socket, buffer, 1024);
        std::cout << buffer << std::endl;
    }
}

void create_user(int client_socket, const std::string &user_id, const std::string &passwd) {
    std::string command = "create_user " + user_id + " " + passwd;
    send(client_socket, command.c_str(), command.size(), 0);
}

void login(int client_socket, const std::string &user_id, const std::string &passwd) {
    std::string command = "login " + user_id + " " + passwd;
    send(client_socket, command.c_str(), command.size(), 0);
}

void create_group(int client_socket, const std::string &group_id) {
    std::string command = "create_group " + group_id;
    send(client_socket, command.c_str(), command.size(), 0);
}

void join_group(int client_socket, const std::string &group_id) {
    std::string command = "join_group " + group_id;
    send(client_socket, command.c_str(), command.size(), 0);
}

void leave_group(int client_socket, const std::string &group_id) {
    std::string command = "leave_group " + group_id;
    send(client_socket, command.c_str(), command.size(), 0);
}

void list_requests(int client_socket, const std::string &group_id) {
    std::string command = "list_requests " + group_id;
    send(client_socket, command.c_str(), command.size(), 0);
}

void accept_request(int client_socket, const std::string &group_id, const std::string &user_id) {
    std::string command = "accept_request " + group_id + " " + user_id;
    send(client_socket, command.c_str(), command.size(), 0);
}

void list_groups(int client_socket) {
    std::string command = "list_groups";
    send(client_socket, command.c_str(), command.size(), 0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: ./client <IP>:<PORT> tracker_info.txt" << std::endl;
        return 1;
    }

    std::string server_ip_port = argv[1];
    std::string tracker_info = argv[2];

    loadTrackerInfo(tracker_info);
    
    // Extract IP and PORT from server_ip_port
    std::string ip = server_ip_port.substr(0, server_ip_port.find(':'));
    int port = std::stoi(server_ip_port.substr(server_ip_port.find(':') + 1));

    // Create socket connection to tracker
    int client_socket;
    struct sockaddr_in serv_addr;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/Address not supported" << std::endl;
        return -1;
    }

    if (connect(client_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        return -1;
    }

    std::cout << "Connected to tracker at " << ip << ":" << port << std::endl;

    // Function to handle commands and communication with tracker
    go_to_tracker(client_socket);

    close(client_socket);
    return 0;
}
