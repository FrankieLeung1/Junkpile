#pragma once

#include "../Resources/ResourceManager.h"
#include "../ECS/ComponentManager.h"
#include "ModelManager.h"

struct ModelComponent : public Component<ModelComponent>
{
	Model m_model;
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


};