#include "Entity.h"

Entity::Entity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material) :
	mesh(mesh), material(material)
{
	transform = std::make_shared<Transform>();
}

Entity::~Entity() {};


#pragma region GETTERS

std::shared_ptr<Mesh> Entity::GetMesh()
{
	return mesh;
}

std::shared_ptr<Transform> Entity::GetTransform()
{
	return transform;
}

std::shared_ptr<Material> Entity::GetMaterial()
{
	return material;
}

#pragma endregion 

#pragma region SETTERS

void Entity::SetMesh(std::shared_ptr<Mesh> mesh)
{
	this->mesh = mesh;
}

void Entity::SetMaterial(std::shared_ptr<Material> material)
{
	this->material = material;
}

#pragma endregion