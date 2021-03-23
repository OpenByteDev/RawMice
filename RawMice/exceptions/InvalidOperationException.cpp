#include "InvalidOperationException.h"
#include <iostream>

InvalidOperationException::InvalidOperationException() :
    InvalidOperationException("Operation is not valid due to the current state of the object.") { }
InvalidOperationException::InvalidOperationException(const std::string message) :
    std::logic_error(message) {}
