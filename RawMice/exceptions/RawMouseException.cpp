#include "RawMouseException.h"

#include <system_error>
#include <exception>


RawMouseException::RawMouseException(const std::string& message) noexcept : RawMouseException(message, ::GetLastError()) { }
RawMouseException::RawMouseException(const std::string& message, const DWORD errorCode) noexcept : _message(message), _errorCode(errorCode), std::runtime_error(message) { }

const char* RawMouseException::what() const noexcept {
	return this->_message.c_str();
}

const std::string RawMouseException::apiErrorMessage() const noexcept {
	return this->_message;
}

const std::string RawMouseException::windowsErrorMessage() const noexcept {
	if (this->_errorCode == 0) {
		return std::string();
	} else {
		return std::system_category().message(this->_errorCode);
	}
}

const DWORD RawMouseException::windowsErrorCode() const noexcept {
	return this->_errorCode;
}
