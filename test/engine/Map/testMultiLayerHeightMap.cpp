/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Map/MultiLayerHeightMap.h"

#include <catch_amalgamated.hpp>


// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static MultiLayerHeightMap MakeMap(int w = 256, int h = 256)
{
    return MultiLayerHeightMap(w + 1, h + 1); // vertex grid is (mapx+1)*(mapy+1)
}


// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST_CASE("MultiLayerHeightMap construction", "[terrain][multilayer]")
{
    MultiLayerHeightMap m(257, 257);

    REQUIRE(m.GetWidth()     == 257);
    REQUIRE(m.GetMapHeight() == 257);
    REQUIRE(m.GetLayerCount() == 3);

    // Default heights should be zero
    REQUIRE(m.GetHeight(0,   0,   0) == 0.0f);
    REQUIRE(m.GetHeight(128, 128, 1) == 0.0f);
    REQUIRE(m.GetHeight(256, 256, 2) == 0.0f);

    // Default layer names from roadmap
    REQUIRE(m.GetLayerInfo(0).name == "Underground");
    REQUIRE(m.GetLayerInfo(1).name == "Surface");
    REQUIRE(m.GetLayerInfo(2).name == "Elevated");

    // No layer is active yet
    REQUIRE_FALSE(m.IsLayerActive(0));
    REQUIRE_FALSE(m.IsLayerActive(1));
    REQUIRE_FALSE(m.IsLayerActive(2));
}


// ---------------------------------------------------------------------------
// GetHeight / SetHeight
// ---------------------------------------------------------------------------

TEST_CASE("MultiLayerHeightMap get/set height per layer", "[terrain][multilayer]")
{
    MultiLayerHeightMap m(257, 257);

    m.SetHeight(100, 100, 0, 100.0f);
    m.SetHeight(100, 100, 1, 200.0f);
    m.SetHeight(100, 100, 2, 300.0f);

    REQUIRE(m.GetHeight(100, 100, 0) == Approx(100.0f));
    REQUIRE(m.GetHeight(100, 100, 1) == Approx(200.0f));
    REQUIRE(m.GetHeight(100, 100, 2) == Approx(300.0f));

    // Other positions should still be zero
    REQUIRE(m.GetHeight(0, 0, 0) == Approx(0.0f));
    REQUIRE(m.GetHeight(0, 0, 1) == Approx(0.0f));
}

TEST_CASE("MultiLayerHeightMap out-of-bounds returns 0", "[terrain][multilayer]")
{
    MultiLayerHeightMap m(5, 5);

    REQUIRE(m.GetHeight(-1,  0, 0) == 0.0f);
    REQUIRE(m.GetHeight( 5,  0, 0) == 0.0f);
    REQUIRE(m.GetHeight( 0, -1, 0) == 0.0f);
    REQUIRE(m.GetHeight( 0,  5, 0) == 0.0f);
    REQUIRE(m.GetHeight( 0,  0, -1) == 0.0f);
    REQUIRE(m.GetHeight( 0,  0,  3) == 0.0f);

    // SetHeight on out-of-bounds is a no-op (no crash)
    m.SetHeight(-1, 0, 0, 999.0f);
    m.SetHeight(0, 0, 5, 999.0f);
}


// ---------------------------------------------------------------------------
// Layer activation
// ---------------------------------------------------------------------------

TEST_CASE("MultiLayerHeightMap layer activation on first SetHeight", "[terrain][multilayer]")
{
    MultiLayerHeightMap m(257, 257);

    REQUIRE_FALSE(m.IsLayerActive(0));
    REQUIRE_FALSE(m.IsLayerActive(1));

    m.SetHeight(10, 10, 0, 50.0f);

    REQUIRE(m.IsLayerActive(0));
    REQUIRE_FALSE(m.IsLayerActive(1)); // other layers still inactive
}


// ---------------------------------------------------------------------------
// GetActiveLayerMask
// ---------------------------------------------------------------------------

TEST_CASE("MultiLayerHeightMap GetActiveLayerMask", "[terrain][multilayer]")
{
    MultiLayerHeightMap m(257, 257);

    // No layers active → mask == 0
    REQUIRE(m.GetActiveLayerMask(50, 50) == 0x00);

    m.SetHeight(50, 50, 0, 100.0f);
    m.SetHeight(50, 50, 1, 200.0f);
    // Layer 2 deliberately not set

    // Mask should have bits 0 and 1 set (0x03)
    REQUIRE(m.GetActiveLayerMask(50, 50) == 0x03);

    m.SetHeight(50, 50, 2, 300.0f);
    REQUIRE(m.GetActiveLayerMask(50, 50) == 0x07);
}


// ---------------------------------------------------------------------------
// GetHeightRange
// ---------------------------------------------------------------------------

