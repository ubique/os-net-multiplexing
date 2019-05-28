#pragma once

#include <iostream>
#include "EventManager.h"


class ConsoleHandler : public IHandler {
public:

    void handleError(EventManager &eventManager) override {
        error_t error = getError(0);
        eventManager.deleteAll();
        if (error != 0) {
            throw HandlerException("Console failed.", error);
        }
    }

    int getFD() override { return STDIN_FILENO; }

    ~ConsoleHandler() override = default;
};
