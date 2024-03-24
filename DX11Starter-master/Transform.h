#pragma once
#include <DirectXMath.h>
#include <memory>
#include <vector>

class Transform
{
private:

	/// <summary>
	/// Represents this objects scale in x, y, and z components 
	/// </summary>
	DirectX::XMFLOAT3 scale;
	/// <summary>
	/// Represents this objects rotation as a euler 
	/// </summary>
	DirectX::XMFLOAT3 eulerRotation;

	DirectX::XMFLOAT4X4 world;
	DirectX::XMFLOAT4X4 worldTranspose;

	/// <summary>
	/// Set the matricies so that they accomodate mutations 
	/// </summary>
	void CleanMatrices();
	void CleanVectors();

	bool matIsDirty;
	bool dirIsDirty;

	// Local Vectors 
	DirectX::XMFLOAT3 right;
	DirectX::XMFLOAT3 up;
	DirectX::XMFLOAT3 forward;

	/// <summary>
	/// Represents this objects position in x, y, and z components 
	/// </summary>
	std::shared_ptr<DirectX::XMFLOAT3> position;

	std::vector<std::shared_ptr<Transform>> children;
	std::shared_ptr<Transform> parent;

public:


	/// <summary>
	/// Create a transform that represents a position, scale, and rotation in 3D space 
	/// </summary>
	Transform();

	#pragma region SETTERS
	/// <summary>
	/// Sets the position of this transform to the given components 
	/// </summary>
	void SetPosition(float x, float y, float z);
	/// <summary>
	/// Sets the position of this transform to the given vector 
	/// </summary>
	/// <param name="position"></param>
	void SetPosition(DirectX::XMFLOAT3 position);
	/// <summary>
	/// Sets the rotation of this transform to the given euler angles 
	/// </summary>
	void SetEulerRotation(float pitch, float yaw, float roll);
	/// <summary>
	/// Sets the rotation of this transform to the given euler angles 
	/// </summary>
	/// <param name="rotation"></param>
	void SetEulerRotation(DirectX::XMFLOAT3 rotation);
	/// <summary>
	/// Sets the scale of this transform to the given components
	/// </summary>
	void SetScale(float x, float y, float z);
	/// <summary>
	/// Sets the scale of this transform to the given vector 
	/// </summary>
	void SetScale(DirectX::XMFLOAT3 scale);
	/// <summary>
	/// Sets all scale components of this transform to the given value
	/// </summary>
	/// <param name="s"></param>
	void SetScale(float s);
	#pragma endregion

	#pragma region GETTERS
	/// <summary>
	/// Get this transform's current x, y, and z position in 3D space
	/// </summary>
	/// <returns></returns>
	std::shared_ptr<DirectX::XMFLOAT3> GetPosition();
	/// <summary>
	/// Get this transform's current euler rotation 
	/// </summary>
	/// <returns></returns>
	DirectX::XMFLOAT3 GetEulerRotation();
	/// <summary>
	/// Get this transform's current x, y, and z scalar components 
	/// </summary>
	/// <returns></returns>
	DirectX::XMFLOAT3 GetScale();
	/// <summary>
	/// Get this transform's world matrix that represents its position, rotation, and scale 
	/// </summary>
	/// <returns></returns>
	DirectX::XMFLOAT4X4 GetWorldMatrix();
	/// <summary>
	/// Get this trasnform's world inverse transpose matrix that represents its position, rotation, and scale 
	/// </summary>
	/// <returns></returns>
	DirectX::XMFLOAT4X4 GetWorldInverseTransposeMatrix();
	/// <summary>
	/// Get the vector that represents the direction right in orientation 
	/// </summary>
	/// <returns></returns>
	DirectX::XMFLOAT3 GetRight();
	/// <summary>
	/// Get the vector that represents the direction up in orientation 
	/// </summary>
	/// <returns></returns>
	DirectX::XMFLOAT3 GetUp();
	/// <summary>
	/// Get the vector that represents the direction forward in orientation 
	/// </summary>
	/// <returns></returns>
	DirectX::XMFLOAT3 GetForward();
	/// <summary>
	/// Get this transform's current parents 
	/// Nullptr if not possible 
	/// </summary>
	/// <returns></returns>
	std::shared_ptr<Transform> GetParent();
	/// <summary>
	/// Get the child trasnform based on index 
	/// Nullptr if not possible 
	/// </summary>
	/// <returns></returns>
	std::shared_ptr<Transform> GetChild(unsigned int index);
	/// <summary>
	/// Get the index of a child of this transform 
	/// -1 if not a child 
	/// </summary>
	/// <param name="child"></param>
	/// <returns></returns>
	int GetChildIndex(std::shared_ptr<Transform> child);
	/// <summary>
	/// Get the current amount of children directly under this transform
	/// </summary>
	/// <returns></returns>
	unsigned int GetChildCount();
	#pragma endregion

	#pragma region MUTATORS
	/// <summary>
	/// Shift this transform's position without taking rotation into account 
	/// </summary>
	void MoveAbs(float x, float y, float z);
	/// <summary>
	/// Shift this transform's position without taking rotation into account 
	/// </summary>
	void MoveAbs(DirectX::XMFLOAT3 offset);
	/// <summary>
	/// Shift this transform's position with taking rotation into account 
	/// </summary>
	void MoveRelative(float x, float y, float z);
	/// <summary>
	/// Shift this transform's position with taking rotation into account 
	/// </summary>
	void MoveRelative(DirectX::XMFLOAT3 offset);
	/// <summary>
	/// Rotate this transform by the given euler angles
	/// </summary>
	void RotateEuler(float pitch, float yaw, float roll);
	/// <summary>
	/// Rotate this transform by the given euler angles
	/// </summary>
	void RotateEuler(DirectX::XMFLOAT3);
	/// <summary>
	/// Scale this transform for each axis 
	/// </summary>
	void Scale(float x, float y, float z);
	/// <summary>
	/// Scale this transform for each axis 
	/// </summary>
	void Scale(DirectX::XMFLOAT3 scale);
	/// <summary>
	/// Scale this transform in all axis
	/// </summary>
	void Scale(float scale);
	#pragma endregion

	#pragma region HIERACHY

	/// <summary>
	/// Connect a transform to make it relative to this transform 
	/// </summary>
	/// <param name="child"></param>
	void AddChild(std::shared_ptr<Transform> child);
	/// <summary>
	/// If possible make a child no longer a child of this transform 
	/// </summary>
	/// <param name="child"></param>
	void RemoveChild(std::shared_ptr<Transform> child);
	/// <summary>
	/// If possible make a child no longer a child of this transform 
	/// </summary>
	/// <param name="childIndex"></param>
	void RemoveChild(int childIndex);
	/// <summary>
	/// Link this transform to a new parent 
	/// </summary>
	/// <param name="parent"></param>
	void SetParent(std::shared_ptr<Transform> parent);
	#pragma endregion
};
