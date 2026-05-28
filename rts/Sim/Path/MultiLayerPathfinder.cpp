/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MultiLayerPathfinder.h"

#include <algorithm>
#include <cassert>
#include <queue>
#include <unordered_map>
#include <cmath>

// ---------------------------------------------------------------------------

MultiLayerPathfinder::MultiLayerPathfinder(const MultiLayerHeightMap* hm)
    : heightMap(hm)
{
    assert(hm != nullptr);
}


// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

float MultiLayerPathfinder::Heuristic(int x, int z, uint8_t layer,
                                      int gx, int gz, uint8_t gl) const
{
    // Octile-distance in XZ, plus a flat penalty for each layer difference
    const float dx = static_cast<float>(std::abs(x - gx));
    const float dz = static_cast<float>(std::abs(z - gz));
    const float dl = static_cast<float>(std::abs(static_cast<int>(layer) - static_cast<int>(gl)));
    return std::max(dx, dz) + (std::sqrt(2.0f) - 1.0f) * std::min(dx, dz) + dl * 10.0f;
}


float MultiLayerPathfinder::MoveCost(int x, int z, uint8_t layer) const
{
    // Flat cost of 1.0 per cell.  A full implementation would query the
    // per-layer type map to scale cost by terrain-type speed modifiers.
    return 1.0f;
}


bool MultiLayerPathfinder::CanEnter(int x, int z, uint8_t layer,
                                    const MovementCapabilities& caps) const
{
    if (x < 0 || x >= heightMap->GetWidth())  return false;
    if (z < 0 || z >= heightMap->GetMapHeight()) return false;
    if (layer >= NUM_LAYERS)                   return false;
    if (!(caps.traversableLayers & (1u << layer))) return false;
    if (!heightMap->IsLayerActive(layer))      return false;
    return true;
}


LayeredPath MultiLayerPathfinder::ReconstructPath(
    const std::vector<Node>& nodes, int goalIdx) const
{
    LayeredPath path;

    // Trace back from goal to start through parent links
    std::vector<int> chain;
    for (int idx = goalIdx; idx != -1; idx = nodes[idx].parent)
        chain.push_back(idx);

    std::reverse(chain.begin(), chain.end());

    uint8_t prevLayer = nodes[chain.front()].layer;
    for (size_t i = 0; i < chain.size(); ++i) {
        const Node& n = nodes[chain[i]];
        LayeredWaypoint wp;
        wp.xz    = {static_cast<float>(n.x), static_cast<float>(n.z)};
        wp.layer = n.layer;
        path.waypoints.push_back(wp);

        if (i > 0 && n.layer != prevLayer)
            path.layerTransitionIndices.push_back(static_cast<int>(i));

        prevLayer = n.layer;
    }

    // Approximate path length as number of waypoints (1 cell ≈ 1 unit)
    path.length = static_cast<float>(path.waypoints.size());
    return path;
}


// ---------------------------------------------------------------------------
// 3-D A* search
// ---------------------------------------------------------------------------

LayeredPath MultiLayerPathfinder::FindPath3D(float2 startXZ, float2 goalXZ,
                                              const MovementCapabilities& caps) const
{
    const int sx = static_cast<int>(std::round(startXZ.x));
    const int sz = static_cast<int>(std::round(startXZ.y));
    const int gx = static_cast<int>(std::round(goalXZ.x));
    const int gz = static_cast<int>(std::round(goalXZ.y));

    // Find lowest active layer at start and goal
    uint8_t startLayer = 255, goalLayer = 255;
    for (uint8_t l = 0; l < NUM_LAYERS; ++l) {
        if (!(caps.traversableLayers & (1u << l))) continue;
        if (!heightMap->IsLayerActive(l))           continue;
        if (startLayer == 255) startLayer = l;
        if (goalLayer  == 255) goalLayer  = l;
    }

    if (startLayer == 255 || goalLayer == 255)
        return {};  // no traversable active layer

    // Closed set: encode (x, z, layer) as single int64
    auto encodeKey = [&](int x, int z, uint8_t layer) -> int64_t {
        return (static_cast<int64_t>(layer) << 48)
             | (static_cast<int64_t>(z & 0xFFFFFF) << 24)
             |  static_cast<int64_t>(x & 0xFFFFFF);
    };

    std::unordered_map<int64_t, float> closedCost; // key → best gCost seen
    std::vector<Node> allNodes;
    allNodes.reserve(4096);

    // min-heap on fCost
    using QEntry = std::pair<float, int>; // {fCost, nodeIdx}
    std::priority_queue<QEntry, std::vector<QEntry>, std::greater<QEntry>> open;

    auto pushNode = [&](int x, int z, uint8_t layer, float g, float h, int parent) {
        Node nd;
        nd.x      = x;
        nd.z      = z;
        nd.layer  = layer;
        nd.gCost  = g;
        nd.fCost  = g + h;
        nd.parent = parent;
        const int idx = static_cast<int>(allNodes.size());
        allNodes.push_back(nd);
        open.push({nd.fCost, idx});
    };

    const float h0 = Heuristic(sx, sz, startLayer, gx, gz, goalLayer);
    pushNode(sx, sz, startLayer, 0.0f, h0, -1);

    // 4-directional neighbours in same layer
    static constexpr int DX[] = { 0, 1,  0, -1 };
    static constexpr int DZ[] = { 1, 0, -1,  0 };

    while (!open.empty()) {
        const auto [fTop, idx] = open.top();
        open.pop();

        const Node& cur = allNodes[idx];
        const int64_t key = encodeKey(cur.x, cur.z, cur.layer);

        // Skip if we already found a better path to this node
        auto it = closedCost.find(key);
        if (it != closedCost.end() && it->second <= cur.gCost)
            continue;
        closedCost[key] = cur.gCost;

        // Goal test
        if (cur.x == gx && cur.z == gz && cur.layer == goalLayer)
            return ReconstructPath(allNodes, idx);

        // --- Neighbours in same layer ---
        for (int d = 0; d < 4; ++d) {
            const int nx = cur.x + DX[d];
            const int nz = cur.z + DZ[d];

            if (!CanEnter(nx, nz, cur.layer, caps))
                continue;

            const float ng = cur.gCost + MoveCost(nx, nz, cur.layer);
            const int64_t nkey = encodeKey(nx, nz, cur.layer);
            auto nit = closedCost.find(nkey);
            if (nit != closedCost.end() && nit->second <= ng)
                continue;

            const float nh = Heuristic(nx, nz, cur.layer, gx, gz, goalLayer);
            pushNode(nx, nz, cur.layer, ng, nh, idx);
        }

        // --- Layer transitions at current (x, z) ---
        for (uint8_t nl = 0; nl < NUM_LAYERS; ++nl) {
            if (nl == cur.layer) continue;
            if (!CanEnter(cur.x, cur.z, nl, caps)) continue;

            const float ng = cur.gCost + caps.layerTransitionCost
                             + MoveCost(cur.x, cur.z, nl);
            const int64_t nkey = encodeKey(cur.x, cur.z, nl);
            auto nit = closedCost.find(nkey);
            if (nit != closedCost.end() && nit->second <= ng)
                continue;

            const float nh = Heuristic(cur.x, cur.z, nl, gx, gz, goalLayer);
            pushNode(cur.x, cur.z, nl, ng, nh, idx);
        }
    }

    return {}; // no path found
}
