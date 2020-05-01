#pragma once

#include "ComponentManager.h"
typedef unsigned int Entity;
typedef std::size_t ComponentId;
#define INVALID_ENTITY 0

template<typename T, typename UnderlyingType = unsigned int>
class Handle
{
public:
	void makeValid() { if (!(*this)) m_id = 0; }
	void advanceHandle() { CHECK_F(*this); m_id++; }
	T getAndAdvance() { T h = *(T*)(this); advanceHandle(); return h; }
	operator bool() const { return m_id != std::numeric_limits<UnderlyingType>::max(); }
	bool operator==(const Handle<T>& h) const { return m_id == h.m_id; }
	bool operator<(const Handle<T>& h) const { return m_id < h.m_id; }
	bool operator>(const Handle<T>& h) const { return m_id > h.m_id; }

private:
	UnderlyingType m_id{ std::numeric_limits<UnderlyingType>::max() };
};

template<typename Super>
struct Component
{
	Entity m_entity;
	static ComponentId componentId() { return typeid(Super).hash_code(); }
};

struct EmptyComponent : public Component<EmptyComponent>
{
	//static constexpr const char* m_cid = "Empty";
};

struct NameComponent : public Component<NameComponent>
{
	char m_name[32];
};