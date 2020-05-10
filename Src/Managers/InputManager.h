#pragma once

#include "../Resources/ResourceManager.h"
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

protected:
	bool m_wantsTrayContext;
	int m_keys[256];
	bool m_hasFocus;
	float m_x, m_y;
	float m_prevX, m_prevY;
	float m_mouseWheel;
};