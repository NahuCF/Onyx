#include "Player.h"


Player::Player(int32_t id, CommodityMap& commodities)
	: m_ID(id)
{
    // Set commodities for player
	for (const std::pair<CommodityType, int>& t : commodities)
		m_Commodities[t.first] = t.second;
}