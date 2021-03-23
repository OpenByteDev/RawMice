#include "RawMouseHandler.h"
#include "hidusage.h"
#include "exceptions/RawMouseException.h"
#include "exceptions/InvalidOperationException.h"

#include <system_error>


RawMouseHandler::RawMouseHandler() noexcept : RawMouseHandler(nullptr) { };
RawMouseHandler::RawMouseHandler(const HWND windowHandle) noexcept : windowHandle(windowHandle), running(false) { };

void RawMouseHandler::start() {
	if (this->running) {
		throw new InvalidOperationException("Already running");
	}

	// Register for mouse messages from WM_INPUT.
	RAWINPUTDEVICE rid{
		.usUsagePage = HID_USAGE_PAGE_GENERIC,
		.usUsage = HID_USAGE_GENERIC_MOUSE,
		.dwFlags = RIDEV_DEVNOTIFY, // RIDEV_NOLEGACY
		.hwndTarget = this->windowHandle
	};

	if (auto result = RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE)); !result) {
		throw new RawMouseException("Failed to register for raw input updates.");
		return;
	}

	this->running = true;
};

void RawMouseHandler::stop() {
	// Unregister for mouse messages from WM_INPUT.
	RAWINPUTDEVICE rid{
		.usUsagePage = HID_USAGE_PAGE_GENERIC,
		.usUsage = HID_USAGE_GENERIC_MOUSE,
		.dwFlags = RIDEV_REMOVE,
		.hwndTarget = nullptr
	};

	if (auto result = RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE)); !result) {
		throw new RawMouseException("Failed to unregister for raw input updates.");
		return;
	}

	this->running = false;
};

RawMouseHandler::~RawMouseHandler() {
	try {
		if (this->running)
			this->stop();
	} catch (RawMouseException&) {
		this->running = false;
	}
};

void RawMouseHandler::handleMessage(const MSG& msg) {
	switch (msg.message) {
		case WM_INPUT:
			this->handleRawInput((HRAWINPUT)msg.lParam);
			break;
		case WM_INPUT_DEVICE_CHANGE:
			this->handleDeviceChange((HANDLE)msg.lParam, msg.wParam);
			break;
		default:
			break;
	}
};

void RawMouseHandler::handleRawInput(const HRAWINPUT input) {
	unsigned int size;
	if (auto result = GetRawInputData(input, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)); result == -1) {
		throw new RawMouseException("Failed to get raw input data size");
		return;
	}

	RAWINPUT raw;

	if (sizeof(RAWINPUT) != size) {
		throw new RawMouseException("Failed to get raw input data.");
		return;
	}

	if (auto result = GetRawInputData(input, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER));
		result != size) {
		throw new RawMouseException("Failed to get raw input data.");
		return;
	}

	if (raw.header.dwType != RIM_TYPEMOUSE)
		return;

	auto mouse = this->miceMap[raw.header.hDevice];
	if (mouse == nullptr) {
		return;
	}

	if (raw.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN) mouse->buttonStates[0] = true;
	if (raw.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_1_UP)   mouse->buttonStates[0] = false;
	if (raw.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN) mouse->buttonStates[1] = true;
	if (raw.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_2_UP)   mouse->buttonStates[1] = false;
	if (raw.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_3_DOWN) mouse->buttonStates[2] = true;
	if (raw.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_3_UP)   mouse->buttonStates[2] = false;

	if (raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)          mouse->isAbsolute = true;
	else if (raw.data.mouse.usFlags & MOUSE_MOVE_RELATIVE)     mouse->isAbsolute = false;
	if (raw.data.mouse.usFlags & MOUSE_VIRTUAL_DESKTOP)        mouse->isVirtualDesktop = true;
	else                                                       mouse->isVirtualDesktop = false;

	mouse->position.x = raw.data.mouse.lLastX;
	mouse->position.y = raw.data.mouse.lLastY;

	if (raw.data.mouse.usButtonFlags & RI_MOUSE_WHEEL)
		mouse->wheel = ((float)raw.data.mouse.usButtonData) / WHEEL_DELTA;

	if (mouse->isAbsolute) {
		mouse->position.x = raw.data.mouse.lLastX;
		mouse->position.y = raw.data.mouse.lLastY;
	} else { // relative
		mouse->position.x += raw.data.mouse.lLastX;
		mouse->position.y += raw.data.mouse.lLastY;
	}
};

