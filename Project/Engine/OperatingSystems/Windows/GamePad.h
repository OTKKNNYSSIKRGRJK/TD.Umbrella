#pragma once
#include <windows.h>
#include <Xinput.h>

enum class GamePadButton : WORD {
    DPadUp        = 0x0001,
    DPadDown      = 0x0002,
    DPadLeft      = 0x0004,
    DPadRight     = 0x0008,
    Start         = 0x0010,
    Back          = 0x0020,
    LeftThumb     = 0x0040,
    RightThumb    = 0x0080,
    LeftShoulder  = 0x0100,
    RightShoulder = 0x0200,
    A             = 0x1000,
    B             = 0x2000,
    X             = 0x4000,
    Y             = 0x8000
};

class GamePad {
public:
    GamePad(int index = 0);
    ~GamePad() = default;
public:
    void Update();
    bool IsConnected() const;
    bool IsPress(WORD button);
    bool IsPressed(WORD button);
	bool IsHold(WORD button) const;
    bool IsRelease(WORD button) const;

    BYTE GetLeftTrigger() const;
    float GetLeftStickX() const;
    float GetLeftStickY() const;

    BYTE GetRightTrigger() const;
	float GetRightStickX() const;
	float GetRightStickY() const;
	

private:
    int controllerIndex_;
    XINPUT_STATE state_;
    XINPUT_STATE prevState_;
    bool isConnected_;
};
