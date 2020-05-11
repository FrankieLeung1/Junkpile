#pragma once

#include "../Resources/ResourceManager.h"
#include "../Misc/Any.h"
class DebugManager : public SingletonResource<DebugManager>
{
public:
	DebugManager();
	~DebugManager();

	void debug(float*, const char* name, const char* file);
	void debug(int*, const char* name, const char* file);
	void debug(std::string*, const char* name, const char* file);
	void debug(glm::vec2*, const char* name, const char* file);
	void debug(glm::vec3*, const char* name, const char* file);

	void debug(float, const char* name, const char* file);
	void debug(int, const char* name, const char* file);
	void debug(const std::string&, const char* name, const char* file);
	void debug(const glm::vec2&, const char* name, const char* file);
	void debug(const glm::vec3&, const char* name, const char* file);

protected:
	void imgui();

protected:
	struct Var
	{
		template<typename T>
		Var(T v, const char* m_name);
		const char *m_name;
		Any m_value;
	};
	std::map< const char*, std::vector<Var> > m_variables;
};

#define DEBUG_VAR(name, value) { ResourcePtr<DebugManager>()->debug(value, name, __FILE__); }