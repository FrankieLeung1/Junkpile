#pragma once

#include "../Resources/ResourceManager.h"
#include "../ECS/ComponentManager.h"
namespace Rendering
{
	class Texture;
}
class TicTacToeSystem : public SingletonResource<TicTacToeSystem>
{
public:
	TicTacToeSystem();
	~TicTacToeSystem();

	void imgui();
	void update();

	struct Settings
	{
		glm::vec3 m_colour[2];
		bool m_grid[5][5];
	};

protected:
	ResourcePtr<Rendering::Texture> createTexture() const;

protected:
	Entity m_component;
};

struct Entity;
struct TicTacToeComponent : public Component<TicTacToeComponent>
{
	enum class State { Init, Reset, Turn, EndGame };
	enum class Player { Player1 = 0, Player2, PlayerCount};
	static const std::size_t MaxPiecesPerPlayer = 9;

	// runtime
	//Entity m_pieces[MaxPiecesPerPlayer * (std::size_t)Player::PlayerCount];
	Entity m_pieces[9];
	State m_state;
	Player m_currentTurn;
	Entity m_dragPiece;

	TicTacToeComponent();
};