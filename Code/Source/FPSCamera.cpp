// Copyright ⓒ 2018 Valentyn Bondarenko. All rights reserved.

#include <StdAfx.h>

#include <FPSCamera.h>

namespace bm
{
    FPSCamera::FPSCamera(const float& width, const float& height)
    {
	position = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        target = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        view = DirectX::XMMatrixLookAtLH(position, target, up);
        projection = DirectX::XMMatrixPerspectiveFovLH(0.4f * DirectX::XM_PI, width / height, 1.f, 100'000.f);
        world = DirectX::XMMatrixIdentity();
    }

    void FPSCamera::update()
    {
        rotation = DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
        target = DirectX::XMVector3TransformCoord(default_forward, rotation);
        target = DirectX::XMVector3Normalize(target);

        right = DirectX::XMVector3TransformCoord(default_right, rotation);
        forward = DirectX::XMVector3TransformCoord(default_forward, rotation);
        up = DirectX::XMVector3Cross(forward, right);

        position += left_right * right;
        position += back_forward * forward;

        left_right = 0.0f;
        back_forward = 0.0f;

        target = position + target;

        view = DirectX::XMMatrixLookAtLH(position, target, up);
    }

	void FPSCamera::setPosition(const float& x, const float& y, const float& z)
	{
            position = DirectX::XMVectorSet(x, y, z, 0);
	}

    void FPSCamera::setRotation(const float& x, const float& y, const float& z)
    {
        // 0.0174532925 is a degree coefficient.
        pitch = x * 0.0174532925f;
        yaw = y * 0.0174532925f;
        roll = z * 0.0174532925f;
    }
}
