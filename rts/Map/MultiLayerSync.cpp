/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MultiLayerSync.h"

#include <algorithm>
#include <cassert>
#include <cstring>


// ---------------------------------------------------------------------------
// Simple CRC-32 (poly 0xEDB88320, table-free for brevity)
// ---------------------------------------------------------------------------

uint32_t MultiLayerSync::CRC32(const float* data, size_t count)
{
    uint32_t crc = 0xFFFFFFFFu;
    const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
    const size_t bytes = count * sizeof(float);

    for (size_t i = 0; i < bytes; ++i) {
        crc ^= p[i];
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xEDB88320u & -(crc & 1u));
    }
    return crc ^ 0xFFFFFFFFu;
}


// ---------------------------------------------------------------------------

MultiLayerSync::MultiLayerSync(const MultiLayerHeightMap* hm)
    : heightMap(hm)
{
    assert(hm != nullptr);
    layerCRCs.fill(0);
    dirtyFlags.fill(true); // force initial computation
}


// ---------------------------------------------------------------------------
// CRC management
// ---------------------------------------------------------------------------

void MultiLayerSync::UpdateLayerCRC(int layer)
{
    assert(layer >= 0 && layer < NUM_LAYERS);

    if (!heightMap->IsLayerActive(layer)) {
        layerCRCs[layer] = 0;
        dirtyFlags[layer] = false;
        return;
    }

    // Accumulate CRC row-by-row using GetHeight() to stay abstracted from
    // the internal storage layout.
    const int w = heightMap->GetWidth();
    const int h = heightMap->GetMapHeight();
    std::vector<float> scratch(static_cast<size_t>(w) * h);

    for (int z = 0; z < h; ++z)
        for (int x = 0; x < w; ++x)
            scratch[z * w + x] = heightMap->GetHeight(x, z, layer);

    layerCRCs[layer]  = CRC32(scratch.data(), scratch.size());
    dirtyFlags[layer] = false;
}


void MultiLayerSync::UpdateDirtyCRCs()
{
    for (int layer = 0; layer < NUM_LAYERS; ++layer) {
        if (dirtyFlags[layer])
            UpdateLayerCRC(layer);
    }
}


uint32_t MultiLayerSync::GetLayerCRC(int layer) const
{
    assert(layer >= 0 && layer < NUM_LAYERS);
    return layerCRCs[layer];
}


void MultiLayerSync::MarkLayerDirty(int layer)
{
    assert(layer >= 0 && layer < NUM_LAYERS);
    dirtyFlags[layer] = true;
}


bool MultiLayerSync::IsLayerDirty(int layer) const
{
    assert(layer >= 0 && layer < NUM_LAYERS);
    return dirtyFlags[layer];
}


// ---------------------------------------------------------------------------
// Serialisation
// ---------------------------------------------------------------------------

void MultiLayerSync::SerializeCRCs(std::vector<uint8_t>& out) const
{
    // 3 layer CRCs + 1 padding word = 16 bytes
    out.resize(16, 0);
    for (int l = 0; l < NUM_LAYERS; ++l) {
        const uint32_t crc = layerCRCs[l];
        std::memcpy(out.data() + l * 4, &crc, sizeof(uint32_t));
    }
}


uint8_t MultiLayerSync::CompareCRCs(const std::vector<uint8_t>& remotePayload) const
{
    if (remotePayload.size() < static_cast<size_t>(NUM_LAYERS) * 4)
        return 0xFF; // corrupt packet → flag all layers

    uint8_t desynced = 0;
    for (int l = 0; l < NUM_LAYERS; ++l) {
        uint32_t remoteCRC = 0;
        std::memcpy(&remoteCRC, remotePayload.data() + l * 4, sizeof(uint32_t));
        if (remoteCRC != layerCRCs[l])
            desynced |= static_cast<uint8_t>(1u << l);
    }
    return desynced;
}


// ---------------------------------------------------------------------------
// Deformation
// ---------------------------------------------------------------------------

void MultiLayerSync::ApplyLayerDeformation(int layer, int x1, int z1,
                                           int x2, int z2,
                                           const std::vector<float>& heightDelta)
{
    assert(layer >= 0 && layer < NUM_LAYERS);

    const int w = x2 - x1 + 1;
    const int h = z2 - z1 + 1;
    assert(static_cast<int>(heightDelta.size()) >= w * h);

    // Only deform layers that are marked deformable in their LayerInfo
    if (!heightMap->GetLayerInfo(layer).deformable)
        return;

    // We need a non-const heightmap pointer here.  In the real engine this
    // would be obtained from the global readMap; for the Phase-4 stub we
    // work through the const accessor and note that actual modification
    // must go through CReadMap::AddHeight() in production code.
    // This function documents the intended interface and is fully testable
    // once a non-const accessor is wired up.

    dirtyFlags[layer] = true; // CRC is stale after deformation
}
