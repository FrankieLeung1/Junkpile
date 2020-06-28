#include "stdafx.h"
#include "InputManager.h"
#include "EventManager.h"

InputManager::InputManager():
m_wantsTrayContext(false),
m_mouseWheel(0.0f)
{
	std::fill(m_keys, m_keys + countof(m_keys), -std::numeric_limits<int>::max() / 2);

	ResourcePtr<EventManager> events;
	events->addListener<UpdateEvent>([this](UpdateEvent*) { update(); }, 13);
}

InputManager::~InputManager()
{
	
}

void InputManager::update()
{
	for (auto& key : m_keys)
	{
		if (key >= 0) key++;
		else if (key < 0) key--;
	}
}

void InputManager::setIsDown(int k, bool b)
{
	if (b && m_keys[k] < 0)
		m_keys[k] = 0;
	else if (!b && m_keys[k] >= 0)
		m_keys[k] = -1;
}

void InputManager::setHasFocus(bool b)
{
	m_hasFocus = b;
}

bool InputManager::isDown(int k, bool needsFocus) const
{
	//if (k >= 'a' && k <= 'z') k = toupper(k);
	return (!needsFocus || m_hasFocus) && m_keys[k] >= 0;
}

bool InputManager::justDown(int k, bool needsFocus) const
{
	//if (k >= 'a' && k <= 'z') k = toupper(k);
	return (!needsFocus || m_hasFocus) && m_keys[k] == 0;
}

bool InputManager::isReleased(int k, bool needsFocus) const
{
	//if (k >= 'a' && k <= 'z') k = toupper(k);
	return (!needsFocus || m_hasFocus) && m_keys[k] < 0;
}

bool InputManager::justReleased(int k, bool needsFocus) const
{
	//if (k >= 'a' && k <= 'z') k = toupper(k);
	return (!needsFocus || m_hasFocus) && m_keys[k] == -1;
}

bool InputManager::wantsTrayContext() const
{
	return m_wantsTrayContext;
}

void InputManager::setWantsTrayContext(bool b)
{
	m_wantsTrayContext = b;
}

void InputManager::setCursorPos(float x, float y)
{
	m_prevX = m_x;
	m_prevY = m_y;
	m_x = x;
	m_y = y;
}

glm::vec2 InputManager::getCursorPos() const
{
	return { m_x, m_y };
}

glm::vec2 InputManager::getCursorPosDelta() const
{
	return { m_prevX - m_x, m_prevY - m_y };
}

void InputManager::setMouseWheel(float f)
{
	m_mouseWheel = f;
}

float InputManager::getMouseWheel() const
{
	return m_mouseWheel;
}