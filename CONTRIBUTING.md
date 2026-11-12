# Contributing to Tether

Tether (**TETHER - ECS Toolkit for High-Efficiency Runtime**) is a C project organized around modular layer boundaries. Read this guide before contributing.

---

## 1. Architectural Layer Boundaries (Strict Downward Dependencies)

Tether enforces strict unidirectional dependencies between subsystems. **Lower layers must never `#include` or call functions from higher layers.**

```
  Layer 5: Host / Examples / Entry   (src/tether.c, examples/, tests/)
               |
               v
  Layer 4: Authoring & Parsers       (include|src/engine/parser/*)
               |
               v
  Layer 3: UI Framework & Widgets    (include|src/ui/*, include|src/ui/widgets/*)
               |
               v
  Layer 2: Engine & Layout           (include|src/engine/tether_engine.*, tether_layout.*)
               |
               v
  Layer 1: Core ECS Kernel           (include|src/core/*)
               |
               v
  Layer 0: Utils & Backend Contracts (include|src/utils/*, include|src/backends/*)
```

### Layer Rules

| Layer | Paths | Allowed Dependencies | Forbidden Dependencies |
| :--- | :--- | :--- | :--- |
| **L0: Utils & HAL/RHI Contracts** | `utils/*`, `backends/*` (`tether_backend.h`) | Standard C99 | Core, Engine, UI, Parser |
| **L1: Core Kernel** | `core/*` (`ecs`, `components`, `events`, `input`, `registry`) | Standard C99, L0 (`utils/`) | Engine, Layout, UI, Widgets, Parser, `chitra.h` |
| **L2: Engine & Layout** | `engine/tether_engine.*`, `engine/tether_layout.*` | L1 (`core/*`), L0 (`tether_backend.h`) | YAML Parser, High-level Widgets (`button`, `dropdown`, etc.) |
| **L3: UI & Widgets** | `ui/*`, `ui/widgets/*` | L2 (`tether_engine`), L1 (`core/*`) | YAML Parser, concrete GPU drivers (`chitra.h`, OpenGL, D3D11) |
| **L4: Authoring / YAML** | `engine/parser/*` | L3 (`ui/*`), L1 (`core/*`), `libyaml` | Concrete GPU backends |
| **L5: Entry & Apps** | `src/tether.c`, `examples/*`, `tests/*` | Wires L0 through L4 | Application state leaking into L1-L3 |

*Exception note:* `tether_engine.c` and `tether_layout.c` read `tether_ui_get_text()` (`tether_text.h`) for text content lookup only. Do not pull compound widgets (`Button`, `Dropdown`, `Slider`, etc.) into L2 or L1.

---

## 2. Core Engineering Invariants

1. **C portability**: CMake requests C99. Prefer portable C, document platform-specific code, and validate supported compilers; the current CI covers Windows.
2. **Memory ownership**: Avoid unnecessary allocations in frame, layout, and input paths. Document allocation and failure behavior, and release owned memory on destruction and shutdown. Existing operations can allocate at runtime.
3. **Never Hold Raw ECS Pointers Across Mutations**: `tether_ecs_remove_component` uses swap-and-pop. Any `tether_ecs_add_component`, `remove_component`, or `destroy_entity` call can relocate elements in the dense array. Re-query `tether_ecs_get_component()` using `Tether_GUID` instead of caching pointers.
4. **Layout vs. Render Separation**: Geometry goes in `Tether_SlotTransform`; visual GPU offsets (`translation`, `scale`, `rotation`, `opacity`) go in `Tether_RenderTransform` without dirtying `TETHER_COMPONENT_DIRTY_LAYOUT`.
5. **C and YAML**: Keep supported YAML widget properties consistent with the C APIs and validate error handling. YAML does not yet expose every C operation.

---

## 3. Build, Test & Sanitizer Verification

```bash
# Clone with submodules (Chitra, libyaml)
git clone --recurse-submodules https://github.com/Yuvi-GD/Tether.git
cd Tether

# Configure & build (MSVC multi-config)
cmake -S . -B build -DTETHER_BUILD_TESTS=ON -DTETHER_BUILD_RUNNERS=ON -DTETHER_BUILD_EXAMPLES=ON
cmake --build build --config Debug
cmake --build build --config Release

# Run automated tests
ctest --test-dir build -C Debug -L correctness --output-on-failure
ctest --test-dir build -C Release --output-on-failure
```

For memory/ECS/parser/input changes, also validate under AddressSanitizer (matches `.github/workflows/ci.yml`):
```bash
cmake -S . -B build/asan -DTETHER_ENABLE_ASAN=ON -DTETHER_BUILD_SHARED=OFF -DTETHER_BUILD_EXAMPLES=OFF -DTETHER_BUILD_RUNNERS=OFF -DTETHER_BUILD_TESTS=ON -DTETHER_ENABLE_INSTALL=OFF
cmake --build build/asan --config Debug --target tether_test tether_ecs_test tether_report_test tether_behavior_test tether_yaml_ast_test tether_yaml_load_test
ctest --test-dir build/asan -C Debug -L correctness --output-on-failure
```

Add behavioral checks to `tests/unit/behavior_test.c` or YAML load/rollback cases to `tests/integration/yaml_load_test.c` for every new feature or bug fix.

---

## 4. Pull Request & Commit Guidelines

- **Branch off `main`**: Never commit directly to `main`. Use focused branches (`feat/<topic>`, `fix/<topic>`, `docs/<topic>`).
- **Keep PRs focused**: One subsystem/feature/fix per PR. Do not mix formatting sweeps with functional changes.
- **Prefix commits by subsystem**: `core:`, `engine:`, `ui:`, `yaml:`, `sandbox:`, `tests:`, `docs:`, `ci:`.
- **Before marking ready**: Zero warnings in Debug and Release, all `correctness` + ASan checks green, and matching updates to docs/headers if public APIs changed.

---

## 5. High-Impact Contribution Areas

- **Widgets** (`src/ui/widgets/`): `TextInput`, `ScrollView`, `Tooltip`, keyboard/focus navigation.
- **Backends** (`src/backends/`): SDL3, ThorVG, Raylib, or pure CPU software rasterizer adapters against `Tether_Backend`.
- **CI Portability**: Extending `.github/workflows/ci.yml` to cover Linux (GCC/Clang) and macOS builds.
- **YAML & Diagnostics**: Expanding schema checks and error reporting in `tether_yaml_widgets.c`.
