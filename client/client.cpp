#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

// Function to load tracker info from the file
void loadTrackerInfo(const std::string &filename, int tracker_no, std::string &ip, int &port) {
    std::ifstream file(filename);
    std::string line;
    int current_tracker = 0;

    // Read the tracker info from the file and select the correct tracker based on tracker_no
    while (std::getline(file, line)) {
        if (current_tracker == tracker_no) {
            ip = line.substr(0, line.find(':'));
            port = std::stoi(line.substr(line.find(':') + 1));
            return;
        }
        current_tracker++;
    }

    // If the tracker_no is not found, print an error and exit
    std::cerr << "Tracker " << tracker_no << " not found in " << filename << std::endl;
    exit(EXIT_FAILURE);
}

// Function to send commands to the tracker and receive responses
void go_to_tracker(int client_socket) {
    std::string command;

    while (true) {
        std::cout << "Enter command: ";
        std::getline(std::cin, command);

        // If the user enters the "quit" command, break the loop
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

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: ./client <IP>:<PORT> tracker_info.txt" << std::endl;
        return 1;
    }

    std::string client_ip_port = argv[1];
    std::string tracker_info = argv[2];

    // Extract client IP and PORT using string manipulation
    std::string client_ip;
    int client_port = 0;

    size_t colon_pos = client_ip_port.find(':');
    if (colon_pos == std::string::npos) {
        std::cerr << "Invalid client IP and port format. Use <IP>:<PORT>" << std::endl;
        return 1;
    }

    client_ip = client_ip_port.substr(0, colon_pos);
    try {
        client_port = std::stoi(client_ip_port.substr(colon_pos + 1));
    } catch (const std::invalid_argument &e) {
        std::cerr << "Invalid port format. Port must be an integer." << std::endl;
        return 1;
    }

    // Retrieve tracker's IP and PORT from tracker_info.txt based on tracker_no
    std::string tracker_ip;
    int tracker_port;
    int tracker_no = 0;  // You can modify this to connect to different trackers by changing the tracker_no

    loadTrackerInfo(tracker_info, tracker_no, tracker_ip, tracker_port);

    std::cout << "Client attempting to connect to tracker at " << tracker_ip << ":" << tracker_port << std::endl;

    // Create socket for the client
    int client_socket;
    struct sockaddr_in tracker_addr;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    tracker_addr.sin_family = AF_INET;
    tracker_addr.sin_port = htons(tracker_port);

    if (inet_pton(AF_INET, tracker_ip.c_str(), &tracker_addr.sin_addr) <= 0) {
        std::cerr << "Invalid tracker address" << std::endl;
        return -1;
    }

    std::cout << "Connecting to tracker at " << tracker_ip << ":" << tracker_port << std::endl;

    // Connect to the tracker
    if (connect(client_socket, (struct sockaddr *)&tracker_addr, sizeof(tracker_addr)) < 0) {
        std::cerr << "Connection to tracker failed" << std::endl;
        return -1;
    }

    std::cout << "Connected to tracker at " << tracker_ip << ":" << tracker_port << std::endl;

    // Function to handle commands and communication with tracker
    go_to_tracker(client_socket);

    close(client_socket);
    return 0;
}







