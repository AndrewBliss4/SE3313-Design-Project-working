#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <signal.h>
#include <mutex>
#define MAX_LEN 200

using namespace std;

bool exit_flag = false;
thread t_send, t_recv;
int client_socket;
string text_colour = "\033[037m";

void catch_ctrl_c(int signal);
int eraseText(int cnt);
void send_message(int client_socket);
void recv_message(int client_socket);

int main()
{
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket: ");
        exit(-1);
    }

    struct sockaddr_in client;
    client.sin_family = AF_INET;
    client.sin_port = htons(10000); // Port no. of server
    client.sin_addr.s_addr = INADDR_ANY;
    //client.sin_addr.s_addr=inet_addr("127.0.0.1"); // Provide IP address of server
    bzero(&client.sin_zero, 0);

    if ((connect(client_socket, (struct sockaddr *)&client, sizeof(struct sockaddr_in))) == -1)
    {
        perror("connect: ");
        exit(-1);
    }
    signal(SIGINT, catch_ctrl_c);
    char name[MAX_LEN];
    cout << "Enter your name : ";
    cin.getline(name, MAX_LEN);
    send(client_socket, name, sizeof(name), 0);

    char roomName[MAX_LEN];
    recv(client_socket, roomName, sizeof(roomName), 0);

    cout << "\n\t  ====== Welcome to " << roomName << " ======   " << endl
         << text_colour;

    thread t1(send_message, client_socket);
    thread t2(recv_message, client_socket);

    t_send = move(t1);
    t_recv = move(t2);

    if (t_send.joinable())
        t_send.join();
    if (t_recv.joinable())
        t_recv.join();

    return 0;
}

// Handler for "Ctrl + C"
void catch_ctrl_c(int signal)
{
    char str[MAX_LEN] = "#exit";
    send(client_socket, str, sizeof(str), 0);
    exit_flag = true;
    t_send.detach();
    t_recv.detach();
    close(client_socket);
    exit(signal);
}

// Erase text from terminal
int eraseText(int cnt)
{
    char back_space = 8;
    for (int i = 0; i < cnt; i++)
    {
        cout << back_space;
    }
    return 1;
}

// Send message to everyone
void send_message(int client_socket)
{
    while (1)
    {
        cout << text_colour << "You : " << text_colour;
        char str[MAX_LEN];
        cin.getline(str, MAX_LEN);
        send(client_socket, str, sizeof(str), 0);
        if (strcmp(str, "#exit") == 0)
        {
            exit_flag = true;
            t_recv.detach();
            close(client_socket);
            return;
        }
    }
}

// Receive message
void recv_message(int client_socket)
{
    while (1)
    {
        if (exit_flag)
            return;
        char name[MAX_LEN], str[MAX_LEN];
        int colour_code;
        int bytes_received = recv(client_socket, name, sizeof(name), 0);
        if (bytes_received <= 0)
            continue;
        recv(client_socket, &colour_code, sizeof(colour_code), 0);
        recv(client_socket, str, sizeof(str), 0);
        eraseText(6);
        if (strcmp(name, "#NULL") != 0)
            cout << text_colour << name << " : " << text_colour << str << endl;
        else
            cout << text_colour << str << endl;
        cout << text_colour << "You : " << text_colour;
        fflush(stdout);
    }
}
