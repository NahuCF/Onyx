#pragma once

#include "Core.h"

class Player
{
public:	
	Player(int32_t id, CommodityMap& commodities);

    void AddCommodity(CommodityType commodityType, uint32_t amount) { m_Commodities[commodityType] += amount; }
    void RemoveCommodity(CommodityType commodityType, uint32_t amount) { m_Commodities[commodityType] -= amount; }

    int32_t ID() const { return m_ID; }

	uint32_t GetCommodity(CommodityType commodity) { return m_Commodities[commodity]; }
private:
	CommodityMap m_Commodities;
	int32_t m_ID;
};

