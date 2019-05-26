#pragma once

#include <iostream>


class ConsoleHandler : public IHandler {
public:

    void handleError(EpollWaiter &waiter) override {
        error_t error = getError(0);
        waiter.deleteAll();
        if (error != 0) {
            throw HandlerException("Console failed.", error);
        }
    }

    int getFD() override { return STDIN_FILENO; }

    uint32_t getActions() override { return WAIT_INPUT; }

    ~ConsoleHandler() override = default;
};