// Copyright ⓒ 2018 Valentyn Bondarenko. All rights reserved.

#pragma once

namespace bm
{
    class FPSCamera
    {
    public:
        FPSCamera(const float& width, const float& height);
       ~FPSCamera() = default;

        FPSCamera(const FPSCamera&) = default;
        FPSCamera(FPSCamera&&) = default;

        FPSCamera& operator=(const FPSCamera&) = default;
        FPSCamera& operator=(FPSCamera&&) = default;

    public:
        void update();

    public:
		void setPosition(const float& x, const float& y, const float& z);
		Vector getPosition() const { return position; }

        void setRotation(const float& x, const float& y, const float& z);
        Matrix getRotation() const { return rotation; }

    public:
        Matrix getWorld() { return world; }
        Matrix getView() { return view; }
        Matrix getProjection() { return projection; }

    public:
        float& getMoveLeftRight() { return left_right; }
        float& getMoveBackForward() { return back_forward; }

        float& getYaw() { return yaw; }
        float& getPitch() { return pitch; }

    private:
        Matrix projection;
        Matrix view;
        Matrix world;

        Matrix rotation;

        Vector position;
        Vector target;
        Vector up;
        Vector default_forward = DirectX::XMVectorSet(0.f, 0.f, 1.f, 0.f);
        Vector default_right = DirectX::XMVectorSet(1.f, 0.f, 0.0f, 0.f);
        Vector forward = DirectX::XMVectorSet(0.f, 0.f, 1.f, 0.f);
        Vector right = DirectX::XMVectorSet(1.f, 0.f, 0.f, 0.f);

        float left_right = 0.f;
        float back_forward = 0.f;

        float yaw = 0.f;
        float pitch = 0.f;
        float roll = 0.f;
    };
}