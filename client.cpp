#include<bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <sys/epoll.h>

using namespace std;

bool is_equal(const char* a, const char* b,const int& len)
{
    for(int i = 0; i < len; i++)
    {
        if(a[i]!=b[i])
        {
            return 0;
        }
    }
    return 1;
}


int main(int argc, char * argv[]) {
    if (argc == 2) {
        if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "-help")) {
            cout << "Try: client [one of the following]" << endl;
            cout << "\tlocalhost [PORT]" << endl;
            cout << "\t\topens socket on port equals PORT" << endl;
            cout << "\t[IP] [PORT]" << endl;
            cout << "\t\topens socket on ip equals IP and port equals PORT" << endl;
            return 0;
        }
        return -1;
    }

    if (argc != 3) {
        cerr << "Error: illegal amount of arguments" << endl;
        cout << "Try: client [-help | -h]" << endl;
        return -1;
    }
    const char * ip = argv[1];
    const char * port = argv[2];

    int sock;
    struct sockaddr_in addr;

    sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if (sock < 0) {
        cerr << strerror (errno) <<endl;
        exit(-2);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(stol(port));

    if (!strcmp(ip, "localhost")) {
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    } else {
        addr.sin_addr.s_addr = inet_addr(ip);
    }

    queue < string > msgs;

    string s;
    cout<<"Enter your message. Or press ctrl + D for sending."<<endl;
    while (getline(cin, s)) {
        cout<<"Enter your message. Or press ctrl + D for sending."<<endl;
        msgs.push(s + char(-1));
    }

    bool connected = true;

    if (connect(sock, (struct sockaddr * ) & addr, sizeof(addr)) < 0 && errno == EINPROGRESS) {
        connected = false;
        cerr << strerror (errno) <<endl;
    }

    const int max_events = 256;
    struct epoll_event ev, events[max_events];

    int epollfd;

    epollfd = epoll_create1(0);

    if (epollfd == -1)
    {
        cerr << strerror (errno) <<endl;
        exit(-1);
    }

    ev.events = EPOLLOUT;
    ev.data.fd = sock;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, & ev) == -1) {
        cerr << strerror (errno) <<endl;
        if(close(epollfd) == -1)
        {
            cerr << strerror (errno) <<endl;
        }
        if(close(sock) == -1)
        {
            cerr << strerror (errno) <<endl;
        }
        return 1;
    }

    while (1) {
        int wait = epoll_wait(epollfd, events, 10, -1);
        if (wait == -1) {
            if(close(epollfd) == -1)
            {
                cerr << strerror (errno) <<endl;
            }
            if(close(sock) == -1)
            {
                cerr << strerror (errno) <<endl;
            }
            return 1;
        }

        for (int i = 0; i < wait; i++)
        {
            if (events[i].data.fd == sock)
            {
                if ((events[i].events & EPOLLIN) > 0) {
                    string recived_message = "";
                    char buf[1024];
                    int recvd_bytes = 0;
                    do
                    {
                        recvd_bytes = recv(sock, buf, 1024, 0);
                        if(recvd_bytes <= 0)
                        {
                            cout << "FINISHING" << endl;
                            if(msgs.front().substr(0, msgs.front().size()-1) != "exit$")
                            {
                                cerr << strerror (errno) <<endl;
                            }
                            if(close(sock) == -1)
                            {
                                cerr << strerror (errno) <<endl;
                            }
                            exit(-1);
                        }
                        recived_message += string(buf).substr(0, recvd_bytes);
                    } while(buf[recvd_bytes - 1] != -1);
                    recived_message = recived_message.substr(0, recived_message.size() - 1);
                    if (msgs.front().substr(0, msgs.front().size()-1) == recived_message)
                    {
                        cout << "Message sent." << endl;
                        msgs.pop();
                        epoll_event event;
                        event.events = EPOLLOUT;
                        event.data.fd = sock;
                        if (epoll_ctl(epollfd, EPOLL_CTL_MOD, sock, & event) == -1)
                        {
                            cerr << strerror(errno) << endl;
                            if(close(epollfd) == -1)
                            {
                                cerr << strerror (errno) <<endl;
                            }
                            if(close(sock) == -1)
                            {
                                cerr << strerror (errno) <<endl;
                            }
                            return 1;
                        }
                        if (msgs.size() == 0) {
                            cout<<"DONE"<<endl;
                            if(close(epollfd) == -1)
                            {
                                cerr << strerror (errno) <<endl;
                            }
                            if(close(sock) == -1)
                            {
                                cerr << strerror (errno) <<endl;
                            }
                            return 0;
                        }
                    }
                    else
                    {
                        cout<<"RESENDING"<<endl;
                    }
                }
                if ((events[i].events & EPOLLOUT) != 0)
                {
                    if (!connected)
                    {
                        int err = 0;
                        socklen_t len = sizeof(int);
                        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, & err, & len) != -1)
                        {
                            if (err == 0)
                            {
                                cout << "Connection established" << endl;
                            }
                            else
                            {
                                cerr << strerror(err) << endl;
                                epoll_event event;
                                event.events = 0;
                                event.data.fd = sock;
                                if(epoll_ctl(epollfd, EPOLL_CTL_DEL, sock, & event) == -1)
                                {
                                    cerr << strerror (errno) <<endl;
                                    exit(-1);
                                }
                                if(close(epollfd) == -1)
                                {
                                    cerr << strerror (errno) <<endl;
                                }
                                if(close(sock) == -1)
                                {
                                    cerr << strerror (errno) <<endl;
                                }
                            }
                        }
                        else
                        {
                            cerr << strerror(errno) << endl;
                            epoll_event event;
                            event.events = 0;
                            event.data.fd = sock;
                            if(epoll_ctl(epollfd, EPOLL_CTL_DEL, sock, & event))
                            {
                                cerr << strerror (errno) <<endl;
                                exit(-1);
                            }
                            if(close(epollfd) == -1)
                            {
                                cerr << strerror (errno) <<endl;
                            }
                            if(close(sock) == -1)
                            {
                                cerr << strerror (errno) <<endl;
                            }
                        }
                        connected = true;
                    }
                    else
                    {
                        cout << "Sending:\n" << msgs.front().substr(0, msgs.front().size()-1) << endl;
                        string message;
                        int sent = 0;
                        do
                        {
                            message = msgs.front().substr(sent, msgs.front().size());
                            int send_bytes = send(sock, message.c_str(), message.size(), 0);
                            if(send_bytes <= 0)
                            {
                                cerr << strerror (errno) <<endl;
                                if(close(sock) == -1)
                                {
                                    cerr << strerror (errno) <<endl;
                                }
                                exit(-1);
                            }
                            sent += send_bytes;
                        } while(sent < s.size());
                        epoll_event event;
                        event.events = EPOLLIN;
                        event.data.fd = sock;
                        if (epoll_ctl(epollfd, EPOLL_CTL_MOD, sock, & event) == -1) {
                            cerr << strerror(errno) << endl;
                            if(close(epollfd) == -1)
                            {
                                cerr << strerror (errno) <<endl;
                            }
                            if(close(sock) == -1)
                            {
                                cerr << strerror (errno) <<endl;
                            }
                            return 1;
                        }
                    }
                }
                if (events[i].events & EPOLLERR)
                {
                    epoll_event event;
                    event.events = 0;
                    event.data.fd = sock;
                    if(epoll_ctl(epollfd, EPOLL_CTL_DEL, sock, & event) == -1)
                    {
                        cerr << strerror (errno) <<endl;
                        exit(-1);
                    }
                    int err = 0;
                    socklen_t len = sizeof(int);
                    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, & err, & len) != -1)
                    {
                        cerr << "Connection failed: " << endl;
                        if(close(epollfd) == -1)
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
