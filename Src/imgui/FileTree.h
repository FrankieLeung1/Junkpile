#pragma once

#include "../Files/FileManager.h"
struct FileTreeState { int m_selected; std::string m_name; std::string m_path; std::vector<FileTreeState> m_children; };
void initState(FileTreeState* state, const char* startingPath);

inline const char* FileTree(const char* startingPath, FileTreeState* state)
{
    if (state->m_path.empty() && state->m_children.empty())
        initState(state, startingPath);

    std::function<bool(FileTreeState* state, bool selected)> renderState;
    renderState = [&renderState](FileTreeState* state, bool selected)
    {
        bool isRoot = state->m_name.empty();
        ImGuiTreeNodeFlags nodeFlags = 0;

        if (state->m_children.empty())   nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        else                            nodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

        if (selected) nodeFlags |= ImGuiTreeNodeFlags_Selected;

        char name[512];
        sprintf_s(name, "%s (%zd)", state->m_name.c_str(), state->m_children.size());
        bool nodeOpen = isRoot ? true : ImGui::TreeNodeEx(state, nodeFlags, name);
        bool clicked = isRoot ? false : ImGui::IsItemClicked();

        if (nodeOpen)
        {
            for (int i = 0; i < state->m_children.size(); i++)
            {
                if (state->m_selected != i)
                    state->m_children[i].m_selected = -1;

                if (renderState(&state->m_children[i], state->m_selected == i))
                {
                    state->m_selected = i;
                    clicked = true;
                }
            }

            if (!isRoot && !state->m_children.empty())
                ImGui::TreePop();
        }

        return clicked;
    };

    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 4);
        renderState(state, false);
    ImGui::PopStyleVar();

    FileTreeState* current = state;
    while (true)
    {
        if (current->m_selected < 0)
            return current->m_path.c_str();

        current = &current->m_children[current->m_selected];
    }
    return nullptr;
}

inline void initState(FileTreeState* state, const char* path)
{
    ResourcePtr<FileManager> files;
    std::vector<FileManager::FileInfo> paths = files->files(path);
    for (FileManager::FileInfo& currentFile : paths)
    {
        if (currentFile.m_type != FileManager::Directory)
            continue;

        std::string& currentPath = currentFile.m_path;
        files->reducePath(currentPath);

        std::size_t lastSlash = currentPath.find_last_of('/');
        FileTreeState child = { -1, currentPath.substr(lastSlash + 1), currentPath, std::vector<FileTreeState>() };
        initState(&child, currentPath.c_str());
        state->m_children.push_back(child);
    }
}