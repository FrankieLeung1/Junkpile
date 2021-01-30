#pragma once

struct tm;
class Analytics
{
public:
	Analytics();
	~Analytics();

	void imgui();

protected:
	void updateGraphData();
	void setToMidnight(tm*) const;

protected:
#define GenerateItems(fn) \
fn(BASIC_ARROW) \
fn(GOLD_ARROW) \
fn(CHILL_ARROW) \
fn(FURY_ARROW) \
fn(GOLD_FURY_ARROW) \
fn(POISON_ARROW) \
fn(ICE_ARROW) \
fn(FIRE_ARROW) \
fn(SHUFFLE_ARROW) \
fn(STONEBREAKER_ARROW) \
fn(DUSTER_ARROW) \
fn(CUPID_ARROW) \
fn(BASIC_BOOTS) \
fn(GOLD_BOOTS) \
fn(QUAKE_BOOTS) \
fn(POTION) \
fn(PHOENIX_POTION) \
fn(FIRE_POTION) \
fn(QUAKE_POTION) \
fn(PAINT_POTION) \
fn(BOMB_POTION) \
fn(WEAK_POTION) \
fn(WOODEN_SHIELD) \
fn(GOLD_SHIELD) \
fn(SPIKE_SHIELD) \
fn(POISON_SHIELD) \
fn(BONE_SHIELD) \
fn(GRINDSTONE_SHIELD) \
fn(SPIN_SWORD) \
fn(GOLD_SPIN_SWORD) \
fn(SLASH_SWORD) \
fn(GOLD_SLASH_SWORD) \
fn(DASH_SWORD) \
fn(GOLD_DASH_SWORD) \
fn(ZAMBONI_SWORD) \
fn(FIRE_SWORD) \
fn(HAUNTED_SWORD) \
fn(GEM_DESTROY_SWORD) \
fn(VINE_SWORD) \
fn(GEM_DUPLICATOR_SWORD) \
fn(BOOMERANG_SWORD) \
fn(HEAVY_SWORD) \
fn(PICKAXE)

#define GenerateArmour(fn) \
fn(CROWN) \
fn(BUTCHER_APRON) \
fn(POISON_RESIST) \
fn(TREASURE_HAT) \
fn(FIRE_COSTUME) \
fn(TOUGH_GUY_SHIRT) \
fn(SKELLY_SUIT) \
fn(SALAD_HELMET) \
fn(BOSSY_PANTS) \
fn(LEATHER_DADDY) \
fn(ICE_SWEATER) \
fn(TEMPLE_HEAD_DRESS) \
fn(ARCHER) \
fn(SANTA) \
fn(BLUE_CREEP_SUMMONER) \
fn(GREEN_CREEP_SUMMONER) \
fn(ORANGE_CREEP_SUMMONER) \
fn(PURPLE_CREEP_SUMMONER) \
fn(RED_CREEP_SUMMONER) \
fn(BOUNCER) \
fn(FIRE_EXTINGUISH) \
fn(CUPID_SUIT) \
fn(GOLD_SUIT) \
fn(PROSPECTOR_SUIT) \
fn(KRAMPUS_SUIT)

#define GenerateSnax(fn) \
fn(THIEF_RELIEVER) \
fn(MYSTIC_SUMMONER) \
fn(CREEP_SALAD) \
fn(EXTINCTION_EVENT)

#define CREATE_ENUM(name) name,
#define CREATE_STRINGS(name) #name,

	enum Item { GenerateItems(CREATE_ENUM) ItemTotal };
	const char* itemStrings[Item::ItemTotal] = { GenerateItems(CREATE_STRINGS) };

	enum Armour { GenerateArmour(CREATE_ENUM) ArmourTotal };
	const char* armourStrings[Armour::ArmourTotal] = { GenerateArmour(CREATE_STRINGS) };

	enum Snax { GenerateSnax(CREATE_ENUM) SnaxTotal };
	const char* snaxString[Snax::SnaxTotal] = { GenerateSnax(CREATE_STRINGS) };

    tm m_startTime, m_endTime;
	bool m_singleDate;
	struct Run
	{
		time_t m_time;
		float m_count;
		float m_average;
		float m_dgCount;

		float m_iOSCount;
		float m_switchCount;

		Item m_items[3];
		Armour m_armour;
		Snax m_snax;
	};
	std::vector<Run> m_runs;

	// imgui
	int m_currentGraphType;
	std::vector<float> m_runsPlotX;
	std::vector<float> m_iosPlot, m_switchPlot;
	std::vector<float> m_runsPlotY;
	std::vector<float> m_dgPlotY;
	std::vector<float> m_averagePlotY;
	std::vector<float> m_itemsPlot;
	std::vector<float> m_armourPlot;
	std::vector<float> m_snax;
};