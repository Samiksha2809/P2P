#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
using namespace std;

#define PORT 8000  

void loadTrackerInfo(const string &filename) {
    ifstream file(filename);
    string line;
    while (getline(file, line)) {
        cout << "Tracker info: " << line << endl;
    }
}

void take_command(int client_socket) {
    string command;
    while (true) {
        cout << "Enter command: ";
        getline(cin, command);
        
        if (command == "quit") {
            send(client_socket, command.c_str(), command.size(), 0);
            cout << "Client shutting down..." << endl;
            break;
        }

        // Send command to tracker
        send(client_socket, command.c_str(), command.size(), 0);

        // Receive response from tracker
        char buffer[1024] = {0};
        int valread = read(client_socket, buffer, 1024);
        cout << buffer << endl;
    }
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        cout << "Give Command as ./client <IP>:<PORT> tracker_info.txt" << endl;
        return 1;
    }

    string server_ip_port = argv[1];
    string tracker_info = argv[2];

    //loadTrackerInfo(tracker_info);
    
    // Extract IP and PORT from server_ip_port
    string ip = server_ip_port.substr(0, server_ip_port.find(':'));
    int port = stoi(server_ip_port.substr(server_ip_port.find(':') + 1));

    // Create socket connection to tracker
    int client_socket;
    struct sockaddr_in serv_addr;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        cout << "Socket creation error" << endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {
        cerr << "Invalid address/Address not supported" << endl;
        return -1;
    }

    if (connect(client_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cout << "Connection failed" << endl;
        return -1;
    }

    cout << "Connected to tracker at " << ip << ":" << port << endl;

    // Function to handle commands and communication with tracker
    take_command(client_socket);

    close(client_socket);
    return 0;
}
