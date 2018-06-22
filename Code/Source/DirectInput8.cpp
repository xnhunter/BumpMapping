// Copyright ⓒ 2018 Valentyn Bondarenko. All rights reserved.

#include <StdAfx.h>

#include <DirectInput8.h>

namespace bm
{
    DirectInput8::DirectInput8(HWND handle)
    {
        auto hr = DirectInput8Create(GetModuleHandleA(nullptr),
                                     DIRECTINPUT_VERSION,
                                     IID_IDirectInput8,
                                     reinterpret_cast<void**>(&direct_input),
                                     nullptr);

        hr = direct_input->CreateDevice(GUID_SysKeyboard,
                                       &DIKeyboard,
                                       nullptr);

        hr = direct_input->CreateDevice(GUID_SysMouse,
                                       &DIMouse,
                                       nullptr);

        hr = DIKeyboard->SetDataFormat(&c_dfDIKeyboard);
        hr = DIKeyboard->SetCooperativeLevel(handle, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

        hr = DIMouse->SetDataFormat(&c_dfDIMouse);
        hr = DIMouse->SetCooperativeLevel(handle, DISCL_EXCLUSIVE | DISCL_NOWINKEY | DISCL_FOREGROUND);
    }

    DirectInput8::~DirectInput8()
    {
        if(DIMouse != nullptr)
            DIMouse->Release();

        if(DIKeyboard != nullptr)
            DIKeyboard->Release();

        if(direct_input != nullptr)
            direct_input->Release();
    }

    void DirectInput8::update(float& moveLeftRight, float& moveBackForward, float& camYaw, float& camPitch)
    {
        DIMOUSESTATE mouseCurrState;
        BYTE keyboardState[256];

        DIKeyboard->Acquire();
        DIMouse->Acquire();

        DIMouse->GetDeviceState(sizeof(decltype(mouseCurrState)), &mouseCurrState);
        DIKeyboard->GetDeviceState(sizeof(decltype(keyboardState)), reinterpret_cast<void**>(&keyboardState));

        auto move_speed = 1.5f;

        if(keyboardState[DIK_A] & 0x80)
            moveLeftRight -= move_speed;

        if(keyboardState[DIK_D] & 0x80)
            moveLeftRight += move_speed;

        if(keyboardState[DIK_W] & 0x80)
        {
            moveBackForward += move_speed;

			if((keyboardState[DIK_LSHIFT] & 0x80))
				moveBackForward += move_speed * 5;
        }

        if(keyboardState[DIK_S] & 0x80)
        {
            moveBackForward -= move_speed;

            if((keyboardState[DIK_LSHIFT] & 0x80))
                moveBackForward -= move_speed * 5;
        }

        if((mouseCurrState.lX != mouseLastState.lX))
            camYaw += mouseCurrState.lX * 0.001f;

		if((mouseCurrState.lY != mouseLastState.lY))
			camPitch += mouseCurrState.lY * 0.001f;

		mouseLastState = mouseCurrState;
    }
}