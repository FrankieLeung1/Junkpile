#pragma once

#include "../Resources/ResourceManager.h"
#include "../ECS/ECS.h"

class File;
struct Model : public Handle<Model> { };
namespace Rendering
{
	class Buffer;
}

class ModelManager : public SingletonResource<ModelManager>
{
public:
	struct ModelData
	{
		ModelData(const char* path);

		std::string m_path;
		ResourcePtr<File> m_file;
		Rendering::Buffer *m_vBuffer, *m_iBuffer;
		std::size_t m_vertexCount, m_indexCount;
	};

public:
	ModelManager();
	~ModelManager();

	Model getModel(const char* path);
	const ModelData* getModelData(const Model&) const;

protected:
	struct ModelData;
	void onModelDataLoaded(ModelData*);
	void loadFBX(ModelData*) const;

protected:
	std::map<Model, ModelData> m_models;

	Model m_nextModelId;
};