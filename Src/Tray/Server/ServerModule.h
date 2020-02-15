#pragma once

#include "../ModuleInterface.h"

class Server;
class ServerModule : public ModuleInterface
{
public:
	ServerModule();
	virtual ~ServerModule();

	const char* getName() const;

	void enable();
	void disable();

protected:
	class ServerThread;
	friend class ServerThread;

	ServerThread* m_thread;
};

class ServerModule::ServerThread : public QThread
{
public:
	ServerThread();
	virtual ~ServerThread();

	Server* m_server;

protected:
	void run();
};