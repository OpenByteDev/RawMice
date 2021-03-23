#pragma once

#include <stdexcept>
#include <string>
#include <Windows.h>


class RawMouseException : public std::runtime_error {
public:
    RawMouseException(const std::string& message) noexcept;
    RawMouseException(const std::string& message, const DWORD errorCode) noexcept;

	const char* what() const noexcept override;
    const std::string apiErrorMessage() const noexcept;
    const std::string windowsErrorMessage() const noexcept;
    const DWORD windowsErrorCode() const noexcept;
private:
	const DWORD _errorCode;
	const std::string _message;
};
