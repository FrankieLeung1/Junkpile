#pragma once

#include "../Resources/ResourceManager.h"
#include "../Misc/Any.h"

struct RenderEvent;
namespace Rendering { class Shader; class Buffer; struct ColouredVert; }
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
	void debug(glm::vec4*, const char* name, const char* file);
	void debug(glm::mat4*, const char* name, const char* file);

	void debug(float, const char* name, const char* file);
	void debug(int, const char* name, const char* file);
	void debug(const std::string&, const char* name, const char* file);
	void debug(const glm::vec2&, const char* name, const char* file);
	void debug(const glm::vec3&, const char* name, const char* file);
	void debug(const glm::vec4&, const char* name, const char* file);
	void debug(const glm::mat4&, const char* name, const char* file);
	void setIncrement(float);

	void addLine3D(const glm::vec4&, const glm::vec4&, const glm::vec4&);

protected:
	void imgui();
	void render(RenderEvent*);
	void ensureBufferSizes(std::size_t vSize, std::size_t iSize);

protected:
	ResourcePtr<Rendering::Shader> m_vertexShader, m_fragmentShader;
	std::shared_ptr<Rendering::Buffer> m_vertexBuffer, m_indexBuffer;

	typedef Rendering::ColouredVert Vertex;
	struct Line
	{
		glm::vec3 m_positions[2];
		glm::vec4 m_colour;
	};
	std::vector<Line> m_lines;

protected:
	struct Var
	{
		template<typename T> Var(T v, const char* name, float increment);
		const char *m_name;
		float m_increment;
		Any m_value;
	};
	std::map< const char*, std::vector<Var> > m_variables;
	float m_increment;
};

#define DEBUG_INC(increment) { ResourcePtr<DebugManager>()->setIncrement(increment); }
#define DEBUG_VAR(name, value) { ResourcePtr<DebugManager>()->debug(value, name, __FILE__); }