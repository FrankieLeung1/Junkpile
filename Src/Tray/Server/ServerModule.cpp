#include "stdafx.h"
#include "EmbeddableWebServer/EmbeddableWebServer.h"
#include "ServerModule.h"

#ifdef WIN32
#pragma comment(lib, "ws2_32") // link against Winsock on Windows
#endif

ServerModule::ServerModule():
m_thread(nullptr)
{
	
}

ServerModule::~ServerModule()
{

}

const char* ServerModule::getName() const
{
	return "Server";
}

void ServerModule::enable()
{
	if (m_thread)
		delete m_thread;

	m_thread = new ServerThread();
	m_thread->start();
}

void ServerModule::disable()
{
	serverStop(m_thread->m_server);
	m_thread->wait();
	delete m_thread;
	m_thread = nullptr;
}

struct Response* createResponseForRequest(const struct Request* request, struct Connection* connection)
{
	return responseAllocWithFormat(200, "OK", "text/html; charset=UTF-8", "Hi");
}

ServerModule::ServerThread::ServerThread():
m_server(nullptr)
{
	
}

ServerModule::ServerThread::~ServerThread()
{
	if(m_server)
		serverDeInit(m_server);
}

void ServerModule::ServerThread::run()
{
	assert(!m_server);
	m_server = new Server;
	memset(m_server, 0x00, sizeof(Server));

	uint16_t port = 8080;
	serverInit(m_server);
	acceptConnectionsUntilStoppedFromEverywhereIPv4(m_server, port);
}