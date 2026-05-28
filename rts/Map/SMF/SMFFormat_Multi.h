/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "SMFFormat.h"

/**
 * Multi-layer terrain extension for the SMF format.
 *
 * A multi-layer map stores up to 3 independent heightmaps in the same .smf
 * file.  Layer 0 is the standard heightmap already present in SMFHeader;
 * layers 1 and 2 are referenced by a MultiLayerExtraHeader appended after the
 * standard header.
 *
 * File layout (multi-layer):
 *   SMFHeader  (as normal; heightmapPtr points to layer-0 data)
 *   MultiLayerExtraHeader  (type == MEH_MultiLayer)
 *   [other extra headers …]
 *   layer-0 heightmap   (uint16[(mapx+1)*(mapy+1)])
 *   layer-1 heightmap   (uint16[(mapx+1)*(mapy+1)])  ← new
 *   layer-2 heightmap   (uint16[(mapx+1)*(mapy+1)])  ← new
 *   …rest of file unchanged…
 */

/// Extra-header type tag for the multi-layer extension
#define MEH_MultiLayer 2

/// Maximum number of terrain layers supported
static constexpr int SMF_MAX_TERRAIN_LAYERS = 3;

/**
 * Extra header written immediately after SMFHeader when a map has more than
 * one terrain layer.  Each active additional layer (indices 1 and 2) gets a
 * heightmapPtr + height-range pair; a ptr value of 0 means the layer is absent.
 */
struct MultiLayerExtraHeader {
    int size;       ///< sizeof(MultiLayerExtraHeader)
    int type;       ///< MEH_MultiLayer

    int numExtraLayers; ///< 1 or 2  (total layers = 1 + numExtraLayers)

    /// File offsets for the extra heightmaps (index 0 → layer 1, index 1 → layer 2).
    /// A value of 0 means that layer is not present.
    int   heightmapPtrs[SMF_MAX_TERRAIN_LAYERS - 1];
    float minHeights   [SMF_MAX_TERRAIN_LAYERS - 1];
    float maxHeights   [SMF_MAX_TERRAIN_LAYERS - 1];
};

/**
 * Per-layer metadata stored inside the multi-layer extra header.
 * Extended from the basic height-range pair to carry human-readable info.
 */
struct SMFLayerInfo {
    char  name[32];       ///< Human-readable layer name (e.g. "Underground")
    float baseHeight;     ///< Logical base elevation for the layer in elmos
    int   flags;          ///< Bitmask; bit 0 = deformable
};

#define SMF_LAYER_FLAG_DEFORMABLE (1 << 0)
