#pragma once

#include <iostream>

struct InvalidOperationException : public std::logic_error {
public:
    InvalidOperationException();
    explicit InvalidOperationException(const std::string message);
};
