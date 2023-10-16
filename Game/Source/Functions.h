#pragma once

#include <SE.h>

lptm::Vector2D PixelToNormalized(lptm::Vector2D size, lptm::Vector2D windowSize);

lptm::Vector2D NormalizedToPixel(lptm::Vector2D size, lptm::Vector2D windowSize);

// x;y to normalized
lptm::Vector2D ToScreen(int x, int y, lptm::Vector2D tileSize, lptm::Vector2D offset);

// Mouse pos to world x;y
lptm::Vector2D ToWorld(lptm::Vector2D normalizedPosition, lptm::Vector2D tileSize, lptm::Vector2D worldOffset);