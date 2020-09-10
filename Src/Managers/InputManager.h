#pragma once

#include "../Resources/ResourceManager.h"
#include "../Managers/EventManager.h"
struct InputChanged : public Event<InputChanged>
{

};

struct InputHeld : public Event<InputHeld>
{

};

class InputManager : public SingletonResource<InputManager>
{
public:
	InputManager();
	~InputManager();

	void update();
	void setIsDown(int, bool);
	void setHasFocus(bool);

	bool isDown(int, bool needsFocus = true) const;
	bool justDown(int, bool needsFocus = true) const;
	bool isReleased(int, bool needsFocus = true) const;
	bool justReleased(int, bool needsFocus = true) const;

	bool wantsTrayContext() const;
	void setWantsTrayContext(bool);

	void setCursorPos(float x, float y);
	glm::vec2 getCursorPos() const;
	glm::vec2 getCursorPosDelta() const;

	void setMouseWheel(float);
	float getMouseWheel() const;

	void emitEvents();

protected:
	bool m_wantsTrayContext;
	int m_keys[256];
	bool m_hasFocus;
	float m_x, m_y;
	float m_prevX, m_prevY;
	float m_mouseWheel;
	bool m_needsEmitChanged, m_needsEmitHeld;
};

namespace Meta
{
	template<>
	inline Object instanceMeta<InputManager>()
	{
		return Object("InputManager").
			func("isDown", &InputManager::isDown, { "key", "needsFocus" }, { true }).
			func("justDown", &InputManager::justDown, { "key", "needsFocus" }, { true }).
			func("isReleased", &InputManager::isReleased, { "key", "needsFocus" }, { true }).
			func("justReleased", &InputManager::justReleased, { "key", "needsFocus" }, { true }).
			func("getCursorPos", &InputManager::getCursorPos).
			func("getCursorPosDelta", &InputManager::getCursorPosDelta);
	}
	template<> inline Object instanceMeta<InputChanged>() { return Object("InputChanged"); }
	template<> inline Object instanceMeta<InputHeld>() { return Object("InputHeld"); }
}