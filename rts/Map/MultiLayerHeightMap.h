/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <array>
#include <string>
#include <vector>

#include "System/type2.h"

/**
 * Metadata describing a single terrain layer.
 * Mirrors the Lua mapinfo.lua `terrain.layers[i]` table.
 */
struct LayerInfo {
    std::string name;
    float baseHeight  = 0.0f;
    bool  deformable  = true;
    bool  active      = false; ///< true once at least one height has been set
};


/**
 * Stores three independent heightmaps and per-square type maps for a map of
 * given width × height.  Provides layer-aware get/set operations plus the
 * query API described in the Phase-1 roadmap.
 *
 * Indexing convention:
 *   layer 0 = Underground
 *   layer 1 = Surface  (equivalent to the classic single heightmap)
 *   layer 2 = Elevated
 *
 * Heights are stored as floats in world-space elmos, exactly as CReadMap uses
 * for its cornerHeightMap arrays.
 */
class MultiLayerHeightMap {
public:
    static constexpr int NUM_LAYERS = 3;

    /**
     * Construct an empty multi-layer heightmap.
     * @param width  Number of corner vertices in X  (mapx + 1)
     * @param height Number of corner vertices in Z  (mapy + 1)
     */
    MultiLayerHeightMap(int width, int height);

    // -----------------------------------------------------------------------
    // Core height access
    // -----------------------------------------------------------------------

    /// Returns the stored height at (x, z) on the given layer.
    /// Returns 0.0f for out-of-bounds or inactive-layer queries.
    float GetHeight(int x, int z, int layer) const;

    /// Writes height h at (x, z) on the given layer.
    /// Automatically marks the layer as active on first write.
    void  SetHeight(int x, int z, int layer, float h);

    // -----------------------------------------------------------------------
    // Layer query API
    // -----------------------------------------------------------------------

    /**
     * Returns a bitmask of which layers have been populated at (x, z).
     * Bit n is set when layer n has any data at all (layer-level granularity;
     * per-square activation tracking is not implemented to keep memory lean).
     * Bit 0 = Underground, bit 1 = Surface, bit 2 = Elevated.
     */
    uint8_t GetActiveLayerMask(int x, int z) const;

    /**
     * Returns {minHeight, maxHeight} across all active layers at (x, z).
     * If no layer is active the result is {0, 0}.
     */
    float2 GetHeightRange(int x, int z) const;

    /// Returns metadata for the given layer (name, baseHeight, deformable).
    const LayerInfo& GetLayerInfo(int layer) const;

    /// Overwrites the metadata for the given layer.
    void SetLayerInfo(int layer, const LayerInfo& info);

    /// Total number of layers (always NUM_LAYERS = 3).
    int GetLayerCount() const { return NUM_LAYERS; }

    /// True if the layer has had at least one SetHeight call.
    bool IsLayerActive(int layer) const;

    // -----------------------------------------------------------------------
    // Bilinear interpolation helper
    // -----------------------------------------------------------------------

    /**
     * Returns a bilinearly-interpolated height for fractional (x, z) on
     * the given layer.  x and z are in vertex space (0 … width-1 / height-1).
     * Falls back to nearest-neighbour for out-of-range input.
     */
    float GetInterpolatedHeight(float x, float z, int layer) const;

    // -----------------------------------------------------------------------
    // Dimensions
    // -----------------------------------------------------------------------

    int GetWidth()       const { return width;  }
    int GetMapHeight()   const { return height; }

private:
    int width;
    int height;

    std::array<std::vector<float>,   NUM_LAYERS> heightMaps;
    std::array<std::vector<uint8_t>, NUM_LAYERS> typeMaps;
    std::array<LayerInfo,            NUM_LAYERS> layerInfos;

    bool IsInBounds(int x, int z)  const { return (x >= 0 && x < width && z >= 0 && z < height); }
    int  GetIdx    (int x, int z)  const { return z * width + x; }
    bool IsValidLayer(int layer)   const { return (layer >= 0 && layer < NUM_LAYERS); }
};
