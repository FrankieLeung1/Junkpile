#include "stdafx.h"
#include "WebServer.h"
#include "../Misc/Misc.h"
#include "../imgui/ImGuiManager.h"

#pragma warning (push)
#pragma warning( disable : 4267 )
#include "EmbeddableWebServer/EmbeddableWebServer.h"
#pragma warning ( pop )

#ifdef WIN32
#pragma comment(lib, "ws2_32") // link against Winsock on Windows
#endif

WebServer::WebServer():
m_thread(nullptr),
m_port(80),
m_body("<h1> TEST HTML </h1>")
{
}

WebServer::~WebServer()
{
	disable();
}

void WebServer::test()
{
	WebServer* server = createTestResource<WebServer>();
	ResourcePtr<ImGuiManager> m;
	m->registerCallback({ [](void* ud) {
		WebServer* server = static_cast<WebServer*>(ud);

		ImGui::Begin("WebServer");
		ImGui::InputTextMultiline("HTML", &server->m_body, ImVec2(-1, -20.0f));
		bool started = (server->m_thread != nullptr);
		if (ImGui::Button(!started ? "Start" : "Stop"))
		{
			if (started) server->disable();
			else server->enable();
		}
		ImGui::SameLine();

		if (started)
		{
			if (ImGui::Button("Visit"))
				ShellExecute(NULL, L"open", L"http://localhost/", NULL, NULL, SW_SHOWNORMAL);

			ImGui::SameLine();
		}
		ImGui::InputInt("port", &server->m_port, 1, 100, started ? ImGuiInputTextFlags_ReadOnly : 0);
		ImGui::End();

	}, server });
}

void WebServer::enable()
{
	if (m_thread && m_server)
		return;

	m_thread = std::make_shared<std::thread>(&WebServer::entry, this, m_port);

	m_server = new Server{ 0 };
	serverInit(m_server);
	m_server->tag = this;
}

void WebServer::disable()
{
	if (!m_thread)
		return;

	if (m_server)
	{
		serverStop(m_server);
		serverDeInit(m_server);
		m_server = nullptr;
	}
	// BUG: joining here seems to block forever
	m_thread->detach();
	m_thread = nullptr;
}

struct Response* createResponseForRequest(const struct Request* request, struct Connection* connection)
{
	WebServer* instance = static_cast<WebServer*>(connection->server->tag);
	std::lock_guard<std::mutex> l(instance->m_mutex);
	return responseAllocWithFormat(200, "OK", "text/html; charset=UTF-8", instance->m_body.c_str());
}

void WebServer::entry(WebServer* instance, int port)
{
	acceptConnectionsUntilStoppedFromEverywhereIPv4(instance->m_server, port);
}