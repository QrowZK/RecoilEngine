/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MultiLayerHeightMap.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <stdexcept>


// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

MultiLayerHeightMap::MultiLayerHeightMap(int w, int h)
    : width(w), height(h)
{
    assert(w > 0 && h > 0);

    const size_t numVerts = static_cast<size_t>(w) * h;
    const size_t numSquares = static_cast<size_t>((w > 1 ? w - 1 : 1)) * (h > 1 ? h - 1 : 1);

    for (int layer = 0; layer < NUM_LAYERS; ++layer) {
        heightMaps[layer].assign(numVerts, 0.0f);
        typeMaps  [layer].assign(numSquares, 0);
    }

    // Default layer metadata matching the roadmap example
    layerInfos[0] = LayerInfo{"Underground", -500.0f, false, false};
    layerInfos[1] = LayerInfo{"Surface",        0.0f, true,  false};
    layerInfos[2] = LayerInfo{"Elevated",      500.0f, false, false};
}


// ---------------------------------------------------------------------------
// Core height access
// ---------------------------------------------------------------------------

float MultiLayerHeightMap::GetHeight(int x, int z, int layer) const
{
    if (!IsValidLayer(layer) || !IsInBounds(x, z))
        return 0.0f;

    return heightMaps[layer][GetIdx(x, z)];
}

void MultiLayerHeightMap::SetHeight(int x, int z, int layer, float h)
{
    if (!IsValidLayer(layer) || !IsInBounds(x, z))
        return;

    heightMaps[layer][GetIdx(x, z)] = h;
    layerInfos[layer].active = true;
}


// ---------------------------------------------------------------------------
// Layer query API
// ---------------------------------------------------------------------------

uint8_t MultiLayerHeightMap::GetActiveLayerMask(int x, int z) const
{
    // Layer activation is tracked at layer granularity (not per-square) to
    // avoid the memory overhead of a per-vertex active flag per layer.
    uint8_t mask = 0;
    for (int layer = 0; layer < NUM_LAYERS; ++layer) {
        if (layerInfos[layer].active)
            mask |= static_cast<uint8_t>(1 << layer);
    }
    return mask;
}

float2 MultiLayerHeightMap::GetHeightRange(int x, int z) const
{
    if (!IsInBounds(x, z))
        return {0.0f, 0.0f};

    float minH = std::numeric_limits<float>::max();
    float maxH = std::numeric_limits<float>::lowest();
    bool  any  = false;

    for (int layer = 0; layer < NUM_LAYERS; ++layer) {
        if (!layerInfos[layer].active)
            continue;
        const float h = heightMaps[layer][GetIdx(x, z)];
        minH = std::min(minH, h);
        maxH = std::max(maxH, h);
        any = true;
    }

    return any ? float2{minH, maxH} : float2{0.0f, 0.0f};
}

const LayerInfo& MultiLayerHeightMap::GetLayerInfo(int layer) const
{
    assert(IsValidLayer(layer));
    return layerInfos[layer];
}

void MultiLayerHeightMap::SetLayerInfo(int layer, const LayerInfo& info)
{
    assert(IsValidLayer(layer));
    layerInfos[layer] = info;
}

bool MultiLayerHeightMap::IsLayerActive(int layer) const
{
    if (!IsValidLayer(layer))
        return false;
    return layerInfos[layer].active;
}


// ---------------------------------------------------------------------------
// Bilinear interpolation
// ---------------------------------------------------------------------------

float MultiLayerHeightMap::GetInterpolatedHeight(float fx, float fz, int layer) const
{
    if (!IsValidLayer(layer))
        return 0.0f;

    // Clamp to valid vertex range
    fx = std::clamp(fx, 0.0f, static_cast<float>(width  - 1));
    fz = std::clamp(fz, 0.0f, static_cast<float>(height - 1));

    const int x0 = static_cast<int>(fx);
    const int z0 = static_cast<int>(fz);
    const int x1 = std::min(x0 + 1, width  - 1);
    const int z1 = std::min(z0 + 1, height - 1);

    const float tx = fx - x0;
    const float tz = fz - z0;

    const auto& hm = heightMaps[layer];
    const float h00 = hm[GetIdx(x0, z0)];
    const float h10 = hm[GetIdx(x1, z0)];
    const float h01 = hm[GetIdx(x0, z1)];
    const float h11 = hm[GetIdx(x1, z1)];

    return (h00 * (1.0f - tx) + h10 * tx) * (1.0f - tz)
         + (h01 * (1.0f - tx) + h11 * tx) * tz;
}
