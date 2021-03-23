#pragma once

#include "RawMouseHandler.h"

#include <Windows.h>
#include <memory>
#include <array>
#include <string>

struct MousePosition {
	int x;
	int y;
};

struct RawMouseData {
	HANDLE deviceHandle = nullptr;
	std::array<bool, 3> buttonStates { false, false, false };
	MousePosition position{ .x = 0, .y = 0 };
	float wheel = 0;
	std::string name;
	bool isAbsolute = false;
	bool isVirtualDesktop = false;
};
