#pragma once

class Narrative
{
public:
	Narrative();
	~Narrative();

	void imgui();

protected:
	static ImGui::Node* createNode(int nt, const ImVec2& pos, const ImGui::NodeGraphEditor& editor);

protected:
	class TestNode : public ImGui::Node {
	public:
		TestNode() {}
		~TestNode() {}
		template<typename T> T* newVar(T&& val) { std::shared_ptr<Any> ptr = std::make_shared<Any>(); *ptr = val; m_variables.push_back(ptr); return ptr->getPtr<T>(); }
		using ImGui::Node::init;
		using ImGui::Node::fields;
		std::vector<std::shared_ptr<Any>> m_variables;
	};
	const char* m_nodeNames[8] = {
		"Start",
		"Finish",
		"Give",
		"Animation",
		"Dialogue",
		"Delay",
		"Split",
		"If"
	};
	ImGui::NodeGraphEditor m_graph;
};