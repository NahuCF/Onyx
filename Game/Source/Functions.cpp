#include "Functions.h"

lptm::Vector2D PixelToNormalized(lptm::Vector2D size, lptm::Vector2D windowSize)
{
	return lptm::Vector2D(
		size.x / windowSize.x,
		size.y / windowSize.y
	);
}

lptm::Vector2D NormalizedToPixel(lptm::Vector2D size, lptm::Vector2D windowSize)
{
	return lptm::Vector2D(
		size.x * windowSize.x,
		size.y * windowSize.y
	);
}

// x;y to normalized
lptm::Vector2D ToScreen(int x, int y, lptm::Vector2D tileSize, lptm::Vector2D offset)
{
	return lptm::Vector2D(
		(x - y) * (tileSize.x / 2) + offset.x,
		(x + y) * (tileSize.y / 2) + offset.y
	);
};

// Mouse pos to world x;y
// Tilesize = Pixels
// worldoffeset in pixels
lptm::Vector2D ToWorld(lptm::Vector2D normalizedPosition, lptm::Vector2D tileSize, lptm::Vector2D worldOffset)
{
	normalizedPosition.x -= worldOffset.x;
	normalizedPosition.y -= worldOffset.y;

	return lptm::Vector2D(
		((normalizedPosition.x + (2 * normalizedPosition.y) - (tileSize.x / 2)) / tileSize.x),
		((-normalizedPosition.x + (2 * normalizedPosition.y) + (tileSize.x / 2)) / tileSize.x)
	);
};

uint32_t WorldToIndex(lptm::Vector2D tilePos, lptm::Vector2D worldSize)
{
    return tilePos.y * worldSize.x + tilePos.x;
}