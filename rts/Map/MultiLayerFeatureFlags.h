/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

/**
 * Compile-time gate for the multi-layer terrain system.
 *
 * Define ENABLE_MULTI_LAYER_TERRAIN=0 on the compiler command line (or in a
 * CMake cache variable) to compile the engine without any multi-layer code.
 * The default is enabled (1).
 *
 * Runtime control is handled separately via the "MultiLayerTerrainEnabled"
 * config key; see SMFGroundDrawer.cpp for the actual guard.
 */
#ifndef ENABLE_MULTI_LAYER_TERRAIN
#define ENABLE_MULTI_LAYER_TERRAIN 1
#endif
