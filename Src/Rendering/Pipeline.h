#pragma once

#include "../Resources/ResourceManager.h"

class LuaTableResource;
namespace Rendering
{
	class Device;
	class Pipeline : public Resource
	{
	public:

	public:
		class PipelineLoader : public Loader
		{
		public:
			PipelineLoader(const char* dataPath);
			~PipelineLoader();
			Pipeline* load(std::tuple<int, std::string>* error);
			std::string getDebugName() const;
			const char* getTypeName() const;

		protected:
			ResourcePtr<LuaTableResource> m_table;
			ResourcePtr<Device> m_device;
		};
		static PipelineLoader* createLoader(const char* dataFilePath);

	protected:
		vk::Pipeline m_pipeline;
	};
}