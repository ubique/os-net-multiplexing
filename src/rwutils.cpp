#include "rwutils.h"
#include <arpa/inet.h>
#include <iostream>

static const size_t BUFFER_SIZE = 4096;
namespace
{
std::string read_exact(int socket_fd, size_t message_size, char *buffer)
{
    ssize_t received_amount =
        recv(socket_fd, buffer,
             std::min(message_size, static_cast<size_t>(4096)), 0);
    if (received_amount == -1) {
        throw std::runtime_error("recv failed");
    }
    if (received_amount == 0) {
        return "";
    }
    return std::string{buffer, static_cast<size_t>(received_amount)};
}

void write_exact(int socket_fd, std::string const &message)
{
    char const *buffer = message.data();
    size_t offset = 0;
    do {
        ssize_t sent_amount =
            send(socket_fd, buffer + offset, message.size() - offset, 0);
        if (sent_amount == -1) {
            throw std::runtime_error("send failed");
        }
        if (sent_amount == 0) {
            throw std::runtime_error("unexpected end of input");
        }
        offset += static_cast<size_t>(sent_amount);
    } while (offset != message.length());
}

} // namespace

dummy_optional message_handler::read(int socket_fd) noexcept(false)
{
    if (current_state == state::NEED_SIZE) {
        std::string received =
            read_exact(socket_fd, 2 - current_message.size(), buffer);
        if (received.empty()) {
            return {"", true};
        }
        current_message += received;

        if (current_message.size() == 2) {
            message_size =
                *reinterpret_cast<uint16_t const *>(current_message.data());
            current_message.clear();
            current_state = state::NEED_MESSAGE;
        }
        return {"", false};
    }
    if (current_state == state::NEED_MESSAGE) {
        std::string received = read_exact(
            socket_fd,
            std::min(message_size - current_message.size(), ::BUFFER_SIZE),
            buffer);
        if (received.empty()) {
            return {"", true};
        }
        current_message += received;
        if (current_message.size() == message_size) {
            current_state = state::NEED_SIZE;
            std::string result;
            swap(result, current_message);
            return {result, true};
        }
    }
    return {"", false};
}

void message_handler::write(int socket_fd,
                            std::string const &message) noexcept(false)
{
    if (message.size() > ::BUFFER_SIZE) {
        throw std::runtime_error("message is too large");
    }
    uint16_t size = static_cast<uint16_t>(message.size());
    std::string message_size_representation =
        std::string{reinterpret_cast<char *>(&size), 2};
    write_exact(socket_fd, message_size_representation);
    write_exact(socket_fd, message);
}
