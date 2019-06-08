#ifndef MULTIPlEXING_RWUTILS_H
#define MULTIPLEXING_RWUTILS_H

#include <string>

struct dummy_optional {
    std::string value;
    bool is_valid;
};

struct message_handler {
    dummy_optional read(int fd) noexcept(false);
    void write(int socker_fd, std::string const &message) noexcept(false);

private:
    static const size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    uint16_t message_size;
    std::string current_message;

    enum class state {
        NEED_SIZE,
        NEED_MESSAGE
    } current_state = state::NEED_SIZE;
};

#endif
