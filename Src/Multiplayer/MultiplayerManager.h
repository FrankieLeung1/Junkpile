#pragma once

#include "../Resources/ResourceManager.h"
class MultiplayerManager : public SingletonResource<MultiplayerManager>
{
public:
	MultiplayerManager();
	~MultiplayerManager();

	void showServerList(bool);

	enum class Type { NotConnected, Host, Client};
	Type getType() const;

protected:
	std::string connect();
	std::string search();
	std::string create();
	std::string join();
	std::string leave();

	void startHost();
	void endHost();
	void startSearch();

	void update();

	void imgui();

protected:
	bool m_showServerList;

	struct Impl;
	std::shared_ptr<Impl> m_impl;
};