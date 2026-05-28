/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <array>
#include <functional>
#include <vector>

#include "Map/MapDrawPassTypes.h"
#include "Map/MultiLayerHeightMap.h"

class CSMFGroundDrawer;
class CCamera;

/**
 * Manages per-layer mesh dirty state for the multi-layer terrain.
 *
 * Each of the 3 terrain layers owns a dirty flag.  When heights change in a
 * layer (via MultiLayerHeightMap::SetHeight), the caller must call
 * MarkLayerDirty() so the renderer knows to regenerate the affected meshes on
 * the next Update().
 */
class MultiLayerTerrainMesh {
public:
    static constexpr int NUM_LAYERS = MultiLayerHeightMap::NUM_LAYERS;

    explicit MultiLayerTerrainMesh(const MultiLayerHeightMap* heightMap);

    /**
     * Mark a layer's mesh as needing regeneration.
     * Called whenever SetHeight() is invoked on a particular layer.
     */
    void MarkLayerDirty(int layer);

    /** True if the layer mesh needs regeneration before the next draw. */
    bool IsMeshDirty(int layer) const;

    /**
     * Clear the dirty flag after the mesh has been regenerated.
     * Called by the renderer after it processes the update.
     */
    void ClearDirty(int layer);

    /**
     * Notify that heights in the given layer within the rectangle defined by
     * (x0,z0)–(x1,z1) have changed.  Stored so the renderer can do partial
     * updates rather than full mesh rebuilds.
     */
    void NotifyHeightChanged(int layer, int x0, int z0, int x1, int z1);

    struct DirtyRect { int x0, z0, x1, z1; };
    const DirtyRect& GetDirtyRect(int layer) const { return dirtyRects[layer]; }

    const MultiLayerHeightMap* GetHeightMap() const { return heightMap; }

private:
    const MultiLayerHeightMap* heightMap;
    std::array<bool,      NUM_LAYERS> dirtyFlags;
    std::array<DirtyRect, NUM_LAYERS> dirtyRects;
};
