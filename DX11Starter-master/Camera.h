#pragma once
#include "Transform.h"
#include "Input.h"
#include <memory>

class Camera
{
public:
	Camera(
		float x, float y, float z, 
		float moveSpeed, 
		float sprintMoveSpeed, 
		float mouseLookSpeed, 
		float fov, 
		float aspectRatio,
		float nearClip = 0.01f,
		float farClip = 1000.0f);

	~Camera();

	// Have constructor for strating orientation 

	void Update(float dt);
	void UpdateViewMatrix();
	void UpdateProjMatrix(float fov, float aspectRatio);
	Transform* GetTransform();

	// Getters 
	std::shared_ptr<DirectX::XMFLOAT4X4> GetViewMatrix();
	std::shared_ptr<DirectX::XMFLOAT4X4> GetProjMatrix();
	float GetCommonMoveSpeed();
	float GetSprintMoveSpeed();
	float GetMouseLookSpeed();
	float GetNearClip();
	float GetFarClip();

	// Setters 
	void SetCommonMoveSpeed(float nextSpeed);
	void SetSprintMoveSpeed(float nextSpeed);
	void SetMouseLookSpeed(float nextSpeed);
	void SetNearClip(float next);
	void SetFarClip(float next);

private:
	// Primary matrices 
	std::shared_ptr<DirectX::XMFLOAT4X4> viewMatrix;
	std::shared_ptr<DirectX::XMFLOAT4X4> projMatrix;


	std::shared_ptr<float> moveSpeed;
	std::shared_ptr<float> sprintMoveSpeed;
	std::shared_ptr<float> mouseLookSpeed;
	std::shared_ptr<float> nearClip;
	std::shared_ptr<float> farClip;
	Transform* transform;
};