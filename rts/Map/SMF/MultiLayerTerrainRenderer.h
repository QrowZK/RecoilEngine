/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <array>
#include <functional>
#include <memory>

#include "MultiLayerTerrainMesh.h"
#include "Map/MapDrawPassTypes.h"
#include "Map/MultiLayerHeightMap.h"

class CSMFGroundDrawer;

/**
 * Orchestrates rendering of all 3 terrain layers.
 *
 * Layers are drawn back-to-front (layer 0 first, layer 2 last) so that
 * higher layers naturally occlude lower ones with the standard depth buffer.
 *
 * Usage (from CSMFGroundDrawer):
 *
 *   auto renderer = std::make_unique<MultiLayerTerrainRenderer>(
 *                       smfGroundDrawer, heightMap);
 *
 *   // When height data changes in a layer:
 *   renderer->GetMesh().NotifyHeightChanged(layer, x0, z0, x1, z1);
 *
 *   // Each frame:
 *   renderer->Update();
 *   renderer->Render(drawPass);
 */
class MultiLayerTerrainRenderer {
public:
    static constexpr int NUM_LAYERS = MultiLayerHeightMap::NUM_LAYERS;

    MultiLayerTerrainRenderer(CSMFGroundDrawer* drawer,
                              const MultiLayerHeightMap* heightMap);
    ~MultiLayerTerrainRenderer();

    // -------------------------------------------------------------------
    // Frame lifecycle
    // -------------------------------------------------------------------

    /**
     * Called once per frame before drawing.
     * Propagates dirty-rect updates, rebuilds any stale per-layer GL data,
     * and invokes any registered pre-render callbacks.
     */
    void Update();

    /**
     * Draw all active terrain layers for the given pass.
     * Fires OnBeforeRenderLayer callbacks in order 0 → 1 → 2.
     */
    void Render(const DrawPass::e& drawPass);

    // -------------------------------------------------------------------
    // Mesh / dirty state
    // -------------------------------------------------------------------

    MultiLayerTerrainMesh&       GetMesh()       { return mesh; }
    const MultiLayerTerrainMesh& GetMesh() const  { return mesh; }

    /**
     * Returns true if the layer mesh has been marked dirty since the last
     * Update() call.  Exposed mainly for unit testing.
     */
    bool IsMeshDirty(int layer) const { return mesh.IsMeshDirty(layer); }

    // -------------------------------------------------------------------
    // Callbacks (for testing / hooks)
    // -------------------------------------------------------------------

    using LayerCallback = std::function<void(int layer)>;

    /** Registered function is called with the layer index before each draw. */
    void OnBeforeRenderLayer(LayerCallback cb) { beforeRenderCb = std::move(cb); }

private:
    void RenderLayer(int layer, const DrawPass::e& drawPass);

    CSMFGroundDrawer*     groundDrawer;
    const MultiLayerHeightMap* heightMap;
    MultiLayerTerrainMesh mesh;
    LayerCallback         beforeRenderCb;
};
