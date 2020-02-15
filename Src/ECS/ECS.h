#pragma once

typedef unsigned int Entity;
typedef long long ComponentId;
typedef int SpriteId;

#define INVALID_ENTITY 0

template<typename Super>
struct Component
{
	Entity m_entity;
	static ComponentId componentId() { return reinterpret_cast<ComponentId>(&Super::m_cid); }
};

struct EmptyComponent : public Component<EmptyComponent>
{
	static constexpr const char* m_cid = "Empty";
};

struct PositionComponent : public Component<PositionComponent>
{
	static constexpr const char* m_cid = "Position";
	glm::vec3 m_position;
};

struct NameComponent : public Component<NameComponent>
{
	static constexpr const char* m_cid = "Name";
	char m_name[32];
};

struct SpriteComponent : public Component<SpriteComponent>
{
	static constexpr const char* m_cid = "Sprite";
	SpriteId m_sprite;
	float m_time;
};