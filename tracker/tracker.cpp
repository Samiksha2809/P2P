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
using namespace std;

#define PORT 8000  // Hardcoded port number

map<string, string> users;          // user_id -> password
map<string, set<string>> groups;  // group_id -> members
map<string, set<string>> pending_requests;  // group_id -> pending user_ids

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
        string response = "User already exists!";
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

void createGroup(const string &group_id, const string &user_id, int client_socket) {
    if (groups.find(group_id) == groups.end()) {
        groups[group_id].insert(user_id);  // Add creator as a member
        string response = "Group created successfully: " + group_id;
        send(client_socket, response.c_str(), response.size(), 0);
    } else {
        string response = "Group already exists!";
        send(client_socket, response.c_str(), response.size(), 0);
    }
}

void joinGroup(const string &group_id, const string &user_id, int client_socket) {
    if (groups.find(group_id) != groups.end()) {
        pending_requests[group_id].insert(user_id);
        string response = "Join request sent for group: " + group_id;
        send(client_socket, response.c_str(), response.size(), 0);
    } else {
        string response = "Group not found!";
        send(client_socket, response.c_str(), response.size(), 0);
    }
}

void leaveGroup(const string &group_id, const string &user_id, int client_socket) {
    if (groups.find(group_id) != groups.end() && groups[group_id].find(user_id) != groups[group_id].end()) {
        groups[group_id].erase(user_id);
        string response = "Left group: " + group_id;
        send(client_socket, response.c_str(), response.size(), 0);
    } else {
        string response = "Group not found or you're not a member!";
        send(client_socket, response.c_str(), response.size(), 0);
    }
}

void listPendingRequests(const string &group_id, int client_socket) {
    if (pending_requests.find(group_id) != pending_requests.end()) {
        string response = "Pending requests for group " + group_id + ":\n";
        for (const auto &user_id : pending_requests[group_id]) {
            response += user_id + "\n";
        }
        send(client_socket, response.c_str(), response.size(), 0);
    } else {
        string response = "No pending requests or group not found!";
        send(client_socket, response.c_str(), response.size(), 0);
    }
}

void acceptRequest(const string &group_id, const string &user_id, int client_socket) {
    if (pending_requests.find(group_id) != pending_requests.end() && pending_requests[group_id].find(user_id) != pending_requests[group_id].end()) {
        groups[group_id].insert(user_id);  // Add user to group
        pending_requests[group_id].erase(user_id);  // Remove from pending
        string response = "Accepted join request for user: " + user_id + " in group: " + group_id;
        send(client_socket, response.c_str(), response.size(), 0);
    } else {
        string response = "Request not found!";
        send(client_socket, response.c_str(), response.size(), 0);
    }
}

void listGroups(int client_socket) {
    string response = "Groups in network:\n";
    for (const auto &group : groups) {
        response += group.first + "\n";
    }
    send(client_socket, response.c_str(), response.size(), 0);
}

void handleClientCommands(int client_socket) {
    char buffer[1024] = {0};
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(client_socket, buffer, 1024);
        if (valread == 0) {
            // Client disconnected
            cout << "Client disconnected" << endl;
            break;
        }

        string command(buffer);
        istringstream iss(command);
        string cmd, arg1, arg2;
        iss >> cmd >> arg1 >> arg2;

        if (cmd == "create_user") {
            createUser(arg1, arg2, client_socket);
        } else if (cmd == "login") {
            loginUser(arg1, arg2, client_socket);
        } else if (cmd == "create_group") {
            createGroup(arg1, arg2, client_socket);
        } else if (cmd == "join_group") {
            joinGroup(arg1, arg2, client_socket);
        } else if (cmd == "leave_group") {
            leaveGroup(arg1, arg2, client_socket);
        } else if (cmd == "list_requests") {
            listPendingRequests(arg1, client_socket);
        } else if (cmd == "accept_request") {
            acceptRequest(arg1, arg2, client_socket);
        } else if (cmd == "list_groups") {
            listGroups(client_socket);
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
        cerr << "Usage: ./tracker tracker_info.txt tracker_no" << endl;
        return 1;
    }

    string tracker_info = argv[1];
    int tracker_no = stoi(argv[2]);

    loadTrackerInfo(tracker_info);
    cout << "Tracker " << tracker_no << " is running..." << endl;

    // Socket setup
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        cerr << "Socket failed" << endl;
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        cerr << "Bind failed" << endl;
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        cerr << "Listen failed" << endl;
        exit(EXIT_FAILURE);
    }

    while (true) {
        cout << "Waiting for connections..." << endl;
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            cerr << "Accept failed" << endl;
            exit(EXIT_FAILURE);
        }
        cout << "Client connected" << endl;
        handleClientCommands(new_socket);
    }

    return 0;
}
