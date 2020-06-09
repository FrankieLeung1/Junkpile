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
	ofbx::IScene* scene = ofbx::load((ofbx::u8*)file->getContents(), (int)file->getSize(), (ofbx::u64)ofbx::LoadFlags::TRIANGULATE);
	const ofbx::Mesh* mesh = scene->getMesh(0);
	const ofbx::Geometry* geo = mesh->getGeometry();
	const ofbx::Vec3* positions = geo->getVertices();
	const ofbx::Vec2* uvs = geo->getUVs();
	const int* face = geo->getFaceIndices();

	//scene->getGlobalSettings();

	// setup and copy into vertex buffer
	struct Vert {
		glm::vec3 m_position;
		glm::vec2 m_uv;
	};
	data->m_vertexCount = geo->getIndexCount();
	data->m_vBuffer = new Rendering::Buffer(Rendering::Buffer::Vertex, Rendering::Buffer::Usage::Mapped, data->m_vertexCount * sizeof(Vert));
	data->m_vBuffer->setFormat({
		{vk::Format::eR32G32B32Sfloat, sizeof(glm::vec3)},
		{vk::Format::eR32G32Sfloat, sizeof(glm::vec2)}
	}, sizeof(Vert));

	Vert* vertexDest = (Vert*)data->m_vBuffer->map();
	for (int i = 0; i < data->m_vertexCount; i++)
	{
		int posIndex = (face[i] < 0 ? (-face[i]) - 1 : face[i]);
		vertexDest[i].m_position = glm::vec3(positions[posIndex].x, positions[posIndex].y, positions[posIndex].z);
		vertexDest[i].m_uv = glm::vec2(uvs[i].x, 1.0f - uvs[i].y);
	}
	data->m_vBuffer->unmap();

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