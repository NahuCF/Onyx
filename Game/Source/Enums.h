#pragma once

enum class CommodityType
{
	Food,
	Stone,
	Wood,
	Gold
};

enum class EntityType
{
	Villager
};

enum class ActionType
{
	Move,
	Attack,
	Gather
};

enum class MapSize
{
    Tiny = 72
};

enum class EventType
{
    Processing,
    Success
};

enum class TileStatus
{
    Unexplored,
    Explored,
    Seeing
};