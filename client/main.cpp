#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include "util/EventManager.h"
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <netdb.h>
#include "util/ConsoleHandler.h"


using namespace std;


in_addr_t hostnameToIp(string const &hostname) {
    const auto he = gethostbyname(hostname.c_str());
    if (he == nullptr) {
        return 0;
    }
    auto **addr_list = (in_addr **) he->h_addr_list;
    return addr_list[0]->s_addr;
}


class Client : public IHandler {
public:
    static const int BUFFER_SIZE = 1024;

    Client(const string &host, in_port_t port) {
        server_socket = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (server_socket == -1) {
            throw HandlerException("Cannot create socket.", errno);
        }
        sockaddr_in addr{};
        addr.sin_family = PF_INET;
        addr.sin_addr.s_addr = hostnameToIp(host);
        addr.sin_port = htons(port);
        if (connect(server_socket, (sockaddr *) &addr, sizeof(addr)) == -1) {
            if (errno != EINPROGRESS) {
                throw HandlerException("Cannot connect to server.", errno);
            }
        }
    }

    void sendMessage(string const &message) {
        if (send(server_socket, message.data(), message.size(), 0) == -1) {
            throw HandlerException("Cannot send message.", errno);
        }
        currentMessage = message;
    }

    void handleData(EventManager &eventManager) override {
        uint8_t buffer[BUFFER_SIZE];
        const int bytes = read(server_socket, buffer, BUFFER_SIZE);

        if (bytes == -1) {
            eventManager.deleteAll();
            throw HandlerException("Cannot read message.", errno);
        } else if (bytes == 0) {
            cout << "Server disconnected." << endl << endl;
            if (isWaiting()) {
                cout << "Answer was:\n"
                     << "    " << currentMessage.substr(0, bytesConfirmed) << endl;
            }
            eventManager.deleteAll();
        } else {
            string answer(buffer, buffer + bytes);
            bool sizeCorrect = bytesConfirmed + answer.size() <= currentMessage.size();
            bool answerCorrect = equal(answer.begin(), answer.end(), currentMessage.begin() + bytesConfirmed);
            if (!sizeCorrect || !answerCorrect) {
                string realAnswer = currentMessage.substr(0, bytesConfirmed) + answer;
                cout << "Unexpected server answer: " << endl
                     << "    " << realAnswer << endl << endl;
            } else if (bytesConfirmed + answer.size() == currentMessage.size()) {
                cout << "Server answer: " << endl
                     << "    " << currentMessage << endl << endl;
                currentMessage.clear();
                bytesConfirmed = 0;
            } else {
                bytesConfirmed += answer.size();
            }
        }
    }

    void handleError(EventManager &eventManager) override {
        const error_t error = getError(server_socket);
        eventManager.deleteAll();
        if (error != 0) {
            throw HandlerException("Client failed.", error);
        }
    }

    int getFD() override { return server_socket; }

    ~Client() override { close(server_socket); }

    bool isWaiting() { return !currentMessage.empty(); }

private:
    int server_socket;
    string currentMessage;
    int bytesConfirmed = 0;
};


class ClientConsole : public ConsoleHandler {
public:
    explicit ClientConsole(IHandler *client) {
        this->client = dynamic_cast<Client *>(client);
        if (this->client == nullptr) {
            throw HandlerException("Handler is not Client", 0);
        }
    }

    void handleData(EventManager &eventManager) override {
        std::string message;
        getline(cin, message);
        if (message == "exit") {
            eventManager.deleteAll();
        } else if (message.empty()) {
            cout << "Message is empty" << endl;
        } else if (client->isWaiting()) {
            cout << "Waiting answer for previous query" << endl;
        } else {
            cout << "Waiting..." << endl;
            client->sendMessage(message);
        }
    }

private:
    Client *client;
};


int main(int argc, char *argv[]) {
    try {
        const in_port_t port = stoi(argv[2]);
        shared_ptr<IHandler> client(new Client(argv[1], port));
        shared_ptr<IHandler> consoleHandler(new ClientConsole(client.get()));
        EventManager epollWaiter;
        epollWaiter.addHandler(client);
        epollWaiter.addHandler(consoleHandler);
        epollWaiter.wait();
    } catch (HandlerException const &e) {
        cerr << e.what() << endl;
    } catch (EventException const &e) {
        cerr << e.what() << endl;
    } catch (logic_error const &e) {
        cerr << "Invalid arguments." << endl;
        cerr << e.what() << endl;
    }
}
