/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "Map/MultiLayerHeightMap.h"

/**
 * Per-layer sync state for multi-layer terrain.
 *
 * Each terrain layer carries its own CRC so that desyncs can be localised
 * to a specific layer rather than the whole heightmap.  The workflow mirrors
 * the existing CReadMap::CalcHeightmapChecksum() but operates independently
 * for each layer.
 *
 * Network protocol integration:
 *   On each frame the server (or a designated peer) computes per-layer CRCs
 *   and broadcasts them inside a standard NETMSG_SYNCRESPONSE extension.
 *   Clients compare their local CRCs; a mismatch triggers a desync report
 *   attributed to the specific layer.
 *
 * Memory layout:
 *   The class owns lightweight state only (CRCs + dirty flags).  The actual
 *   heightmap data lives in MultiLayerHeightMap.
 */
class MultiLayerSync {
public:
    static constexpr int NUM_LAYERS = MultiLayerHeightMap::NUM_LAYERS;

    explicit MultiLayerSync(const MultiLayerHeightMap* heightMap);

    // -----------------------------------------------------------------------
    // CRC management
    // -----------------------------------------------------------------------

    /**
     * Recompute the CRC for the given layer from the full heightmap.
     * Call this after any SetHeight() calls have been applied.
     */
    void UpdateLayerCRC(int layer);

    /** Update CRCs for all layers that have been marked dirty. */
    void UpdateDirtyCRCs();

    /** Return the current CRC for a layer (uint32_t, same format as mapChecksum). */
    uint32_t GetLayerCRC(int layer) const;

    /** Marks a layer dirty so its CRC is recomputed on the next UpdateDirtyCRCs(). */
    void MarkLayerDirty(int layer);

    /** True if the layer CRC needs recomputing. */
    bool IsLayerDirty(int layer) const;

    // -----------------------------------------------------------------------
    // Serialisation helpers (network packet payload)
    // -----------------------------------------------------------------------

    /**
     * Serialise all layer CRCs into an 8-byte-aligned byte buffer.
     * Format: [uint32_t crc0][uint32_t crc1][uint32_t crc2][uint32_t padding]
     * Total: 16 bytes (4 × sizeof(uint32_t)).
     */
    void SerializeCRCs(std::vector<uint8_t>& out) const;

    /**
     * Deserialise layer CRCs received from the network and compare against
     * our local CRCs.
     * @return bitmask of layers that are out-of-sync (0 = all good).
     */
    uint8_t CompareCRCs(const std::vector<uint8_t>& remotePayload) const;

    // -----------------------------------------------------------------------
    // Deformation helpers
    // -----------------------------------------------------------------------

    /**
     * Apply a terrain deformation (explosion crater) to a specific layer.
     * Height values in heightDelta[] are *added* to the existing heights.
     *
     * @param layer      Target layer (0-2)
     * @param x1,z1      Top-left corner of affected region (vertex coords)
     * @param x2,z2      Bottom-right corner
     * @param heightDelta Flat array [width*height] of height deltas to apply
     *                   where width = x2-x1+1, height = z2-z1+1
     */
    void ApplyLayerDeformation(int layer, int x1, int z1, int x2, int z2,
                               const std::vector<float>& heightDelta);

private:
    static uint32_t CRC32(const float* data, size_t count);

    const MultiLayerHeightMap* heightMap;
    std::array<uint32_t, NUM_LAYERS> layerCRCs;
    std::array<bool,     NUM_LAYERS> dirtyFlags;
};
