#include "stdafx.h"
#include "AssetBrowser.h"
#include "../imgui/ImGuiManager.h"
#include "../imgui/FileTree.h"
#include "../Rendering/Texture.h"
#include "../Rendering/RenderingDevice.h"
#include "../Framework/Framework.h"

AssetBrowser::AssetBrowser()
{
	m_selection = FileTreeState();
}

AssetBrowser::~AssetBrowser()
{

}

void AssetBrowser::imgui(bool* open, const char* resPath)
{
    if (!open)
    {
        ResourcePtr<ImGuiManager> imgui;
        open = imgui->win("Asset Browser");
        if (*open == false)
            return;
    }

	if (ImGui::Begin("Asset Browser", open, ImGuiWindowFlags_MenuBar))
	{
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Close")) 
                    *open = false;
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        if (!resPath)
        {
            static std::string resPathStr = std::string(Framework::getResPath()) + "2D assets";
            resPath = resPathStr.c_str();
        }

        ImGui::Columns(2);
        ImGui::SetColumnWidth(-1, 300);

        // left
        ImGui::BeginChild("left pane", ImVec2(0, 0), false);
            const char* selected = FileTree(resPath, m_selection.getPtr<FileTreeState>());
            setCurrent(selected);
        ImGui::EndChild();
        ImGui::NextColumn();

        // right
        ImGui::BeginGroup();
        ImGui::BeginChild("top part", ImVec2(0, 18));
        ImGui::Text("%s (%d)", selected ? selected : "(null)", m_current.m_textures.size());
        ImGui::Separator();
        ImGui::EndChild();

        ImGui::BeginChild("item view");
        float padding = ImGui::GetStyle().FramePadding.x;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        int itemsPerRow = (int) (ImGui::GetWindowContentRegionWidth() / (32.0f + (padding * 2) + spacing));
        itemsPerRow = std::max(itemsPerRow, 1);
        for (int i = 0; i < m_current.m_textures.size(); i++)
        {
            const CurrentInfo::Texture& current = *m_current.m_textures[i];
            Rendering::Texture* texture = &(*current.m_texture);
            ImGui::ImageButton(texture, ImVec2(32, 32), ImVec2(0, 0), ImVec2(1, 1), -1);
            if (ImGui::IsItemHovered())
            {
                ImVec2 size;
                if (current.m_width < 256 && current.m_height < 256)
                {
                    size = ImVec2((float)current.m_width, (float)current.m_height);
                }
                else
                {
                    float ratio = (float)current.m_width / (float)current.m_height;
                    size = ImVec2(256.0f * ratio, 256.0f);
                }

                ImGui::BeginTooltip();                
                ImGui::Image(texture, size);
                ImGui::Separator();
                ImGui::Text("%s (%d x %d)", current.m_name.c_str(), current.m_width, current.m_height);
                ImGui::EndTooltip();
            }

            if (i % itemsPerRow != itemsPerRow - 1)
                ImGui::SameLine();
        }
        ImGui::EndChild();
        ImGui::EndGroup();

        ImGui::Columns(1);
    }

	ImGui::End();
}

void AssetBrowser::setCurrent(const char* path)
{
    if (path == m_current.m_path)
        return;

    ResourcePtr<Rendering::Device> device;
    device->waitIdle();
    m_current.m_textures.clear();

    if (!path)
        return;

    ResourcePtr<FileManager> fileManager;
    std::vector<FileManager::FileInfo> files = fileManager->files(path);
    for (int i = 0; i < files.size(); ++i)
    {
        FileManager::FileInfo& currentFile = files[i];
        const std::string& path = currentFile.m_path;
        if (currentFile.m_type != FileManager::Type::File)
            continue;
        
        if (path.substr(path.find_last_of('.')) == ".png")
        {
            ResourcePtr<File> file(NewPtr, path.c_str());
            ResourcePtr<VulkanFramework> vf;

            unsigned char* pixels;
            unsigned int width, height;
            int error = lodepng_decode32(&pixels, &width, &height, (const unsigned char*)file->getContents().c_str(), file->getSize());
            if (error == 0)
            {
                std::shared_ptr<Rendering::Texture> texture(new Rendering::Texture());
                texture->setSoftware(width, height, 32);
                char* dest = (char*)texture->map();
                memcpy(dest, pixels, width * height * 4);
                texture->unmap();
                free(pixels);

                vf->uploadTexture(&(*texture));

                std::size_t name = path.find_last_of('/') + 1;
                m_current.m_textures.push_back(std::shared_ptr<CurrentInfo::Texture>(new CurrentInfo::Texture{ path.substr(name), (int)width, (int)height, texture }));
            }
        }
    }

    m_current.m_path = path;
}