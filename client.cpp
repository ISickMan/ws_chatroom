#include <winsock2.h>  // Windows Socket API V2
#include <ws2tcpip.h>  // inet_ntop, inet_pton

#include <iostream>
#include <string>
#include <thread>

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

void InitSocket(SOCKET *sockfd)
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

void ConnectToServer(SOCKET sockfd, sockaddr_in *server_addr)
{
    if (connect(sockfd, (struct sockaddr *)server_addr, sizeof(*server_addr)) !=
        0)
    {
        std::cout << "Failed to connect to server, error code: "
                  << WSAGetLastError();
        exit(EXIT_FAILURE);
    }

    std::cout << "Successfully connected to server." << std::endl;
}

void Exit(SOCKET sockfd)
{
    std::cout << "Exitting..." << std::endl;

    closesocket(sockfd);

    WSACleanup();

    exit(EXIT_SUCCESS);
}

void SendConsoleInput(SOCKET sockfd)
{
    const int buff_len = 1024;
    char buff[buff_len];

    std::string message;

    while (true)
    {
        // include white spaces
        std::getline(std::cin, message);

        if (message == "exit")
        {
            Exit(sockfd);
            return;
        }
        else if (!message.empty())
        {
            //send the message to the server unless empty
            strcpy(buff, message.c_str());

            send(sockfd, buff, buff_len, 0);
        }
    }
}

int main()
{
    std::cout << "--------------------------" << std::endl;
    std::cout << "Client / TCP Chatroom Demo " << std::endl;
    std::cout << "--------------------------" << std::endl;

    SOCKET sockfd;
    sockaddr_in server_addr;

    std::string string_ip;

    u_long server_ip;
    u_short server_port;

    bool is_exiting;

    const int buff_len = 1024;
    char buff[buff_len];

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

    InitSocket(&sockfd);

    SetServerSockAddr(&server_addr, server_ip, server_port);

    ConnectToServer(sockfd, &server_addr);

    std::cout << "--------------------------" << std::endl;
    std::cout << "Type \"exit\" to exit chatroom." << std::endl;
    std::cout << "--------------------------" << std::endl;

    std::thread send_input(SendConsoleInput, sockfd);
    send_input.detach();

    while (!is_exiting)
    {
        int bytes_received;

        bytes_received = recv(sockfd, buff, buff_len, 0);

        if (bytes_received <= 0)
        {
            std::cout << "Connection to server was closed." << std::endl;
            is_exiting = true;
            break;
        }

        std::cout << buff << std::endl;
    }

    Exit(sockfd);
}