TEST_CASE("MultiLayerHeightMap GetHeightRange no active layers", "[terrain][multilayer]")
{
    MultiLayerHeightMap m(257, 257);
    const float2 range = m.GetHeightRange(100, 100);
    REQUIRE(range.x == Approx(0.0f));
    REQUIRE(range.y == Approx(0.0f));
}

TEST_CASE("MultiLayerHeightMap GetHeightRange with data", "[terrain][multilayer]")
{
    MultiLayerHeightMap m(257, 257);

    m.SetHeight(100, 100, 0, -200.0f);
    m.SetHeight(100, 100, 1,    0.0f);
    m.SetHeight(100, 100, 2,  400.0f);

    const float2 range = m.GetHeightRange(100, 100);
    REQUIRE(range.x == Approx(-200.0f)); // min
    REQUIRE(range.y == Approx( 400.0f)); // max
}

TEST_CASE("MultiLayerHeightMap GetHeightRange single active layer", "[terrain][multilayer]")
{
    MultiLayerHeightMap m(257, 257);
    m.SetHeight(5, 5, 1, 150.0f);

    const float2 range = m.GetHeightRange(5, 5);
    REQUIRE(range.x == Approx(150.0f));
    REQUIRE(range.y == Approx(150.0f));
}


// ---------------------------------------------------------------------------
// GetLayerInfo / SetLayerInfo
// ---------------------------------------------------------------------------

TEST_CASE("MultiLayerHeightMap layer info read/write", "[terrain][multilayer]")
{
    MultiLayerHeightMap m(257, 257);

    LayerInfo info;
    info.name       = "MyLayer";
    info.baseHeight = 1234.0f;
    info.deformable = false;

    m.SetLayerInfo(1, info);

    const LayerInfo& got = m.GetLayerInfo(1);
    REQUIRE(got.name       == "MyLayer");
    REQUIRE(got.baseHeight == Approx(1234.0f));
    REQUIRE(got.deformable == false);
}


// ---------------------------------------------------------------------------
// GetInterpolatedHeight
// ---------------------------------------------------------------------------

TEST_CASE("MultiLayerHeightMap bilinear interpolation midpoint", "[terrain][multilayer]")
{
    MultiLayerHeightMap m(3, 3); // 3x3 vertex grid

    // Four corners of the [0,1] x [0,1] cell at vertex (0,0)
    m.SetHeight(0, 0, 1,   0.0f);
    m.SetHeight(1, 0, 1, 100.0f);
    m.SetHeight(0, 1, 1, 100.0f);
    m.SetHeight(1, 1, 1, 200.0f);

    // Bilinear interpolation at exact centre (0.5, 0.5) should give 100
    REQUIRE(m.GetInterpolatedHeight(0.5f, 0.5f, 1) == Approx(100.0f));

    // At corners themselves should match SetHeight values
    REQUIRE(m.GetInterpolatedHeight(0.0f, 0.0f, 1) == Approx(  0.0f));
    REQUIRE(m.GetInterpolatedHeight(1.0f, 0.0f, 1) == Approx(100.0f));
    REQUIRE(m.GetInterpolatedHeight(0.0f, 1.0f, 1) == Approx(100.0f));
    REQUIRE(m.GetInterpolatedHeight(1.0f, 1.0f, 1) == Approx(200.0f));
}

TEST_CASE("MultiLayerHeightMap bilinear interpolation clamped OOB", "[terrain][multilayer]")
{
    MultiLayerHeightMap m(3, 3);
    m.SetHeight(0, 0, 0, 42.0f);

    // Out-of-bounds is clamped, not zero
    REQUIRE(m.GetInterpolatedHeight(-1.0f, -1.0f, 0) == Approx(42.0f));
    REQUIRE(m.GetInterpolatedHeight(99.0f, 99.0f, 0) == Approx(0.0f)); // far corner
}


// ---------------------------------------------------------------------------
// Independence of layers
// ---------------------------------------------------------------------------

TEST_CASE("MultiLayerHeightMap layers are independent storage", "[terrain][multilayer]")
{
    MultiLayerHeightMap m(10, 10);

    for (int z = 0; z < 10; ++z) {
        for (int x = 0; x < 10; ++x) {
            m.SetHeight(x, z, 0, static_cast<float>(x + z * 10));
            m.SetHeight(x, z, 1, static_cast<float>(x + z * 10) * 2.0f);
            m.SetHeight(x, z, 2, static_cast<float>(x + z * 10) * 3.0f);
        }
    }

    for (int z = 0; z < 10; ++z) {
        for (int x = 0; x < 10; ++x) {
            const float base = static_cast<float>(x + z * 10);
            REQUIRE(m.GetHeight(x, z, 0) == Approx(base));
            REQUIRE(m.GetHeight(x, z, 1) == Approx(base * 2.0f));
            REQUIRE(m.GetHeight(x, z, 2) == Approx(base * 3.0f));
        }
    }
}
