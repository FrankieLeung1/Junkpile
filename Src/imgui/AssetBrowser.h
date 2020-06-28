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

		struct Texture
		{
			std::string m_name;
			int m_width, m_height;
			std::shared_ptr<Rendering::Texture> m_texture;
		};
		std::vector<std::shared_ptr<Texture>> m_textures;
	} m_current;
	
};