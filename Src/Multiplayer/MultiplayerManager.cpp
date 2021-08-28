#include "stdafx.h"

#include "MultiplayerManager.h"
#include "../imgui/ImGuiManager.h"
#include "../Managers/EventManager.h"
#include "../Threading/ThreadPool.h"
#include "Credentials.h"

#define USE_IMPORT_EXPORT
#include "aws/core/Aws.h"
#include "aws/gamelift/GameLiftClient.h"
#include "aws/gamelift/model/SearchGameSessionsRequest.h"
#include "aws/gamelift/model/CreatePlayerSessionRequest.h"
#include "aws/gamelift/model/CreateGameSessionRequest.h"
#include "aws/core/auth/AwsCredentials.h"

struct MultiplayerManager::Impl
{
	enum class State{ Idle, Connecting, Searching, Creating, Joining };
	std::shared_ptr<Aws::GameLift::GameLiftClient> m_client;
	State m_state;
	std::future<std::string> m_future;
	bool m_hosting{ false };
	const char* m_fleetId{ "fleet-b0284404-8873-49ed-bf6b-aeebf788cdaf" };
	int m_playerId{ std::rand() };
	std::shared_ptr<Aws::GameLift::Model::GameSession> m_session; // the game session we're currently in
	std::string m_error;
};

MultiplayerManager::MultiplayerManager() :
m_showServerList(false),
m_impl(new Impl())
{
	ResourcePtr<EventManager> events;
	events->addListener<ImGuiRenderEvent>([this](ImGuiRenderEvent*) { imgui(); });

	ResourcePtr<ThreadPool> pool;
	m_impl->m_state = Impl::State::Connecting;
	m_impl->m_future = pool->enqueue([this]() { return connect(); });
}

MultiplayerManager::~MultiplayerManager()
{
	Aws::ShutdownAPI(Aws::SDKOptions{});
}

void MultiplayerManager::showServerList(bool b)
{
	m_showServerList = b;
}

MultiplayerManager::Type MultiplayerManager::getType() const
{
	return MultiplayerManager::Type::NotConnected;
}

void MultiplayerManager::update()
{
	if (m_impl->m_state == Impl::State::Idle)
		return;

	CHECK_F(m_impl->m_future.valid());
	if (m_impl->m_future.wait_for(std::chrono::nanoseconds(0)) != std::future_status::ready)
		return;

	m_impl->m_error = m_impl->m_future.get();

	if (m_impl->m_state == Impl::State::Connecting)
	{
		m_impl->m_state = Impl::State::Idle;
		startSearch();
	}

	m_impl->m_state = Impl::State::Idle;
}

void MultiplayerManager::startHost()
{
	ResourcePtr<ThreadPool> pool;
	m_impl->m_state = Impl::State::Creating;
	m_impl->m_future = pool->enqueue([this]() { return create(); });
}

void MultiplayerManager::endHost()
{
	//SetStatus
	if (m_impl->m_session == nullptr)
		return;

	m_impl->m_session->SetStatus(Aws::GameLift::Model::GameSessionStatus::TERMINATED);
	m_impl->m_session = nullptr;
}

void MultiplayerManager::startSearch()
{
	if (m_impl->m_state != Impl::State::Idle)
		return;

	ResourcePtr<ThreadPool> pool;
	m_impl->m_state = Impl::State::Searching;
	m_impl->m_future = pool->enqueue([this]() { return search(); });
}

std::string MultiplayerManager::connect()
{
	Aws::SDKOptions options;
	Aws::InitAPI(options);
	Aws::Auth::AWSCredentials credentials(AWSAccessKeyId, AWSSecretKey);
	Aws::Client::ClientConfiguration config;
	m_impl->m_client = std::make_shared<Aws::GameLift::GameLiftClient>(credentials, config);
	return "";
}

std::string MultiplayerManager::search()
{
	Aws::GameLift::Model::SearchGameSessionsRequest request;
	request.SetFleetId(m_impl->m_fleetId);
	Aws::GameLift::Model::SearchGameSessionsOutcome games = m_impl->m_client->SearchGameSessions(request);
	return games.IsSuccess() ? "" : games.GetError().GetMessage();
}

std::string MultiplayerManager::create()
{
	Aws::GameLift::Model::CreateGameSessionRequest request;
	request.SetFleetId(m_impl->m_fleetId);
	request.SetName("Test Game 1");
	request.SetCreatorId(stringf("%d", m_impl->m_playerId));
	request.SetMaximumPlayerSessionCount(2);
	Aws::GameLift::Model::CreateGameSessionOutcome game = m_impl->m_client->CreateGameSession(request);
	if (game.IsSuccess())
	{
		m_impl->m_session = std::make_shared<Aws::GameLift::Model::GameSession>(game.GetResult().GetGameSession());
		return join();
	}
	else
	{
		return game.GetError().GetMessage();
	}
}

std::string MultiplayerManager::join()
{
	Aws::GameLift::Model::CreatePlayerSessionRequest request;
	request.SetGameSessionId(m_impl->m_session->GetGameSessionId());
	request.SetPlayerId(stringf("%d", m_impl->m_playerId));
	Aws::GameLift::Model::CreatePlayerSessionOutcome session = m_impl->m_client->CreatePlayerSession(request);
	if (session.IsSuccess())
	{
		return "";
	}
	else
	{
		return session.GetError().GetMessage();
	}
}

std::string MultiplayerManager::leave()
{
	return "";
}

void MultiplayerManager::imgui()
{
	update();

	typedef MultiplayerManager::Impl::State State;
	ImGui::Begin("ServerList");

	ImGui::Text("PlayerId: %d", m_impl->m_playerId);
	switch (m_impl->m_state)
	{
	case State::Idle:
		if (ImGui::Button("Host")) startHost();
		if (ImGui::Button("Refresh")) startSearch();
		break;
	case State::Connecting: ImGui::Text("Connecting"); break;
	case State::Searching: ImGui::Text("Searching"); break;
	case State::Creating: ImGui::Text("Creating"); break;
	case State::Joining: ImGui::Text("Joining"); break;
	}

	if (!m_impl->m_error.empty())
		ImGui::Text(m_impl->m_error.c_str());

	ImGui::End();
}