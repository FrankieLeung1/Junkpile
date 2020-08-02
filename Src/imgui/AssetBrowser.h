#pragma once

#include "../Resources/ResourceManager.h"

namespace Rendering {
	class Texture;
}

class AssetBrowser : public SingletonResource<AssetBrowser>
{
public:
	AssetBrowser();
	~AssetBrowser();

	void imgui(bool* open = nullptr, const char* resPath = nullptr);

protected:
	void setCurrent(const char* path);

protected:
	Any m_selection;
	struct CurrentInfo
	{
		std::string m_path;
		std::mutex m_mutex;

		struct File
		{
			enum Type { Texture, Script, Folder };
			Type m_type;
			std::string m_name;
			ResourcePtr<Rendering::Texture> m_texture;
		};
		std::vector<std::shared_ptr<File>> m_files;
	} m_current;
	
};