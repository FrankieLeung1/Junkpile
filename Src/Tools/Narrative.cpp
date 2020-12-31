#include "stdafx.h"
#include "../Misc/Misc.h"
#include "Narrative.h"

Narrative::Narrative()
{
	m_graph.registerNodeTypes(m_nodeNames, (int)countof(m_nodeNames), &createNode, NULL, -1); 
	m_graph.registerNodeTypeMaxAllowedInstances(0, 1);
	m_graph.registerNodeTypeMaxAllowedInstances(1, 1);
	m_graph.user_ptr = this;

	auto node1 = m_graph.addNode(0, ImVec2(0, 0));
	auto node2 = m_graph.addNode(1, ImVec2(100.0f, 50.0f));
	m_graph.addLink(node1, 0, node2, 0);
}

Narrative::~Narrative()
{

}

ImGui::Node* Narrative::createNode(int type, const ImVec2& pos, const ImGui::NodeGraphEditor& editor)
{
	Narrative* instance = static_cast<Narrative*>(editor.user_ptr);
	const char* name = instance->m_nodeNames[type];
	TestNode* node = (TestNode*)ImGui::MemAlloc(sizeof(TestNode));
	IM_PLACEMENT_NEW(node) TestNode();

	switch (type)
	{
	case 0: // Start
		node->init(name, pos, "", "out", type);
		node->setOpen(false);
		break;
	case 1: // Finish
		node->init(name, pos, "in", "", type);
		node->setOpen(false);
		break;
	case 2: // Give
		node->init(name, pos, "in", "out", type);
		node->fields.addFieldEnum(node->newVar<int>(0), "Potion\0Sword\0Armor\0\0", "item");
		break;
	case 3: // Animation
		node->init(name, pos, "in", "out", type);
		node->fields.addFieldEnum(node->newVar<int>(0), "Animation1\0Animation2\0Animation3\0\0", "animation");
		break;
	case 4: // Dialogue
		node->init(name, pos, "in", "out", type);
		node->fields.addFieldEnum(node->newVar<int>(0), "LID_INTRO\0LID_BACKSTORY\0LID_GAME_MEME420\0\0", "animation");
		break;
	case 5: // Delay
		node->init(name, pos, "in", "out", type);
		node->fields.addField(node->newVar<float>(1.0f), 1, "delay", NULL, 3, 0, 999.0f);
		break;
	case 6: // Split
		node->init(name, pos, "in", "out1;out2;out3;out4;out5", type);
		using ImGui::FieldInfo;
		node->fields.addFieldCustom([](FieldInfo&) { ImGui::Dummy(ImVec2(100.0f, 20.0f)); return false; }, [](FieldInfo&, const FieldInfo&) { return false; }, nullptr);
		//typedef bool (*RenderFieldDelegate)(FieldInfo& field);
		//typedef bool (*CopyFieldDelegate)(FieldInfo& fdst, const FieldInfo& fsrc);
		break;
	case 7: // If
		node->init(name, pos, "in", "true;false", type);
		node->fields.addFieldEnum(node->newVar<int>(0), "var1\0var2\0var3\0\0", "A");
		node->fields.addFieldEnum(node->newVar<int>(0), "==\0!=\0<=\0>=\0<\0>\0\0", "operator");
		node->fields.addFieldEnum(node->newVar<int>(0), "var1\0var2\0var3\0\0", "B");
		break;
	}

	return node;
}

void Narrative::imgui()
{
	ImGui::Begin("Narrative");
		//ImGui::TestNodeGraphEditor();

	m_graph.render();

	ImGui::End();
}