# World Thumbnail Readback Plan

## Current Facts

- Asset thumbnails are stored as cooked texture resources named `<thumbnail_guid>.sfg_thumb_bin`.
- The thumbnail guid is derived from `<asset_guid>_thumb`.
- `editor_asset_thumbnailer_t::get_thumbnail()` is query-only and returns `NULL_SID` until the thumbnail texture resource is ready.
- CPU thumbnail generation currently exists for texture and font assets.
- Builtin thumbnail asset types use preloaded editor textures.
- Unsupported generated thumbnail types should not save a fallback thumbnail file.
- World rendering already renders into offscreen per-frame textures through `world_render_context_t`.
- GPU readback primitives already exist: readback buffers, `map_resource`, and `cmd_copy_texture_to_buffer`.

## Target Ownership

Keep `editor_asset_thumbnailer_t` as the cache/file/resource-facing API.

Add `editor_thumbnail_render_service_t` for GPU-rendered thumbnails. It owns request state, temporary preview worlds, render contexts, readback buffers, and completion state.

Suggested ownership:

- Main-thread enqueue/query through the editor app or asset thumbnailer.
- Render-thread execution called from `editor_renderer_t`.
- Cook/save completion handed back to the main thread before loading the resulting texture resource.

## Thumbnail Categories

- CPU generated: texture, font.
- Builtin: audio, shader, animation, texture sampler, physical material.
- GPU rendered: material, mesh, prefab, world.
- Future GPU rendered: hdr skybox.

## Service API Shape

Main thread:

- `request_thumbnail(const editor_asset_t& asset, const char* asset_name, bool force)`
- `is_requested(sid_t asset_guid)`
- `flush_completed()`

Render thread:

- `render_pending(gfx_handle_t queue_gfx, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)`

## Render Flow

1. `editor_asset_thumbnailer_t::ensure()` classifies the asset.
2. CPU thumbnails generate immediately.
3. GPU thumbnails enqueue a render-service request and return without creating a file.
4. The render service waits until all required resources are ready.
5. A temporary `world_t` is built for the asset.
6. `world_snapshot_producer_t` produces a snapshot.
7. `world_rendering_t::render_world()` renders into a reusable 256x256 thumbnail render context.
8. A copy command copies the final thumbnail texture to a readback buffer.
9. The service waits for or polls the readback fence.
10. The readback rows are copied into tightly packed RGBA8 pixels.
11. `texture_cooker::cook_from_data()` saves `<thumbnail_guid>.sfg_thumb_bin` with `use_streaming = false`.
12. The completed thumbnail is loaded through `resource_manager_t` when the main thread processes completions.

## Preview Scene Builders

Mesh:

- Load the mesh and its material dependencies.
- Create a temporary entity with `component_mesh_renderer_t`.
- Use material slots from mesh runtime data, falling back to default gbuffer material when needed.
- Fit camera from `mesh_internals_t::local_bounds`.

Material:

- Use an editor default preview mesh, preferably a sphere.
- Assign the requested material.
- Use deterministic camera and environment.

Prefab:

- Load prefab resource.
- Spawn into a temporary world.
- Scan/load resources.
- Fit camera to combined render bounds.

World:

- Parse embedded world source with `world_cooker_t::world_from_json()`.
- Load scanned resources.
- Use the world camera if available, otherwise fit a generated preview camera.

Skybox:

- Defer until a dedicated cubemap/skybox preview path exists.
- Do not generate a fallback file.

## Important Constraints

- Do not render a request until dependencies are ready. `world_snapshot_producer_t` skips not-ready mesh/material/texture resources, which would produce empty thumbnails.
- `world_rendering_t::render_world()` is not reentrant because it uses static render graph state. Thumbnail rendering must be serialized with normal world rendering.
- Existing final world output is `r16g16b16a16_sfloat`; thumbnails should use a dedicated RGBA8 final/readback target instead of CPU half-float conversion.
- UI should continue to tolerate `NULL_SID` thumbnails and draw its fallback rect until a generated thumbnail is loaded.
- Failed GPU thumbnail generation should leave no thumbnail cache file.

## Implementation Phases

1. Add the render service and request queue, but only process mesh thumbnails.
2. Add reusable 256x256 thumbnail render context and RGBA8 readback path.
3. Save cooked readback output and load it through the existing thumbnail resource path.
4. Add material thumbnails using a preview mesh.
5. Add prefab/world thumbnails with temporary world construction and camera fitting.
6. Add nonblocking fence query if needed to avoid stalls during readback.
7. Add thumbnail regeneration/reload handling for thumbnails already loaded by the UI.
