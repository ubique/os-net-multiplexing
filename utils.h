#ifndef OS_MULTIPLEXING_UTILS_H
#define OS_MULTIPLEXING_UTILS_H

void send_message(std::string message, int fd) {
    message += 3;
    size_t sended = 0;
    while (sended < message.size()) {
        ssize_t curr_portion = send(fd, message.data(), message.length(), 0);
        if (curr_portion == -1) {
            perror("Can't send message");
            continue;
        }
        sended += curr_portion;
    }
}

size_t receive_message(int fd, char *buffer, size_t buffer_size) {
    size_t received = 0;
    char last_received = 'a';

    while (last_received != 3) {
        ssize_t len = recv(fd, buffer, buffer_size, 0);
        if (len == -1) {
            perror("Can't read response./");
            continue;
        }
        if (len == 0) {
            break;
        }
        received += len;
        last_received = buffer[len - 1];
    }

    received--;
    buffer[received] = 0;
    
    return received;
}

#endif //OS_MULTIPLEXING_UTILS_H
