/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

#include "Map/MultiLayerHeightMap.h"
#include "System/float3.h"
#include "System/type2.h"

/**
 * Specifies which terrain layers a unit can traverse and the cost of
 * switching between them.  The bitmask directly mirrors MoveDef::traversableLayers.
 */
struct MovementCapabilities {
    uint8_t traversableLayers = 0x02; ///< default: surface only (bit 1)
    float   layerTransitionCost = 50.0f; ///< added to gCost for each layer switch
};

/**
 * A waypoint produced by MultiLayerPathfinder::FindPath3D.
 * Stores world-space XZ + the layer it belongs to.
 */
struct LayeredWaypoint {
    float2  xz;    ///< world-space position (elmos)
    uint8_t layer; ///< 0=Underground, 1=Surface, 2=Elevated
};

/**
 * Result of a 3D path search.
 */
struct LayeredPath {
    std::vector<LayeredWaypoint>  waypoints;
    std::vector<int>              layerTransitionIndices; ///< indices into waypoints where layer changes
    float                         length = 0.0f;          ///< estimated path length in elmos

    bool IsEmpty() const { return waypoints.empty(); }
};

/**
 * Layer-aware A* pathfinder operating on a MultiLayerHeightMap grid.
 *
 * Nodes are (x, z, layer) triples.  Within a layer movement follows the
 * standard 4-connected grid (N/S/E/W); layer transitions are modelled as
 * zero-distance moves with an added layerTransitionCost so the planner
 * only switches layers when it is genuinely beneficial.
 *
 * Design notes:
 *  - Uses square-cell coordinates in the vertex grid (same resolution as
 *    MultiLayerHeightMap), NOT world-space elmos.  Callers convert via
 *    SQUARE_SIZE from GlobalConstants.h.
 *  - Slope-blocking is intentionally omitted here; the existing HAPFS/QTPFS
 *    infrastructure handles that for the surface layer.  This finder is
 *    intended for new unit types that explicitly list their layers.
 *  - Not thread-safe; synchronised calls only.
 */
class MultiLayerPathfinder {
public:
    static constexpr int NUM_LAYERS = MultiLayerHeightMap::NUM_LAYERS;

    explicit MultiLayerPathfinder(const MultiLayerHeightMap* heightMap);

    /**
     * Find the cheapest path from start to goal in (x,z) vertex-grid coords.
     * Both start and goal are clamped to valid grid positions.
     *
     * @param startXZ  Start position in vertex-grid coords {x, z}
     * @param goalXZ   Goal position in vertex-grid coords {x, z}
     * @param caps     Movement capabilities of the requesting unit
     * @return         LayeredPath; empty if no path exists
     */
    LayeredPath FindPath3D(float2 startXZ, float2 goalXZ,
                           const MovementCapabilities& caps) const;

private:
    struct Node {
        float   gCost;
        float   fCost;
        int     x, z;
        uint8_t layer;
        int     parent; ///< index into nodes[], -1 for start

        bool operator>(const Node& o) const { return fCost > o.fCost; }
    };

    struct NodeKey {
        int x, z;
        uint8_t layer;
        bool operator==(const NodeKey& o) const {
            return x == o.x && z == o.z && layer == o.layer;
        }
    };

    float Heuristic(int x, int z, uint8_t layer, int gx, int gz, uint8_t gl) const;
    float MoveCost (int x, int z, uint8_t layer) const;
    bool  CanEnter (int x, int z, uint8_t layer, const MovementCapabilities& caps) const;
    LayeredPath ReconstructPath(const std::vector<Node>& nodes, int goalIdx) const;

    const MultiLayerHeightMap* heightMap;
};