void RawMouseHandler::handleDeviceChange(const HANDLE hDevice, WPARAM wParam) {
	switch (wParam) {
		case GIDC_ARRIVAL:
			this->tryRegisterMouse(hDevice);
			break;
		case GIDC_REMOVAL:
			this->tryUnregisterMouse(hDevice);
			break;
		default:
			break;
	}
};
#include <fstream>
void RawMouseHandler::tryRegisterMouse(const HANDLE hDevice) {
	RID_DEVICE_INFO deviceInfo;
	deviceInfo.cbSize = sizeof(RID_DEVICE_INFO);
	if (UINT size = sizeof(RID_DEVICE_INFO); GetRawInputDeviceInfo(hDevice, RIDI_DEVICEINFO, &deviceInfo, &size) != size) {
		throw new RawMouseException("Unable to determine raw device info.");
		return;
	}

	UINT nameLength;
	if (GetRawInputDeviceInfoA(hDevice, RIDI_DEVICENAME, nullptr, &nameLength) != 0) {
		throw new RawMouseException("Unable to determine raw device name length.");
		return;
	}

	std::string name; name.resize(nameLength);
	auto realLength = GetRawInputDeviceInfoA(hDevice, RIDI_DEVICENAME, name.data(), &nameLength);
	if (realLength == 0) {
		throw new RawMouseException("Unable to determine raw device name.");
		return;
	}
	name.resize(realLength);

	auto mouseData = std::make_unique<RawMouseData>();
	mouseData->deviceHandle = hDevice;
	mouseData->name = std::move(name);
	this->miceMap.insert(std::make_pair(mouseData->deviceHandle, mouseData.get()));
	this->mice.push_back(std::move(mouseData));
};

void RawMouseHandler::tryUnregisterMouse(const HANDLE hDevice) {
	this->miceMap.erase(hDevice);
	for (size_t i = 0; i < this->mice.size(); i++) {
		if (this->mice[i]->deviceHandle == hDevice) {
			this->mice.erase(this->mice.cbegin() + i);
			return;
		}
	}
};

const MousePosition RawMouseHandler::getMousePositionDelta(const size_t index) const noexcept {
	auto position = this->mice[index]->position;
	this->mice[index]->position = MousePosition{ .x = 0, .y = 0 };
	return position;
};
const RawMouseData* RawMouseHandler::getMouseData(const size_t index) const noexcept {
	return this->mice[index].get();
};

const bool RawMouseHandler::getLeftMouseButtonUp(const size_t mouseIndex) const noexcept {
	return this->getMouseButtonUp(mouseIndex, 0);
};
const bool RawMouseHandler::getLeftMouseButtonDown(const size_t mouseIndex) const noexcept {
	return !this->getLeftMouseButtonUp(mouseIndex);
};
const bool RawMouseHandler::getRightMouseButtonUp(const size_t mouseIndex) const noexcept {
	return this->getMouseButtonUp(mouseIndex, 1);
};
const bool RawMouseHandler::getRightMouseButtonDown(const size_t mouseIndex) const noexcept {
	return !this->getRightMouseButtonUp(mouseIndex);
};
const bool RawMouseHandler::getMiddleMouseButtonUp(const size_t mouseIndex) const noexcept {
	return this->getMouseButtonUp(mouseIndex, 2);
};
const bool RawMouseHandler::getMiddleMouseButtonDown(const size_t mouseIndex) const noexcept {
	return !this->getMiddleMouseButtonUp(mouseIndex);
};
const bool RawMouseHandler::getMouseButtonUp(const size_t mouseIndex, const size_t buttonIndex) const noexcept {
	return this->mice[mouseIndex]->buttonStates[buttonIndex];
};
const bool RawMouseHandler::getMouseButtonDown(const size_t mouseIndex, const size_t buttonIndex) const noexcept {
	return !this->getMouseButtonUp(mouseIndex, buttonIndex);
};

const size_t RawMouseHandler::getMouseCount() const noexcept {
	return this->mice.size();
};
