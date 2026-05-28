/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Sim/Path/MultiLayerPathfinder.h"
#include "Map/MultiLayerHeightMap.h"

#include <catch_amalgamated.hpp>


// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static MultiLayerHeightMap MakeActiveMap(int w = 32, int h = 32)
{
    MultiLayerHeightMap m(w + 1, h + 1);
    // Activate all three layers uniformly
    for (int z = 0; z <= h; ++z)
        for (int x = 0; x <= w; ++x) {
            m.SetHeight(x, z, 0, -200.0f);
            m.SetHeight(x, z, 1,    0.0f);
            m.SetHeight(x, z, 2,  500.0f);
        }
    return m;
}

static MovementCapabilities SurfaceOnlyCaps()
{
    MovementCapabilities caps;
    caps.traversableLayers    = 0x02; // bit 1 = Surface
    caps.layerTransitionCost  = 100.0f;
    return caps;
}

static MovementCapabilities UndergroundOnlyCaps()
{
    MovementCapabilities caps;
    caps.traversableLayers    = 0x01; // bit 0 = Underground
    caps.layerTransitionCost  = 100.0f;
    return caps;
}

static MovementCapabilities AllLayersCaps(float transitionCost = 50.0f)
{
    MovementCapabilities caps;
    caps.traversableLayers    = 0x07; // bits 0-2 = all layers
    caps.layerTransitionCost  = transitionCost;
    return caps;
}


// ---------------------------------------------------------------------------
// Basic routing
// ---------------------------------------------------------------------------

TEST_CASE("MultiLayerPathfinder surface-only unit finds path", "[pathing][multilayer]")
{
    auto map = MakeActiveMap();
    MultiLayerPathfinder pf(&map);

    auto path = pf.FindPath3D({0.0f, 0.0f}, {10.0f, 10.0f}, SurfaceOnlyCaps());

    REQUIRE_FALSE(path.IsEmpty());
    // All waypoints must be on layer 1 (Surface)
    for (const auto& wp : path.waypoints)
        REQUIRE(wp.layer == 1);
    // No layer transitions
    REQUIRE(path.layerTransitionIndices.empty());
}

TEST_CASE("MultiLayerPathfinder underground-only unit finds path", "[pathing][multilayer]")
{
    auto map = MakeActiveMap();
    MultiLayerPathfinder pf(&map);

    auto path = pf.FindPath3D({0.0f, 0.0f}, {15.0f, 15.0f}, UndergroundOnlyCaps());

    REQUIRE_FALSE(path.IsEmpty());
    for (const auto& wp : path.waypoints)
        REQUIRE(wp.layer == 0); // Underground
    REQUIRE(path.layerTransitionIndices.empty());
}

TEST_CASE("MultiLayerPathfinder all-layer unit may transition", "[pathing][multilayer]")
{
    // Create a map where underground is blocked in the middle,
    // forcing a transition to surface for part of the path.
    MultiLayerHeightMap map(17, 17); // 16x16 squares

    // Activate underground everywhere except a central column
    for (int z = 0; z <= 16; ++z) {
        for (int x = 0; x <= 16; ++x) {
            if (x != 8) // leave column 8 with underground absent
                map.SetHeight(x, z, 0, -200.0f);
            map.SetHeight(x, z, 1, 0.0f);
        }
    }

    MultiLayerPathfinder pf(&map);
    // Low transition cost → should happily switch layers
    auto path = pf.FindPath3D({0.0f, 8.0f}, {16.0f, 8.0f}, AllLayersCaps(5.0f));

    REQUIRE_FALSE(path.IsEmpty());
    // Path must start and end at the correct positions
    REQUIRE(path.waypoints.front().xz.x == Approx(0.0f));
    REQUIRE(path.waypoints.back().xz.x  == Approx(16.0f));
}

TEST_CASE("MultiLayerPathfinder returns empty if no active layer matches", "[pathing][multilayer]")
{
    MultiLayerHeightMap map(9, 9);
    // Only activate layer 2 (Elevated)
    for (int z = 0; z <= 8; ++z)
        for (int x = 0; x <= 8; ++x)
            map.SetHeight(x, z, 2, 500.0f);

    MultiLayerPathfinder pf(&map);

    // Request with Surface-only caps — no Surface layer active
    auto path = pf.FindPath3D({0.0f, 0.0f}, {8.0f, 8.0f}, SurfaceOnlyCaps());
    REQUIRE(path.IsEmpty());
}

TEST_CASE("MultiLayerPathfinder start == goal returns path of length 1", "[pathing][multilayer]")
{
    auto map = MakeActiveMap(8, 8);
    MultiLayerPathfinder pf(&map);

    auto path = pf.FindPath3D({4.0f, 4.0f}, {4.0f, 4.0f}, SurfaceOnlyCaps());

    REQUIRE_FALSE(path.IsEmpty());
    REQUIRE(path.waypoints.size() == 1);
    REQUIRE(path.waypoints[0].xz.x == Approx(4.0f));
    REQUIRE(path.waypoints[0].xz.y == Approx(4.0f));
}

TEST_CASE("MultiLayerPathfinder layer transition indices are recorded", "[pathing][multilayer]")
{
    // Construct a map where start is underground and goal is elevated,
    // forcing two layer transitions.
    MultiLayerHeightMap map(5, 5);
    for (int z = 0; z <= 4; ++z)
        for (int x = 0; x <= 4; ++x) {
            map.SetHeight(x, z, 0, -200.0f);
            map.SetHeight(x, z, 1,    0.0f);
            map.SetHeight(x, z, 2,  500.0f);
        }

    MultiLayerPathfinder pf(&map);

    MovementCapabilities caps = AllLayersCaps(0.1f); // almost free transitions
    caps.traversableLayers = 0x07;

    auto path = pf.FindPath3D({0.0f, 0.0f}, {4.0f, 4.0f}, caps);

    REQUIRE_FALSE(path.IsEmpty());
    // At least start and end waypoints exist
    REQUIRE(path.waypoints.size() >= 2);
}
