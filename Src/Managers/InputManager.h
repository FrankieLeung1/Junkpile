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

	bool isMouseDown(int, bool needsFocus = true) const;
	bool justMouseDown(int, bool needsFocus = true) const;
	bool isMouseReleased(int, bool needsFocus = true) const;
	bool justMouseReleased(int, bool needsFocus = true) const;
	bool isMouseDoubleClicked(int, bool needsFocus = true) const;
	void setMouseData(int button, bool down, bool justDown, bool justReleased, bool doubleClicked);

	bool wantsTrayContext() const;
	void setWantsTrayContext(bool);

	void setCursorPos(float x, float y);
	glm::vec2 getCursorPos() const;
	glm::vec2 getCursorPosDelta() const;

	glm::vec2 getCursorPosNDC() const;
	glm::vec2 getCursorPosDeltaNDC() const;

	glm::vec2 getCursorPosNormalizedInPixels() const;

	void setWindowSize(int width, int height);

	void setMouseWheel(float);
	float getMouseWheel() const;

	void emitEvents();

protected:
	bool m_wantsTrayContext;
	bool m_hasFocus;

	int m_keys[256];

	static const std::size_t m_buttonCount = 5;
	float m_x, m_y;
	float m_prevX, m_prevY;
	bool m_buttonDown[m_buttonCount];
	bool m_buttonJustDown[m_buttonCount], m_buttonJustReleased[m_buttonCount], m_buttonDoubleClicked[m_buttonCount];
	float m_mouseWheel;

	int m_width, m_height; // client area dimensions for calculating NDC
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