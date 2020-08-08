#include "stdafx.h"
#include "AssetBrowser.h"
#include "../imgui/ImGuiManager.h"
#include "../imgui/FileTree.h"
#include "../Rendering/Texture.h"
#include "../Rendering/RenderingDevice.h"
#include "../Framework/Framework.h"
#include "../Threading/ThreadPool.h"
#include "../Scripts/ScriptManager.h"

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
            static std::string resPathStr = std::string(Framework::getResPath());
            resPath = resPathStr.c_str();
        }

        ImGui::Columns(2);
        static bool first = true;
        if (first)
        {
            ImGui::SetColumnWidth(-1, 200);
            first = false;
        }

        // left
        ImGui::BeginChild("left pane", ImVec2(0, 0), false);
            const char* selected = FileTree(resPath, m_selection.getPtr<FileTreeState>());
            setCurrent(selected);
        ImGui::EndChild();
        ImGui::NextColumn();

        std::lock_guard<std::mutex> l(m_current.m_mutex);

        // right
        ImGui::BeginGroup();
        ImGui::BeginChild("top part", ImVec2(0, 18));
        ImGui::Text("%s (%d)", selected ? selected : "(null)", m_current.m_files.size());
        ImGui::Separator();
        ImGui::EndChild();

        float itemViewHeight = ImGui::GetFrameHeightWithSpacing() - 52.0f;
        ImGui::BeginChild("item view", ImVec2(0.0f, itemViewHeight));
        float padding = ImGui::GetStyle().FramePadding.x;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        bool hasScript = false;
        int itemsPerRow = (int) (ImGui::GetWindowContentRegionWidth() / (32.0f + (padding * 2) + spacing));
        itemsPerRow = std::max(itemsPerRow, 1);
        for (int i = 0; i < m_current.m_files.size(); i++)
        {
            const CurrentInfo::File& current = *m_current.m_files[i];
            ResourcePtr<Rendering::Texture> texture = current.m_texture;
            std::tuple<int, std::string> error;
            if(!texture.ready(&error) || std::get<0>(error) != 0)
                continue;

            int width = (int)current.m_texture->getWidth(), height = (int)current.m_texture->getHeight();
            if (!texture->getVkImage())
            {
                ResourcePtr<VulkanFramework> vf;
                vf->uploadTexture(&(*texture));
            }

            ImGui::ImageButton(texture, ImVec2(32, 32), ImVec2(0, 0), ImVec2(1, 1), -1);
            if (ImGui::IsItemHovered())
            {
                ImVec2 size;
                if (width < 256 && height < 256)
                {
                    size = ImVec2((float)width, (float)height);
                }
                else
                {
                    float ratio = (float)width / (float)height;
                    size = ImVec2(256.0f * ratio, 256.0f);
                }

                ImGui::BeginTooltip();                
                ImGui::Image(texture, size);
                ImGui::Separator();
                ImGui::Text("%s (%d x %d)", current.m_name.c_str(), width, height);
                ImGui::EndTooltip();
            }

            if (i % itemsPerRow != itemsPerRow - 1)
                ImGui::SameLine();

            hasScript = (hasScript || current.m_type == CurrentInfo::File::Script);
        }
        ImGui::EndChild();
        if (hasScript)
        {
            ResourcePtr<ScriptManager> scripts;
            ImGui::BeginChild("Run");
            ImGui::SameLine(ImGui::GetColumnWidth() - 100.0f);
            if (ImGui::Button("Run All"))
            {
                // unload current level
                scripts->runScriptsInFolder(m_current.m_path);
            }
            ImGui::EndChild();
        }

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

    std::lock_guard<std::mutex> l(m_current.m_mutex);
    m_current.m_files.clear();
    m_current.m_path = path;

    if (!path)
        return;

    auto task = [this]()
    {
        std::string folderPath;
        {
            std::lock_guard<std::mutex> l(m_current.m_mutex);
            folderPath = m_current.m_path;
        }

        ResourcePtr<FileManager> fileManager;
        std::vector<FileManager::FileInfo> files = fileManager->files(folderPath.c_str());
        for (int i = 0; i < files.size(); ++i)
        {
            FileManager::FileInfo& currentFile = files[i];
            std::string path = currentFile.m_path;
            if (currentFile.m_type != FileManager::Type::File)
                continue;

            std::shared_ptr<CurrentInfo::File> data;
            std::size_t name = path.find_last_of('/') + 1;
            std::string ext = path.substr(path.find_last_of('.'));
            if (ext == ".png")
            {
                data = std::shared_ptr<CurrentInfo::File>(new CurrentInfo::File{ CurrentInfo::File::Type::Texture, path.substr(name), {NewPtr, StringView(path)} });
            }
            else if (ext == ".py")
            {
                data = std::shared_ptr<CurrentInfo::File>(new CurrentInfo::File{ CurrentInfo::File::Type::Script, path.substr(name), {NewPtr, StringView("Art/File Icons/Python.png")} });
            }

            if(data)
            {
                std::lock_guard<std::mutex> l(m_current.m_mutex);
                m_current.m_files.push_back(data);
            }

			if (folderPath != m_current.m_path)
				return;
        }
    };

    ResourcePtr<ThreadPool> pool;
    pool->enqueue(task);
}