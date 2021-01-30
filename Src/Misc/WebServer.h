#pragma once

struct Server;
class WebServer
{
public:
	WebServer();
	virtual ~WebServer();

	static void test();
	void imgui();

	void enable();
	void disable();

protected:
	static void entry(WebServer*, int port);

protected:
	std::shared_ptr<std::thread> m_thread;

	std::mutex m_mutex;
	Server* m_server;

	int m_port;
	std::string m_body;

	Rendering::Texture* m_texture;

	friend struct Response* createResponseForRequest(const struct Request* request, struct Connection* connection);
};