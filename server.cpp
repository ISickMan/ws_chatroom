#include <winsock2.h>  // Windows Socket API V2
#include <ws2tcpip.h>  // inet_ntop, inet_pton

#include <algorithm>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

void InitWinSock()
{
    WSADATA wsaData;
    int err = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (err != 0)
    {
        std::cout << "Failed to initialize winsock, error code: " << err;
        exit(EXIT_FAILURE);
    }

    std::cout << "Winsock2 initialized successfully." << std::endl;
}

void InitServerSocket(SOCKET *sockfd)
{
    // create socket file descriptor (identifier)
    *sockfd =
        socket(AF_INET,      // adress family: ipv4
               SOCK_STREAM,  // socket type: socket stream, reliable byte
                             // connection-based byte stream
               IPPROTO_TCP   // protocol: transmission control protocol (tcp)
        );

    if (*sockfd == INVALID_SOCKET)
    {
        std::cout << "Failed to initialize socket, error code: "
                  << WSAGetLastError();
        exit(EXIT_FAILURE);
    }

    std::cout << "Socket initialized successfully." << std::endl;
}

void SetServerSockAddr(sockaddr_in *sockaddr, u_long address, u_short port)
{
    // internet socket address structure
    sockaddr->sin_family = AF_INET;  // ipv4
    sockaddr->sin_addr.s_addr =
        address;  // no htonl required as it is already performed in inet_pton()
    sockaddr->sin_port = htons(port);  // host to network short
}

void BindServerSock(SOCKET sockfd, sockaddr_in *server_addr)
{
    if (bind(sockfd, (struct sockaddr *)server_addr, sizeof(*server_addr)) != 0)
    {
        std::cout << "Failed to bind server socket to the specified address, "
                     "error code: "
                  << WSAGetLastError();
        exit(EXIT_FAILURE);
    }

    std::cout << "Socket successfully binded to the specified address."
              << std::endl;
}

SOCKET AcceptSocket(SOCKET server_sock)
{
    SOCKET connecting_sock;
    sockaddr_in sock_addr;
    int addr_len;

    connecting_sock =
        accept(server_sock, (struct sockaddr *)&sock_addr, &addr_len);

    if (connecting_sock == INVALID_SOCKET)
    {
        std::cout << "Failed to accept incoming connection, error code: "
                  << WSAGetLastError();
        exit(EXIT_FAILURE);
    }

    // convert the ip address from network byte order to text
    char ipv4[INET_ADDRSTRLEN];
    inet_ntop(
        AF_INET, (void *)&sock_addr.sin_addr.s_addr, ipv4,
        sizeof(ipv4));  // inet = internet, n = network, p = (text) presentation

    std::cout << "Successfully accepted incoming connection from: " << ipv4
              << std::endl;

    return connecting_sock;
}

SOCKET AcceptNewConnection(SOCKET server_sock, int maxQueueLen)
{
    if (listen(server_sock, maxQueueLen) != 0)
    {
        std::cout << "Failed to listen for new connections, error code: "
                  << WSAGetLastError();
        exit(EXIT_FAILURE);
    }

    return AcceptSocket(server_sock);
}

void HandleClient(SOCKET client_sock, std::vector<SOCKET> *connected_clients)
{
    const int buff_len = 1024;
    char buff[buff_len];

    strcpy(buff, "Hello from server!");

    send(client_sock, buff, buff_len, 0);

    std::cout << "Sent hello message to connected client." << std::endl;

    // Wait for messages, once received, forward them to all the other clients
    while (true)
    {
        int res = recv(client_sock, buff, buff_len, 0);

        // in case of an error:
        if (res <= 0)
        {
            // remove the cononected client from the vector
            std::remove(connected_clients->begin(), connected_clients->end(),
                        client_sock);

            std::cout << "Connection to client was closed." << std::endl;

            // and "kill" its thread
            break;
        }

        std::cout << "Received message from client: " << buff << std::endl;

        int clients_count = connected_clients->size();

        for (int i = 0; i < clients_count; i++)
        {
            if ((*connected_clients)[i] != client_sock)
            {
                send((*connected_clients)[i], buff, buff_len, 0);
            }
        }

        if (clients_count > 1)
        {
            std::cout << "Forwarded message to other clients." << std::endl;
        }
    }
}

int main()
{
    std::cout << "--------------------------" << std::endl;
    std::cout << "Server / TCP Chatroom Demo" << std::endl;
    std::cout << "--------------------------" << std::endl;

    bool is_exiting;

    SOCKET server_sock;
    std::vector<SOCKET> connected_sockets;
    sockaddr_in server_addr;

    std::string string_ip;

    u_long server_ip;
    u_short server_port;

    do
    {
        std::cout << "Enter the server's ip: ";
        std::cin >> string_ip;

        inet_pton(AF_INET, string_ip.c_str(),
                  &server_ip);  // p = (text) presentation, n = network
    } while (server_ip == INADDR_NONE);

    std::cout << "Enter the server's port: ";
    std::cin >> server_port;

    InitWinSock();

    InitServerSocket(&server_sock);

    SetServerSockAddr(&server_addr, server_ip, server_port);

    BindServerSock(server_sock, &server_addr);

    std::cout << "--------------------------" << std::endl;
    std::cout << "Listening for new connections..." << std::endl;

    while (!is_exiting)
    {
        SOCKET new_sock = AcceptNewConnection(server_sock, 5);
        connected_sockets.insert(connected_sockets.begin(), new_sock);

        std::thread client_thread(HandleClient, new_sock, &connected_sockets);
        client_thread.detach();
    }

    std::cout << "Exitting..." << std::endl;

    closesocket(server_sock);

    for (int i = 0; i < connected_sockets.size(); i++)
    {
        closesocket(connected_sockets[i]);
    }

    WSACleanup();
}