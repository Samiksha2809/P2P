#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>

// Global data structures to manage users, groups, and pending requests
std::map<std::string, std::string> users;          // user_id -> password
std::map<std::string, std::set<std::string>> groups;  // group_id -> members

// Function to load tracker information from the file
void loadTrackerInfo(const std::string &filename, int tracker_no, std::string &ip, int &port) {
    std::ifstream file(filename);
    std::string line;
    
    // Read the tracker info from the file and select the correct tracker based on tracker_no
    int current_tracker = 0;
    while (std::getline(file, line)) {
        if (current_tracker == tracker_no) {
            ip = line.substr(0, line.find(':'));
            port = std::stoi(line.substr(line.find(':') + 1));
            return;
        }
        current_tracker++;
    }
    std::cerr << "Tracker " << tracker_no << " not found in tracker_info.txt" << std::endl;
    exit(EXIT_FAILURE);
}

// Function to create a new user
void createUser(const std::string &user_id, const std::string &passwd, int client_socket) {
    if (users.find(user_id) == users.end()) {
        users[user_id] = passwd;
        std::string response = "User created successfully: " + user_id;
        send(client_socket, response.c_str(), response.size(), 0);
    } else {
        std::string response = "User already exists!";
        send(client_socket, response.c_str(), response.size(), 0);
    }
}

// Function to log in a user and store their session
void loginUser(const std::string &user_id, const std::string &passwd, int client_socket, std::string &current_user) {
    if (users.find(user_id) != users.end() && users[user_id] == passwd) {
        current_user = user_id;  // Store the logged-in user ID
        std::string response = "Login successful: " + user_id;
        send(client_socket, response.c_str(), response.size(), 0);
    } else {
        std::string response = "Invalid user_id or password!";
        send(client_socket, response.c_str(), response.size(), 0);
    }
}

// Function to create a new group
void createGroup(const std::string &group_id, const std::string &user_id, int client_socket) {
    if (groups.find(group_id) == groups.end()) {
        groups[group_id].insert(user_id);  // Add creator to the group
        std::string response = "Group created successfully: " + group_id;
        send(client_socket, response.c_str(), response.size(), 0);
    } else {
        std::string response = "Group already exists!";
        send(client_socket, response.c_str(), response.size(), 0);
    }
}

// Function to list all groups
void listGroups(int client_socket) {
    std::string response = "Groups in network:\n";
    for (const auto &group : groups) {
        response += group.first + "\n";
    }
    send(client_socket, response.c_str(), response.size(), 0);
}

// Function to join an existing group
void joinGroup(const std::string &group_id, const std::string &user_id, int client_socket) {
    if (groups.find(group_id) != groups.end()) {
        // Group exists, add the user to the group
        if (groups[group_id].find(user_id) == groups[group_id].end()) {
            groups[group_id].insert(user_id);
            std::string response = "User " + user_id + " joined the group: " + group_id;
            send(client_socket, response.c_str(), response.size(), 0);
        } else {
            std::string response = "User " + user_id + " is already a member of the group: " + group_id;
            send(client_socket, response.c_str(), response.size(), 0);
        }
    } else {
        std::string response = "Group does not exist!";
        send(client_socket, response.c_str(), response.size(), 0);
    }
}

// Function to leave a group
void leaveGroup(const std::string &group_id, const std::string &user_id, int client_socket) {
    if (groups.find(group_id) != groups.end()) {
        // Group exists, check if the user is a member
        if (groups[group_id].find(user_id) != groups[group_id].end()) {
            groups[group_id].erase(user_id);  // Remove user from the group

            // If the group becomes empty, remove it
            if (groups[group_id].empty()) {
                groups.erase(group_id);
            }

            std::string response = "User " + user_id + " left the group: " + group_id;
            send(client_socket, response.c_str(), response.size(), 0);
        } else {
            std::string response = "User " + user_id + " is not a member of the group: " + group_id;
            send(client_socket, response.c_str(), response.size(), 0);
        }
    } else {
        std::string response = "Group does not exist!";
        send(client_socket, response.c_str(), response.size(), 0);
    }
}

// Function to handle incoming client commands in a separate thread
void handleClientCommands(int client_socket) {
    char buffer[1024] = {0};
    std::string current_user;  // Store the current logged-in user for the session
    
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(client_socket, buffer, 1024);  // Read the command from the client
        if (valread == 0) {
            std::cout << "Client disconnected" << std::endl;
            break;
        }

        std::string command(buffer);
        std::istringstream iss(command);
        std::string cmd, arg1, arg2;
        iss >> cmd >> arg1 >> arg2;

        std::cout << "Received command: " << cmd << " with arguments: " << arg1 << " " << arg2 << std::endl;

        if (cmd == "create_user") {
            createUser(arg1, arg2, client_socket);
        } else if (cmd == "login") {
            loginUser(arg1, arg2, client_socket, current_user);
        } else if (cmd == "create_group") {
            if (!current_user.empty()) {
                createGroup(arg1, current_user, client_socket);
            } else {
                std::string response = "Please login first!";
                send(client_socket, response.c_str(), response.size(), 0);
            }
        } else if (cmd == "join_group") {
            if (!current_user.empty()) {
                joinGroup(arg1, current_user, client_socket);
            } else {
                std::string response = "Please login first!";
                send(client_socket, response.c_str(), response.size(), 0);
            }
        } else if (cmd == "leave_group") {
            if (!current_user.empty()) {
                leaveGroup(arg1, current_user, client_socket);
            } else {
                std::string response = "Please login first!";
                send(client_socket, response.c_str(), response.size(), 0);
            }
        } else if (cmd == "list_groups") {
            listGroups(client_socket);
        } else if (cmd == "quit") {
            std::string response = "Tracker shutting down...";
            send(client_socket, response.c_str(), response.size(), 0);
            close(client_socket);
            break;
        } else {
            std::string response = "Unknown command: " + cmd;
            send(client_socket, response.c_str(), response.size(), 0);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: ./tracker tracker_info.txt tracker_no" << std::endl;
        return 1;
    }

    std::string tracker_info = argv[1];
    int tracker_no = std::stoi(argv[2]);

    // Load the tracker's IP and port from the tracker_info.txt file
    std::string ip;
    int port;
    loadTrackerInfo(tracker_info, tracker_no, ip, port);

    // Setup the socket for the tracker
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        std::cerr << "Socket creation failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        std::cerr << "Listen failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Accept and handle client connections
    while (true) {
        std::cout << "Waiting for connections on port " << port << "..." << std::endl;
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            std::cerr << "Accept failed" << std::endl;
            exit(EXIT_FAILURE);
        }
        std::cout << "Client connected" << std::endl;
        
        std::thread client_thread(handleClientCommands, new_socket);
        client_thread.detach();  // Detach the thread to handle clients independently
    }

    return 0;
}
