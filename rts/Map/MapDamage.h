/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _MAP_DAMAGE_H_
#define _MAP_DAMAGE_H_

#include "System/float3.h"

#include <cstdint>

class IMapDamage
{
public:
	static IMapDamage* InitMapDamage();
	static void FreeMapDamage(IMapDamage*);

public:
	virtual ~IMapDamage() {}

	virtual void Explosion(const float3& pos, float strength, float radius, float& maxHeightDiff) = 0;

	/**
	 * Recalculate all derived terrain data (normals, slope, LOS, pathfinding)
	 * for the region (x1,y1)-(x2,y2).
	 *
	 * @param layerMask  Bitmask of which terrain layers were modified
	 *                   (bit 0 = Underground, bit 1 = Surface, bit 2 = Elevated).
	 *                   Pass 0xFF to invalidate all layers (default, backward-compatible).
	 */
	virtual void RecalcArea(int x1, int x2, int y1, int y2, uint8_t layerMask = 0xFF) = 0;

	virtual void TerrainTypeHardnessChanged(int ttIndex) {}
	virtual void TerrainTypeSpeedModChanged(int ttIndex) {}

	virtual void Init() = 0;
	virtual void Update() = 0;

	virtual bool Disabled() const = 0;

	float mapHardness = 0.0f;
};

extern IMapDamage* mapDamage;

#endif // _MAP_DAMAGE_H_
