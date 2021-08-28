#include "stdafx.h"
#include "InputManager.h"
#include "EventManager.h"

InputManager::InputManager():
m_wantsTrayContext(false),
m_mouseWheel(0.0f),
m_width(1),
m_height(1)
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
		if (key >= 0) m_needsEmitHeld = true;
		if (key >= 0) key++;
		else if (key < 0) key--;
	}
}

void InputManager::setIsDown(int k, bool b)
{
	if (b && m_keys[k] < 0)
	{
		m_keys[k] = 0;
		m_needsEmitChanged = true;
	}
	else if (!b && m_keys[k] >= 0)
	{
		m_keys[k] = -1;
		m_needsEmitChanged = true;
	}
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

bool InputManager::isMouseDown(int button, bool needsFocus) const
{
	CHECK_F(button < m_buttonCount);
	return (!needsFocus || m_hasFocus) && m_buttonDown[button];
}

bool InputManager::justMouseDown(int button, bool needsFocus) const
{
	CHECK_F(button < m_buttonCount);
	return (!needsFocus || m_hasFocus) && m_buttonJustDown[button];
}

bool InputManager::isMouseReleased(int button, bool needsFocus) const
{
	return !isMouseDown(button, needsFocus);
}

bool InputManager::justMouseReleased(int button, bool needsFocus) const
{
	CHECK_F(button < m_buttonCount);
	return (!needsFocus || m_hasFocus) && m_buttonJustReleased[button];
}

bool InputManager::isMouseDoubleClicked(int button, bool needsFocus) const
{
	CHECK_F(button < m_buttonCount);
	return (!needsFocus || m_hasFocus) && m_buttonDoubleClicked[button];
}

void InputManager::setMouseData(int button, bool down, bool justDown, bool justReleased, bool doubleClicked)
{
	CHECK_F(button < m_buttonCount);
	m_buttonDown[button] = down;
	m_buttonJustDown[button] = justDown;
	m_buttonJustReleased[button] = justReleased;
	m_buttonDoubleClicked[button] = doubleClicked;
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

glm::vec2 InputManager::getCursorPosNDC() const
{
	// are NDCs scaled to the longest length?
	/*int longest = std::max(m_width, m_height);
	glm::vec2 cursor = getCursorPos();
	return { (cursor.x - m_width * 0.5f) / longest, (cursor.y - m_height * 0.5f) / longest };*/
	
	glm::vec2 cursor = getCursorPos();
	return { (cursor.x - m_width * 0.5f) / m_width, (cursor.y - m_height * 0.5f) / m_height };
}

glm::vec2 InputManager::getCursorPosDeltaNDC() const
{
	int longest = std::max(m_width, m_height);
	glm::vec2 cursor = getCursorPosDelta();
	return { (cursor.x - m_width * 0.5f) / longest, (cursor.y - m_height * 0.5f) / longest };
}

glm::vec2 InputManager::getCursorPosNormalizedInPixels() const
{
	glm::vec2 cursor = getCursorPos();
	return { (cursor.x - m_width * 0.5f), -(cursor.y - m_height * 0.5f) };
}

void InputManager::setWindowSize(int width, int height)
{
	m_width = width;
	m_height = height;
}

void InputManager::setMouseWheel(float f)
{
	m_mouseWheel = f;
}

float InputManager::getMouseWheel() const
{
	return m_mouseWheel;
}

void InputManager::emitEvents()
{
	if (m_needsEmitChanged || m_needsEmitHeld)
	{
		ResourcePtr<EventManager> e;
		if (m_needsEmitChanged)
			e->addOneFrameEvent<InputChanged>();
		if (m_needsEmitHeld)
			e->addOneFrameEvent<InputHeld>();

		m_needsEmitChanged = m_needsEmitHeld = false;
	}
}