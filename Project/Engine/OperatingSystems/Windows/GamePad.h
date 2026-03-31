#pragma once
#include <windows.h>
#include <Xinput.h>

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
