# Git Feature Tracker

We have built multiple layers of features on top of each other. To cleanly commit these one by one from a clean slate, here is the chronological list of features we implemented since the last commit:

## 1. Core ECS & Build Fixes
- **CMakeLists.txt**: Added `WGPU_IMPLIB` and `THORVG_LIB` paths to `tether_ecs_test` target to fix linker errors.
- **tether_ecs.c / .h**: Removed the unused `destructor` field from `Tether_ComponentType` registration and dense arrays to fix syntax/compilation errors.
- **tether.h**: Fixed header include ordering (added `<stdint.h>`).

## 2. Layout Engine Architecture (Phase 1)
- **tether_content.c / .h**: Created new files to handle generic content measuring (decoupling text-specific logic from the layout engine).
- **tether_layout.c**: Removed the $O(N)$ `tether_text_measure_all()` function. Implemented `tether_layout_get_intrinsic_size()` to fetch sizes dynamically during the layout pass.
- **tether.c**: Removed the dependency on `tether_raster.h`.

## 3. Component Schema (Flex & Anchor Slots)
- **tether_components.h**: Defined `Tether_FlexSlot` and `Tether_AnchorSlot` structs.
- **tether_yaml.c**: Added parsing logic so `flex_slot:` and `anchor_slot:` YAML nodes automatically populate the ECS components.
- **tether_widgets.c / .h**: Updated widget creation factories to support these new slots.
- **thorvg_raster.c**: Updated rasterizer to handle the new layout boundary schema.

## 4. Layout Engine Bug Fixes (Floating Fallback)
- **tether_layout.c**: Fixed a bug where children in a `FlexFlow` without an explicit `flex_slot` were accidentally skipping the flex constraints and being treated as absolute floating elements. They now correctly default to `fill_ratio: 0.0`.

## 5. Advanced UI Sandboxes & Navigation
- **sandbox_main.c**: Added C-bindings for sidebar navigation buttons (`btn_nav_flex`, etc.) to dynamically load different YAML test files.
- **index.yaml**: Updated the root sandbox to use proper slots and fixed the floating notification badge offset array.
- **test_flex_sizing.yaml**: Built an Analytics Dashboard UI to test nested flex ratios.
- **test_canvas_anchor.yaml**: Built an advanced Game HUD overlay to test pivots and anchors.
- **test_text_wrap.yaml**: Built a Chat Interface to test dynamic text wrapping and width constraints.
