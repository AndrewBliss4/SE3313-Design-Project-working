#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <vector>

#define MAX_LEN 200

using namespace std;

struct terminal
{
    int id;
    string name;
    int socket;
    thread th;
};

struct chatroom
{
    int id;
    string name;
    vector<int> members;
};

vector<terminal> clients;
vector<chatroom> chatrooms;
vector<vector<pair<int, string>>> chatroom_messages; // Declare chatroom_messages vector
string text_colour = "\033[37m";
int seed = 0;
mutex cout_mtx, clients_mtx, chatrooms_mtx;

void set_name(int id, char name[]);
void shared_print(string str, bool endLine);
int broadcast_message(string message, int sender_id);
int broadcast_message(int num, int sender_id);
void end_connection(int id);
void handle_client(int client_socket, int id);
char roomName[MAX_LEN];

int main()
{
    int server_socket;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket: ");
        exit(-1);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(10000);
    server.sin_addr.s_addr = INADDR_ANY;
    bzero(&server.sin_zero, 0);

    if ((bind(server_socket, (struct sockaddr *)&server, sizeof(struct sockaddr_in))) == -1)
    {
        perror("bind error: ");
        exit(-1);
    }

    if ((listen(server_socket, 8)) == -1)
    {
        perror("listen error: ");
        exit(-1);
    }

    struct sockaddr_in client;
    int client_socket;
    unsigned int len = sizeof(sockaddr_in);

    cout << "Enter Chat-room name: " << endl;
    cin.getline(roomName, MAX_LEN);
    cout << "\n\t  ====== Welcome to " << roomName << " ======   " << endl
         << text_colour;

    while (1)
    {
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client, &len)) == -1)
        {
            perror("accept error: ");
            exit(-1);
        }
        seed++;
        thread t(handle_client, client_socket, seed);
        lock_guard<mutex> guard(clients_mtx);
        clients.push_back({seed, string("Anonymous"), client_socket, move(t)});
    }

    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i].th.joinable())
            clients[i].th.join();
    }

    close(server_socket);
    return 0;
}

// Set name of client
void set_name(int id, char name[])
{
    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i].id == id)
        {
            clients[i].name = string(name);
        }
    }
}

// For synchronization of cout statements
void shared_print(string str, bool endLine)
{
    lock_guard<mutex> guard(cout_mtx);
    cout << str;
    if (endLine)
        cout << endl;
}

// Broadcast message to all clients except the sender
int broadcast_message(string message, int sender_id)
{
    char temp[MAX_LEN];
    strcpy(temp, message.c_str());
    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i].id != sender_id)
        {
            send(clients[i].socket, temp, sizeof(temp), 0);
        }
    }
    return 1;
}

// Broadcast a number to all clients except the sender
int broadcast_message(int num, int sender_id)
{
    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i].id != sender_id)
        {
            send(clients[i].socket, &num, sizeof(num), 0);
        }
    }
    return 1;
}

void end_connection(int id)
{
    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i].id == id)
        {
            lock_guard<mutex> guard(clients_mtx);
            clients[i].th.detach();
            clients.erase(clients.begin() + i);
            close(clients[i].socket);
            break;
        }
    }
}

// Helper function to send a message to all members of a chatroom except the sender
void send_message_to_chatroom_members(int room_id, const string& message, int sender_id)
{
    char temp[MAX_LEN];
    strcpy(temp, message.c_str());
    for (int member_id : chatrooms[room_id - 1].members)
    {
        if (member_id != sender_id)
        {
            send(clients[member_id - 1].socket, temp, sizeof(temp), 0);
        }
    }
}

void handle_client(int client_socket, int id)
{
    char name[MAX_LEN], str[MAX_LEN];
    recv(client_socket, name, sizeof(name), 0);
    set_name(id, name);

    // Display welcome message
    string welcome_message = string(name) + string(" has joined");
    send(client_socket, roomName, sizeof(roomName), 0);
    //broadcast_message("#NULL", id);
    //broadcast_message(id, id);
    //broadcast_message(welcome_message, id);
    shared_print(text_colour + welcome_message + text_colour, true);

    int room_id;
    
    while (1)
    {
        {
        lock_guard<mutex> guard(chatrooms_mtx);
        if (chatrooms.empty() || chatrooms.back().members.size() == 2) {
            room_id = chatrooms.size() + 1;
            chatrooms.push_back({room_id, string("Chatroom ") + to_string(room_id), {id}});
            // Send a message to the new chatroom here and print on the server side
            string server_welcome_message = string(name) + " has joined Chatroom " + to_string(room_id);
            shared_print(text_colour + server_welcome_message + text_colour, true);
        } else {
            room_id = chatrooms.back().id;
            chatrooms.back().members.push_back(id);
            // Print on the server side
            string server_welcome_message = string(name) + " has joined Chatroom " + to_string(room_id);
            shared_print(text_colour + server_welcome_message + text_colour, true);
        }
        }

        while (chatrooms[room_id - 1].members.size() < 2)
        {
            // Wait for another client to join this chatroom
            this_thread::sleep_for(chrono::milliseconds(100));
        }

        // Now this chatroom is full with two clients, start handling messages
        while (1)
        {
            int bytes_received = recv(client_socket, str, sizeof(str), 0);
            if (bytes_received <= 0)
                return;
            if (strcmp(str, "#exit") == 0)
            {
                // Display leaving message
                string message = string(name) + string(" has left");
                broadcast_message("#NULL", id);
                broadcast_message(id, id);
                broadcast_message(message, id);
                shared_print(text_colour + message + text_colour, true);
                end_connection(id);
                return;
            }

            // Broadcast the message only to clients within the same chatroom
            for (int member_id : chatrooms[room_id - 1].members)
            {
                if (member_id != id)
                {
                    send(clients[member_id - 1].socket, name, sizeof(name), 0);
                    send(clients[member_id - 1].socket, &id, sizeof(id), 0);
                    send(clients[member_id - 1].socket, str, sizeof(str), 0);
                    shared_print(text_colour + name + " : " + text_colour + str, true);
                }
            }
        }
    }
}
