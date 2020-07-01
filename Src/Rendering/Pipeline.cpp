#include "stdafx.h"
#include "Pipeline.h"
#include "../LuaHelpers.h"
#include "../Rendering/RenderingDevice.h"

using namespace Rendering;

Pipeline::PipelineLoader* Pipeline::createLoader(const char* dataFilePath)
{
	return new PipelineLoader(dataFilePath);
}

Pipeline::PipelineLoader::PipelineLoader(const char* dataPath):
m_table(NewPtr, dataPath),
m_device()
{

}

Pipeline::PipelineLoader::~PipelineLoader()
{

}

Pipeline* Pipeline::PipelineLoader::load(std::tuple<int, std::string>* error)
{
	if (!ready(error, m_table, m_device))
		return nullptr;

	vk::Device device = m_device->getDevice();

	//device.createGraphicsPipeline();

	return new Pipeline;
}

std::string Pipeline::PipelineLoader::getDebugName() const
{
	return "Pipeline";
}

StringView Pipeline::PipelineLoader::getTypeName() const
{
	return "Pipeline";
}