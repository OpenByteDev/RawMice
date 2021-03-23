#pragma once

#include "RawMouseData.h"

#include <Windows.h>
#include <vector>
#include <unordered_map>


class RawMouseHandler {
public:
	static constexpr size_t const MAX_MOUSE_BUTTON_COUNT = 3;
	RawMouseHandler() noexcept;
	explicit RawMouseHandler(const HWND windowHandle) noexcept;
	~RawMouseHandler() noexcept;

	void handleMessage(const MSG& msg);
	void start();
	void stop();

	const MousePosition getMousePositionDelta(const size_t mouseIndex) const noexcept;
	const RawMouseData* getMouseData(const size_t index) const noexcept;
	const bool getLeftMouseButtonUp(const size_t mouseIndex) const noexcept;
	const bool getLeftMouseButtonDown(const size_t mouseIndex) const noexcept;
	const bool getRightMouseButtonUp(const size_t mouseIndex) const noexcept;
	const bool getRightMouseButtonDown(const size_t mouseIndex) const noexcept;
	const bool getMiddleMouseButtonUp(const size_t mouseIndex) const noexcept;
	const bool getMiddleMouseButtonDown(const size_t mouseIndex) const noexcept;
	const bool getMouseButtonUp(const size_t mouseIndex, const size_t buttonIndex) const noexcept;
	const bool getMouseButtonDown(const size_t mouseIndex, const size_t buttonIndex) const noexcept;
	const size_t getMouseCount() const noexcept;

private:
	void handleRawInput(const HRAWINPUT input);
	void handleDeviceChange(const HANDLE input, const WPARAM wParam);
	void tryRegisterMouse(const HANDLE hDevice);
	void tryUnregisterMouse(const HANDLE hDevice);

	std::vector<std::unique_ptr<RawMouseData>> mice;
	std::unordered_map<HANDLE, RawMouseData*> miceMap;
	const HWND windowHandle;
	bool running;
};

