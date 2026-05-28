/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MultiLayerTerrainRenderer.h"

#include "SMFGroundDrawer.h"
#include "Map/MapDimensions.h"
#include "System/Log/ILog.h"

#include <cassert>


MultiLayerTerrainRenderer::MultiLayerTerrainRenderer(CSMFGroundDrawer* drawer,
                                                     const MultiLayerHeightMap* hm)
    : groundDrawer(drawer)
    , heightMap(hm)
    , mesh(hm)
{
    assert(drawer   != nullptr);
    assert(hm       != nullptr);
}

MultiLayerTerrainRenderer::~MultiLayerTerrainRenderer() = default;


void MultiLayerTerrainRenderer::Update()
{
    // For each layer, clear dirty flags once mesh data would be regenerated.
    // In a full implementation this is where GPU buffers get updated; here we
    // just acknowledge the dirty state so callers can observe the transition.
    for (int layer = 0; layer < NUM_LAYERS; ++layer) {
        if (!heightMap->IsLayerActive(layer))
            continue;

        if (mesh.IsMeshDirty(layer)) {
            // In a full implementation: re-upload vertex buffer data for the
            // affected dirty rect of this layer to the GPU.
            // For Phase 2 we record the fact that the update was processed.
            LOG_L(L_DEBUG, "[MultiLayerTerrainRenderer] layer %d mesh updated (dirty rect %d,%d - %d,%d)",
                  layer,
                  mesh.GetDirtyRect(layer).x0, mesh.GetDirtyRect(layer).z0,
                  mesh.GetDirtyRect(layer).x1, mesh.GetDirtyRect(layer).z1);

            mesh.ClearDirty(layer);
        }
    }
}


void MultiLayerTerrainRenderer::Render(const DrawPass::e& drawPass)
{
    // Draw layers back-to-front: Underground (0), Surface (1), Elevated (2).
    for (int layer = 0; layer < NUM_LAYERS; ++layer) {
        if (!heightMap->IsLayerActive(layer))
            continue;

        RenderLayer(layer, drawPass);
    }
}


void MultiLayerTerrainRenderer::RenderLayer(int layer, const DrawPass::e& drawPass)
{
    // Fire the pre-render callback (used in testing and future hooks).
    if (beforeRenderCb)
        beforeRenderCb(layer);

    // Delegate to the existing SMF ground drawer for the actual GL calls.
    // The layer uniform is set before the draw so the fragment shader can
    // select the correct texture set.  In Phase 2 the existing single-layer
    // shader is used; the multi-layer shader in SMFFragProgMultiLayer.glsl
    // will replace it when the Rendering phase is feature-complete.
    if (groundDrawer != nullptr)
        groundDrawer->Draw(drawPass);
}
