#pragma once

#include "../Resources/ResourceManager.h"

class ImGuiManager;
class TestManager : public SingletonResource<TestManager>
{
public:
	typedef std::function<void(float)> UpdateFn;
	typedef std::function<void()> RenderFn;

public:
	TestManager();
	~TestManager();

	void addTest(const char* name, void(*TestFn)());
	void addTest(const char* name, void(*TestFn)(UpdateFn&, RenderFn&));

protected:
	void update(float);
	void render();

	void imgui();
	void reset();

protected:
	ResourcePtr<ImGuiManager> m_imgui;
	std::map<const char*, std::function<void (UpdateFn&, RenderFn&)>> m_tests;

	struct Current
	{
		const char* m_name{ nullptr };
		UpdateFn m_update{ nullptr };
		RenderFn m_render{ nullptr };
	};
	Current m_current;
};