#pragma once
#include "KH_Common.h"

class KH_Ray;

enum class CameraMovement
{
    Forward,
    Backward,
    Left,
    Right,
    Up,
    Down
};

class KH_Camera
{
public:
    KH_Camera(
        uint32_t width = 1920, uint32_t height = 1080,
        glm::vec3 position = { 2.0f, 2.0f, 2.0f },
        glm::vec3 up = { 0.0f, 1.0f, 0.0f },
        float yaw = -140.0f,
        float pitch = -35.0f
    );

    glm::mat4 GetViewMatrix() const;

    glm::mat4 GetProjMatrix() const;


    KH_Ray GetRay(int i, int j, int width, int height) const;

    KH_Ray GetRay(int i, int j) const;

    KH_Ray GetRay(float u, float v) const;

    KH_Ray GetRay(glm::vec2 ndc) const;

    void UpdateAspect();

    void ProcessKeyboard(CameraMovement direction, float deltaTime);
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    void ProcessMouseScroll(float yoffset);

    // ²ÎÊý
    float MovementSpeed = 2.5f;
    float MouseSensitivity = 0.1f;
    float SpeedSensitivity = 0.5f;
    float Fovy = 45.0f;
    float NearPlane = 0.1f;
    float FarPlane = 100.0f;
    float Aspect = 0.0f;
    uint32_t Width;
    uint32_t Height;

    // ×´Ì¬
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;

private:
    void UpdateCameraVectors();

    glm::vec3 GetRayDirection(int i, int j, int width, int height) const;

    glm::vec3 GetRayDirection(float u, float v) const;
};