/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "System/type2.h"
#include <cstdint>
#include <string>

/**
 * Abstract terrain system interface.
 *
 * Both the classic single-layer CReadMap and the new MultiLayerHeightMap
 * implement this contract, enabling code paths to operate on either without
 * knowing which concrete type is active.
 *
 * Coordinate convention:
 *   - (x, z) are vertex-grid indices in [0, mapx] × [0, mapy]
 *   - Layer 0 = Underground, 1 = Surface, 2 = Elevated
 *   - Single-layer implementations always report layer 1 as the only active layer
 */
struct ITerrainSystem {
    virtual ~ITerrainSystem() = default;

    // -----------------------------------------------------------------------
    // Dimensions
    // -----------------------------------------------------------------------

    virtual int GetWidth()       const = 0; ///< Number of corner vertices in X
    virtual int GetMapHeight()   const = 0; ///< Number of corner vertices in Z
    virtual int GetLayerCount()  const = 0; ///< Always 1 for single-layer, 3 for multi

    // -----------------------------------------------------------------------
    // Height access
    // -----------------------------------------------------------------------

    /** Return the height at vertex (x, z) on the given layer. */
    virtual float GetHeight(int x, int z, int layer) const = 0;

    /** Write height h at vertex (x, z) on the given layer. */
    virtual void  SetHeight(int x, int z, int layer, float h) = 0;

    // -----------------------------------------------------------------------
    // Layer queries
    // -----------------------------------------------------------------------

    /**
     * Bitmask of populated layers.
     * Single-layer: always 0x02 (Surface). Multi-layer: 0–7.
     */
    virtual uint8_t GetActiveLayerMask(int x, int z) const = 0;

    /** {minHeight, maxHeight} across all active layers at (x, z). */
    virtual float2  GetHeightRange(int x, int z)     const = 0;

    /** True if the layer has had at least one SetHeight call. */
    virtual bool    IsLayerActive(int layer)          const = 0;

    // -----------------------------------------------------------------------
    // Layer metadata
    // -----------------------------------------------------------------------

    virtual std::string GetLayerName(int layer)      const = 0;
    virtual float       GetLayerBaseHeight(int layer) const = 0;
    virtual bool        IsLayerDeformable(int layer)  const = 0;
};
