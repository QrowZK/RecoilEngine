/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MultiLayerTerrainMesh.h"

#include <algorithm>
#include <cassert>
#include <limits>


MultiLayerTerrainMesh::MultiLayerTerrainMesh(const MultiLayerHeightMap* hm)
    : heightMap(hm)
{
    assert(hm != nullptr);
    dirtyFlags.fill(false);

    // Initialise dirty rects to "invalid / empty"
    const DirtyRect empty{
        std::numeric_limits<int>::max(),
        std::numeric_limits<int>::max(),
        std::numeric_limits<int>::min(),
        std::numeric_limits<int>::min()
    };
    dirtyRects.fill(empty);
}


void MultiLayerTerrainMesh::MarkLayerDirty(int layer)
{
    assert(layer >= 0 && layer < NUM_LAYERS);
    dirtyFlags[layer] = true;
}


bool MultiLayerTerrainMesh::IsMeshDirty(int layer) const
{
    assert(layer >= 0 && layer < NUM_LAYERS);
    return dirtyFlags[layer];
}


void MultiLayerTerrainMesh::ClearDirty(int layer)
{
    assert(layer >= 0 && layer < NUM_LAYERS);
    dirtyFlags[layer] = false;

    const DirtyRect empty{
        std::numeric_limits<int>::max(),
        std::numeric_limits<int>::max(),
        std::numeric_limits<int>::min(),
        std::numeric_limits<int>::min()
    };
    dirtyRects[layer] = empty;
}


void MultiLayerTerrainMesh::NotifyHeightChanged(int layer, int x0, int z0, int x1, int z1)
{
    assert(layer >= 0 && layer < NUM_LAYERS);
    DirtyRect& r = dirtyRects[layer];

    r.x0 = std::min(r.x0, x0);
    r.z0 = std::min(r.z0, z0);
    r.x1 = std::max(r.x1, x1);
    r.z1 = std::max(r.z1, z1);

    dirtyFlags[layer] = true;
}
