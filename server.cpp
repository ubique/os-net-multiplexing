#include <iostream>
#include <string>
#include <cstdlib>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <cstring>
#include <unistd.h>
#include <sys/epoll.h>

using std::cin, std::cout, std::endl, std::cerr;

const unsigned int BACKLOG = 5, BUFFER_SIZE = 1024, DEFAULT_PORT = 8080, MAX_EVENTS = 64;

void print_usage_args_and_exit() {
    cerr << "Wrong arguments: \"<host>:<port>\" expected, port from 1 to 65535" << endl;
    cerr << "Example: \"127.0.0.1:8080\"" << endl;
    cerr << "NB: adress 255.255.255.255 isn't valid" << endl;
    exit(0);
}

void trying_again_and_log_cerr(std::string err, int try_cnt) {
    cerr << err << " failure. " << std::strerror(errno) << endl;
    if (try_cnt == 2) {
        exit(0);
    }
    cerr << "trying again" << endl;
    sleep(3);
}

void closeAll(int epoll, int curSocket)
{
    for (int i = 0; i < 3; i++) {
        if (close(epoll) != -1) {
            break;
        }
        trying_again_and_log_cerr("Closing epoll", i);
    }
    for (int i = 0; i < 3; i++) {
        if (close(curSocket) != -1) {
            break;
        }
        trying_again_and_log_cerr("Closing socket", i);
    }
}

int main(int argc, char* argv[], char *envp[])
{
    //Args checking
    if (argc != 2) {
        print_usage_args_and_exit();
    }

    std::string a = argv[1],
                host = "",
                port = "";
    std::size_t found = a.find(":");

    if (found == std::string::npos) {
        print_usage_args_and_exit();
    }

    host = a.substr(0, found);
    port = a.substr(0, found + 1);
    if (inet_addr(host.c_str()) == INADDR_NONE || port == "") {
        print_usage_args_and_exit();
    }
    try {
        if (stoi(port) < 1 || stoi(port) > 65535) {
            print_usage_args_and_exit();
        }
    } catch (...) {
        print_usage_args_and_exit();
    }
    
    //Initialisation
    int curSocket;
    for (int i = 0; i < 3; i++) {
        curSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (curSocket != -1) {
            break;
        }
        trying_again_and_log_cerr("Socket initialisation", i);
    }

    int epoll = epoll_create1(0);
    if (epoll == -1) {
        cerr << "Epoll creating failere" << endl;
        for (int i = 0; i < 3; i++) {
            if (close(curSocket) != -1) {
                break;
            }
            trying_again_and_log_cerr("Closing socket", i);
        }
        exit(0);
    }

    struct epoll_event curClient, clients[MAX_EVENTS];

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(stoi(port));
    addr.sin_addr.s_addr = inet_addr(host.c_str());

    for (int i = 0; i < 3; i++) {
        if (bind(curSocket, (sockaddr*) &addr, sizeof(addr)) != -1) {
            break;
        }
        trying_again_and_log_cerr("Binding", i);
    }

    for (int i = 0; i < 3; i++) {
        if (listen(curSocket, BACKLOG) != -1) {
            break;
        }
        trying_again_and_log_cerr("Listening", i);
    }

    curClient.events = EPOLLIN;
    curClient.data.fd = curSocket;

    if (epoll_ctl(epoll, EPOLL_CTL_ADD, curSocket, &curClient) == -1) {
        cerr << "Adding to epol failure" << endl;
        closeAll(epoll, curSocket);
        exit(0);
    }

    //Run
    cout << "Server is running" << endl;
    while (true)
    {
        int clientsAmount = epoll_wait(epoll, clients, MAX_EVENTS, -1);

        if (clientsAmount == -1) {
            cerr << "Epol waiting failure" << endl;
            closeAll(epoll, curSocket);
            exit(0);
        }

        for (int i = 0; i < clientsAmount; i++) {
            if (clients[i].data.fd == curSocket) {

                struct sockaddr_in client;
                socklen_t len = sizeof(client);

                int clientSockd;
                for (int i = 0; i < 3; i++) {
                    clientSockd = accept(curSocket, (sockaddr*) &client, &len);
                    if (clientSockd != -1) {
                        break;
                    }
                    trying_again_and_log_cerr("Accepting", i);
                }

                cout << "Client \"" << clientSockd << "\" connected" << endl;


                curClient.events = EPOLLIN;
                curClient.data.fd = clientSockd;

                if (epoll_ctl(epoll, EPOLL_CTL_ADD, clientSockd, &curClient) == -1) {
                    cerr << "Adding to epol failure" << endl;
                    closeAll(epoll, curSocket);
                    exit(0);
                }

            } else {

                char request[BUFFER_SIZE] = {};
                int requestSize = recv(clients[i].data.fd, request, BUFFER_SIZE, 0);

                if (requestSize == -1) {

                    cerr << "Message delivery failure. " << std::strerror(errno) << endl;
                    
                    if (epoll_ctl(epoll, EPOLL_CTL_DEL, clients[i].data.fd, nullptr) == -1) {
                        cerr << "Delete from epoll failure" << endl;
                    }
                    if (close(clients[i].data.fd) == -1) {
                        cerr << "Socket closing failure" << endl;
                    }
                    break;
                } else if (requestSize == 0) {
                    break;
                }

                int responseSize = 0, tries = 5;

                while (responseSize < requestSize) {
                    int sended;
                    for (int i = 0; i < 3; i++) {
                        sended = send(clients[i].data.fd, request + responseSize, requestSize - responseSize, 0);
                        if (sended != -1) {
                            break;
                        }
                        trying_again_and_log_cerr("Local sending", i);
                    }
                    responseSize += sended;
                }
            }
        }
    }
    closeAll(epoll, curSocket);
}