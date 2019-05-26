#include<bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <sys/epoll.h>

using namespace std;

int main(int argc, char* argv[])
{
    if(argc == 2)
    {
        if(!strcmp(argv[1], "-h") || !strcmp(argv[1], "-help"))
        {
            cout << "Try: server [one of the following]" << endl;
            cout << "\t any [PORT]" << endl;
            cout << "\t\t opens socket on port equals PORT" << endl;
            cout << "\t [IP] [PORT]" << endl;
            cout << "\t\t opens socket on ip equals IP and port equals PORT" << endl;
            cout << "While server is online you can try COMMAND as client:" << endl;
            cout << "\tCOMMAND = exit$. Closes server and client" << endl;
            return 0;
        }
        return -1;
    }
    if(argc != 3)
    {
        cerr << "Error: illegal amount of arguments" << endl;
        cout << "Try: server [-help | -h]" << endl;
        return -1;
    }

    const char* ip = argv[1];
    const char* port = argv[2];

    int sock, listener;
    struct sockaddr_in addr;
    char buf[1024];
    int bytes_read;

    listener = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if(listener == -1)
    {
        cerr << strerror (errno) <<endl;
        exit(1);
    }

    int yes = 1;

    if(setsockopt (listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (yes)) == -1)
    {
        cerr << strerror (errno) <<endl;
        if(close(listener) == -1)
        {
            cerr << strerror (errno) <<endl;
        }
        return 1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(stol(port));

    if(!strcmp(ip, "any"))
    {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else
    {
        addr.sin_addr.s_addr = inet_addr(ip);
    }

    if(bind(listener, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        cerr << strerror (errno) <<endl;
        if(close(listener) == -1)
        {
            cerr << strerror (errno) <<endl;
        }
        exit(2);
    }

    if(listen(listener, 5) == -1)
    {
        cerr << strerror (errno) << endl;
        if(close(listener) == -1)
        {
            cerr << strerror (errno) <<endl;
        }
        return 1;
    }

    const int max_event = 256;
    struct epoll_event ev, events[max_event];

    int epollfd;

    epollfd = epoll_create1(0);

    if (epollfd == -1)
    {
        cerr << strerror (errno) <<endl;
        if(close(listener) == -1)
        {
            cerr << strerror (errno) <<endl;
        }
        exit(-1);
    }

    ev.events = EPOLLIN;
    ev.data.fd = listener;

    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, listener, &ev) == -1)
    {
        cerr << strerror (errno) <<endl;
        if(close(listener) == -1)
        {
            cerr << strerror (errno) <<endl;
        }
        if(close(epollfd) == -1)
        {
            cerr << strerror (errno) <<endl;
        }
        exit(-1);
    }

    while(1)
    {
        int wait = epoll_wait(epollfd, events, max_event, -1);
        for(int i = 0; i < wait; i++)
        {
            if(events[i].data.fd == listener)
            {
                if ((EPOLLIN & events[i].events) != 0)
                {
                    socklen_t size = sizeof(addr);
                    sock = accept(listener, (struct sockaddr *)&addr, &size);
                    if(sock == -1)
                    {
                        cerr << strerror (errno) <<endl;
                        continue;
                    }

                    ev.events = EPOLLIN | EPOLLRDHUP;
                    ev.data.fd = sock;
                    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, &ev) == -1)
                    {
                        cerr << strerror (errno) <<endl;
                        if(close(sock) == -1)
                        {
                            cerr << strerror (errno) <<endl;
                        }
                        continue;
                    }

                    ev.events = 0;
                    ev.data.fd = listener;
                    if(epoll_ctl(epollfd, EPOLL_CTL_DEL, listener, &ev) == -1)
                    {
                        cerr << strerror (errno) <<endl;
                        if(close(sock) == -1)
                        {
                            cerr << strerror (errno) <<endl;
                        }
                        if(close(listener) == -1)
                        {
                            cerr << strerror (errno) <<endl;
                        }
                        if(close(epollfd) == -1)
                        {
                            cerr << strerror (errno) <<endl;
                        }
                        exit(-1);
                    }
                }
            }
            else
            {
                if(events[i].data.fd == sock)
                {
                    if((events[i].events & EPOLLIN) > 0)
                    {
                        //bytes_read = recv(sock, buf, 1024, 0);

                        char buf[1024];
                        int bytes_read;
                        string received_string = "";
                        do
                        {
                            bytes_read = recv(sock, buf, 1024, 0);
                            if(bytes_read <= 0)
                            {
                                //socket closed
                                //cout<<"socket closed"<<endl;
                                break;
                            }
                            received_string += string(buf).substr(0, bytes_read);
                        } while(buf[bytes_read - 1] != -1);
                        //*******
                        if(bytes_read <= 0)
                        {
                            ev.events = EPOLLIN;
                            ev.data.fd = listener;
                            if(epoll_ctl(epollfd, EPOLL_CTL_ADD, listener, &ev) == -1)
                            {
                                cerr << strerror (errno) <<endl;
                                if(close(sock) == -1)
                                {
                                    cerr << strerror (errno) <<endl;
                                }
                                if(close(listener) == -1)
                                {
                                    cerr << strerror (errno) <<endl;
                                }
                                if(close(epollfd) == -1)
                                {
                                    cerr << strerror (errno) <<endl;
                                }
                                exit(-1);
                            }
                            ev.events = 0;
                            ev.data.fd = sock;
                            if(epoll_ctl(epollfd, EPOLL_CTL_DEL, sock, &ev) == -1)
                            {
                                cerr << strerror (errno) <<endl;
                                if(close(sock) == -1)
                                {
                                    cerr << strerror (errno) <<endl;
                                }
                                continue;
                            }
                            if(close(sock) == -1)
                            {
                                cerr << strerror (errno) <<endl;
                            }
                            continue;
                        }
                        received_string = received_string.substr(0, received_string.size() - 1);
                        if(received_string == "exit$")
                        {
                            cout << "FINISHING" << endl;
                            int msg = send(sock, (const char*)NULL, 0, 0);
                            if (msg < 0)
                            {
                                cerr << strerror (errno) <<endl;
                            }
                            if(close(sock) == -1)
                            {
                                cerr << strerror (errno) <<endl;
                            }
                            if(close(listener) == -1)
                            {
                                cerr << strerror (errno) <<endl;
                                exit(-1);
                            }
                            if(close(epollfd) == -1)
                            {
                                cerr << strerror (errno) <<endl;
                                exit(-1);
                            }
                            return 0;
                        }
                        for(int i = 0; i < received_string.size(); i++)
                        {
                            cout<<received_string[i];
                        }
                        cout<<endl;
                        received_string += -1;
                        int sent = 0;
                        do
                        {
                            string message = received_string.substr(sent, received_string.size());
                            int bytes_sent = send(sock, message.c_str(), message.size(), 0);
                            if(bytes_sent <= 0)
                            {
                                cerr<<"Error while sending"<<endl;
                                break;
                            }
                            sent += bytes_sent;
                        } while (sent < received_string.size());
                    }

                    if((events[i].events & EPOLLRDHUP) != 0)
                    {
                        ev.events = EPOLLIN;
                        ev.data.fd = listener;
                        if(epoll_ctl(epollfd, EPOLL_CTL_ADD, listener, &ev) == -1)
                        {
                            cerr << strerror (errno) <<endl;
                            exit(-1);
                        }
                        ev.events = 0;
                        ev.data.fd = sock;
                        if(epoll_ctl(epollfd, EPOLL_CTL_DEL, sock, &ev) == -1)
                        {
                            cerr << strerror (errno) <<endl;
                        }
                        if(close(sock) == -1)
                        {
                            cerr << strerror (errno) <<endl;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

