#pragma once

#include "../Resources/ResourceManager.h"
#include "../ECS/ComponentManager.h"
#include "ModelManager.h"

class ModelSystem;
struct ModelComponent : public Component<ModelComponent, ModelSystem>
{
	Model m_model;
	ResourcePtr<Rendering::Texture> m_texture1{ EmptyPtr };
};

struct UpdateEvent;
class ModelSystem : public SingletonResource<ModelSystem>
{
public:
	ModelSystem();
	~ModelSystem();

	static void test();

protected:
	void update(float);

protected:
	Entity m_cameraEntity;


};