#pragma once
#include "Mesh.h"
#include "Transform.h"
#include "Material.h"

class Entity
{
public:
	// Constructor and Destructor 
	Entity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material);
	~Entity();

	// Getters 
	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<Transform> GetTransform();
	std::shared_ptr<Material> GetMaterial();
	 
	// Set mesh 
	void SetMesh(std::shared_ptr<Mesh>);
	void SetMaterial(std::shared_ptr<Material> material);
private:
	// Mesh pointer and transform
	// Mesh must be specified on constructor 
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> material;
	std::shared_ptr<Transform> transform;
};

