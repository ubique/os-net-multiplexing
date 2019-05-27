//
// Created by utusi on 16.05.19.
//
#pragma once

#include <string>
#include <vector>

enum States {AUTHORIZATION, TRANSACTION};

const std::string CRLF = "\r\n";

class Utils {
public:
    static std::vector<std::string> split(const std::string& str);
private:
};
