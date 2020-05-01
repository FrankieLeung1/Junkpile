#include "stdafx.h"
#include "ModelManager.h"
#include "../Files/File.h"
#include "../Rendering/Buffer.h"
#include "../Managers/EventManager.h"
#include "../Misc/Misc.h"
#include "../Rendering/Buffer.h"

ModelManager::ModelManager()
{
	m_nextModelId.makeValid();
}

ModelManager::~ModelManager()
{
	for (auto v : m_models)
	{
		delete v.second.m_vBuffer;
		delete v.second.m_iBuffer;
	}
}

Model ModelManager::getModel(const char* _path)
{
	std::string path = normalizePath(_path);
	for (auto& model : m_models)
	{
		if (model.second.m_path == path)
			return model.first;
	}

	Model model = m_nextModelId.getAndAdvance();
	auto it = m_models.emplace(std::make_pair(model, ModelData(path.c_str())));
	ModelData& modelData = it.first->second;

	ResourcePtr<EventManager> events;
	events->addListener<ResourceStateChanged>([&modelData, this](ResourceStateChanged* e) {
		if (modelData.m_file == e->m_resourceData)
		{
			this->onModelDataLoaded(const_cast<ModelData*>(&modelData)); // where does this const come from...?
			e->discardListener();
		}
	});

	return model;
}

const ModelManager::ModelData* ModelManager::getModelData(const Model& model) const
{
	auto it = m_models.find(model);
	return it == m_models.end() ? nullptr : &it->second;
}

void ModelManager::onModelDataLoaded(ModelData* e)
{
	if (endsWith(e->m_path, ".fbx", 4))
		loadFBX(e);
}

void ModelManager::loadFBX(ModelData* data) const
{
	ResourcePtr<File>& file = data->m_file;
	ofbx::IScene* scene = ofbx::load((ofbx::u8*)file->getContents(), (int)file->getSize(), 0);
	const ofbx::Mesh* mesh = scene->getMesh(0);
	const ofbx::Geometry* geo = mesh->getGeometry();

	// setup and copy into vertex buffer
	data->m_vertexCount = geo->getVertexCount();
	data->m_vBuffer = new Rendering::Buffer(Rendering::Buffer::Vertex, Rendering::Buffer::Usage::Mapped, data->m_vertexCount * sizeof(glm::vec3));
	data->m_vBuffer->setFormat({ {vk::Format::eR32G32B32Sfloat, sizeof(glm::vec3)}, }, sizeof(glm::vec3));
	glm::vec3* vertexDest = (glm::vec3*)data->m_vBuffer->map();
	for (int i = 0; i < data->m_vertexCount; i++)
	{
		const ofbx::Vec3* vertex = &(geo->getVertices()[i]);
		vertexDest[i] = glm::vec3(vertex->x, vertex->y, vertex->z);
	}
	data->m_vBuffer->unmap();

	// setup and copy into index buffer
	data->m_indexCount = geo->getIndexCount();
	const int* indexSrc = geo->getFaceIndices();
	
	std::size_t currentFaceSize = 0;
	std::vector<unsigned int> indices; indices.reserve(data->m_indexCount * 2);
	for (int i = 0; i < data->m_indexCount; i++)
	{
		indices.push_back(indexSrc[i] < 0 ? (-indexSrc[i]) - 1 : indexSrc[i]);
		currentFaceSize++;

		if (indexSrc[i] < 0)
		{
			if (currentFaceSize == 3)
			{
				//indices.push_back(indices.back());
			}
			else
			{
				indices.push_back(indices[indices.size() - 4]);
				indices.push_back(indices[indices.size() - 3]);
			}
			currentFaceSize = 0;
		}
	}
	data->m_indexCount = indices.size();

	data->m_iBuffer = new Rendering::Buffer(Rendering::Buffer::Index, Rendering::Buffer::Usage::Mapped, data->m_indexCount * sizeof(int));
	data->m_iBuffer->setFormat({ {vk::Format::eR32Uint, sizeof(int)} }, sizeof(int));
	
	int* indexDest = (int*)data->m_iBuffer->map();
	memcpy(indexDest, &indices.front(), indices.size() * sizeof(unsigned int));
	data->m_iBuffer->unmap();

	scene->destroy();
}

ModelManager::ModelData::ModelData(const char* path):
m_path(path),
m_file(NewPtr, path),
m_vBuffer(nullptr),
m_iBuffer(nullptr),
m_vertexCount(0),
m_indexCount(0)
{

}