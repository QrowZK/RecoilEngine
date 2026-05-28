/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Map/MultiLayerSync.h"
#include "Map/MultiLayerHeightMap.h"

#include <catch_amalgamated.hpp>


// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::unique_ptr<MultiLayerHeightMap> MakeMap(int w = 16, int h = 16)
{
    auto m = std::make_unique<MultiLayerHeightMap>(w + 1, h + 1);
    for (int z = 0; z <= h; ++z)
        for (int x = 0; x <= w; ++x) {
            m->SetHeight(x, z, 0, -100.0f);
            m->SetHeight(x, z, 1,    0.0f);
            m->SetHeight(x, z, 2,  200.0f);
        }
    return m;
}


// ---------------------------------------------------------------------------
// CRC computation
// ---------------------------------------------------------------------------

TEST_CASE("MultiLayerSync initial CRCs are dirty", "[sync][multilayer]")
{
    auto map = MakeMap();
    MultiLayerSync sync(map.get());

    REQUIRE(sync.IsLayerDirty(0));
    REQUIRE(sync.IsLayerDirty(1));
    REQUIRE(sync.IsLayerDirty(2));
}

TEST_CASE("MultiLayerSync UpdateLayerCRC clears dirty flag", "[sync][multilayer]")
{
    auto map = MakeMap();
    MultiLayerSync sync(map.get());

    sync.UpdateLayerCRC(1);

    REQUIRE_FALSE(sync.IsLayerDirty(1));
    REQUIRE(sync.GetLayerCRC(1) != 0);
}

TEST_CASE("MultiLayerSync CRCs differ between layers with different data", "[sync][multilayer]")
{
    auto map = MakeMap();
    MultiLayerSync sync(map.get());

    sync.UpdateDirtyCRCs();

    // Layers have different height data → different CRCs
    REQUIRE(sync.GetLayerCRC(0) != sync.GetLayerCRC(1));
    REQUIRE(sync.GetLayerCRC(1) != sync.GetLayerCRC(2));
}

TEST_CASE("MultiLayerSync same height data produces same CRC", "[sync][multilayer]")
{
    auto map = MakeMap();
    MultiLayerSync sync(map.get());

    sync.UpdateLayerCRC(1);
    const uint32_t crc1 = sync.GetLayerCRC(1);

    // Mark dirty and recompute — result must be identical
    sync.MarkLayerDirty(1);
    sync.UpdateLayerCRC(1);
    REQUIRE(sync.GetLayerCRC(1) == crc1);
}

TEST_CASE("MultiLayerSync CRC changes when heights change", "[sync][multilayer]")
{
    auto map = MakeMap();
    MultiLayerSync sync(map.get());

    sync.UpdateLayerCRC(1);
    const uint32_t original = sync.GetLayerCRC(1);

    // Modify the underlying map (normally done via CReadMap::SetHeight)
    map->SetHeight(5, 5, 1, 99.0f);
    sync.MarkLayerDirty(1);
    sync.UpdateLayerCRC(1);

    REQUIRE(sync.GetLayerCRC(1) != original);
}

TEST_CASE("MultiLayerSync inactive layer has CRC zero", "[sync][multilayer]")
{
    MultiLayerHeightMap map(9, 9);
    // Only activate layers 0 and 1; leave 2 empty
    for (int z = 0; z < 9; ++z)
        for (int x = 0; x < 9; ++x) {
            map.SetHeight(x, z, 0, -50.0f);
            map.SetHeight(x, z, 1,   0.0f);
        }

    MultiLayerSync sync(&map);
    sync.UpdateLayerCRC(2);

    REQUIRE(sync.GetLayerCRC(2) == 0);
    REQUIRE_FALSE(sync.IsLayerDirty(2));
}


// ---------------------------------------------------------------------------
// Serialisation / CRC comparison
// ---------------------------------------------------------------------------

TEST_CASE("MultiLayerSync SerializeCRCs round-trips through CompareCRCs", "[sync][multilayer]")
{
    auto map = MakeMap();
    MultiLayerSync sync(map.get());
    sync.UpdateDirtyCRCs();

    std::vector<uint8_t> payload;
    sync.SerializeCRCs(payload);

    REQUIRE(payload.size() == 16);
    // Comparing against our own payload → no desync
    REQUIRE(sync.CompareCRCs(payload) == 0x00);
}

TEST_CASE("MultiLayerSync CompareCRCs detects layer-specific desync", "[sync][multilayer]")
{
    auto map = MakeMap();
    MultiLayerSync sync(map.get());
    sync.UpdateDirtyCRCs();

    std::vector<uint8_t> payload;
    sync.SerializeCRCs(payload);

    // Corrupt the CRC for layer 1 (bytes 4-7)
    payload[4] ^= 0xFF;

    const uint8_t desynced = sync.CompareCRCs(payload);
    REQUIRE((desynced & 0x02) != 0); // bit 1 = layer 1
    REQUIRE((desynced & 0x05) == 0); // layers 0 and 2 are clean
}

TEST_CASE("MultiLayerSync CompareCRCs flags all on corrupt packet", "[sync][multilayer]")
{
    auto map = MakeMap();
    MultiLayerSync sync(map.get());
    sync.UpdateDirtyCRCs();

    // Payload too short
    const std::vector<uint8_t> bad(4, 0x00);
    REQUIRE(sync.CompareCRCs(bad) == 0xFF);
}


// ---------------------------------------------------------------------------
// MarkLayerDirty / UpdateDirtyCRCs
// ---------------------------------------------------------------------------

TEST_CASE("MultiLayerSync UpdateDirtyCRCs only recomputes dirty layers", "[sync][multilayer]")
{
    auto map = MakeMap();
    MultiLayerSync sync(map.get());

    sync.UpdateDirtyCRCs(); // compute all
    const uint32_t crc0 = sync.GetLayerCRC(0);
    const uint32_t crc2 = sync.GetLayerCRC(2);

    // Only mark layer 1 dirty
    sync.MarkLayerDirty(1);
    REQUIRE(sync.IsLayerDirty(1));
    REQUIRE_FALSE(sync.IsLayerDirty(0));
    REQUIRE_FALSE(sync.IsLayerDirty(2));

    sync.UpdateDirtyCRCs();

    // Layers 0 and 2 unchanged
    REQUIRE(sync.GetLayerCRC(0) == crc0);
    REQUIRE(sync.GetLayerCRC(2) == crc2);
}
