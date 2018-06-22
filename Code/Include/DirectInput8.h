// Copyright ⓒ 2018 Valentyn Bondarenko. All rights reserved.

#pragma once

namespace bm
{
    class DirectInput8
    {
    public:
        DirectInput8(HWND handle);
       ~DirectInput8();

	    DirectInput8(const DirectInput8&) = delete;
		DirectInput8(DirectInput8&&) = delete;

		DirectInput8& operator=(const DirectInput8&) = delete;
		DirectInput8& operator=(DirectInput8&&) = delete;

    public:
        void update(float& moveLeftRight, float& moveBackForward, float& camYaw, float& camPitch);

    private:
        IDirectInput8A* direct_input;

        IDirectInputDevice8A* DIKeyboard;
        IDirectInputDevice8A* DIMouse;

        DIMOUSESTATE mouseLastState;
    };
}