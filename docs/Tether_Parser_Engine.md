# Tether Parser Engine

## Purpose

This document defines the long-term architecture for Tether's declarative UI parser system.

The parser must support three execution modes without duplicating UI rules:

```text
YAML / TBC / optional source adapters
            |
            v
      Parser frontend
            |
            v
     Shared Scene IR
            |
      +-----+-----+
      |           |
      v           v
 Live ECS     AOT/TBC
 loader       compiler
      |
      v
   ECS tree
```

The central rule is:

> A source format describes a scene. The ECS instantiator creates the live scene. Neither YAML nor the AOT format owns widget behavior.

This keeps YAML, compiled assets, editor previews, hot reload, and future formats aligned.

## Tether IR As The UI Recipe

The Tether intermediate representation is the main recipe for building a UI. It is not merely a temporary parser structure. It is the stable boundary between authored content and the systems that install that content into a runtime.

The IR describes facts such as:

- Which widget or component is requested.
- Which properties are assigned.
- Which children are created.
- Which existing children receive `content`.
- Which templates, slots, and scene references are expanded.
- Which registered actions or rules are requested.

The IR does not contain renderer-specific handles, ECS pointers, or executable callbacks. It can therefore be consumed by several backends:

```text
Tether IR
  -> live ECS instantiator
  -> AOT/TBC serializer
  -> editor preview
  -> hot-reload replacement
  -> cached creation plan
```

Third-party extensions should target this boundary. A component or rule can be installed into a Tether runtime without changing the YAML parser itself, as long as it provides a registered schema and the runtime behavior needed to consume it.

## Goals

The parser engine should provide:

- Human-readable YAML for development.
- A shared intermediate representation for all source formats.
- Direct ECS instantiation for live interpretation.
- A path to an offline AOT format such as `.tbc`.
- A cacheable runtime representation for JIT-like workflows.
- Reusable widget templates, slots, variables, and scene composition.
- Deterministic errors with source file and location information.
- Public APIs that remain stable while parser internals evolve.
- No parser dependency on a particular rendering backend.

## Non-Goals

The parser engine should not:

- Contain layout math.
- Render pixels.
- Own ECS component definitions.
- Know how a `Button` is implemented.
- Execute arbitrary code from a scene file.
- Require any one authoring format at runtime in production builds.
- Expose libyaml types through the public API.

## Execution Modes

### Live Interpretation

Used during development and for dynamic applications:

```text
YAML
  -> parse
  -> validate and resolve
  -> Scene IR
  -> instantiate ECS entities
  -> layout and render
```

Advantages:

- Fast iteration.
- Easy hot reload.
- Human-readable assets.
- Good editor workflow.

Costs:

- YAML parser dependency at runtime.
- Text parsing and allocation.
- More startup work.

### AOT Compilation

Used for production, embedded systems, and deployments that do not need YAML at runtime:

```text
YAML
  -> parse
  -> validate and resolve
  -> Scene IR
  -> serialize .tbc
```

Runtime:

```text
.tbc
  -> deserialize
  -> instantiate ECS entities
```

The AOT compiler should be a separate tool or build step. It should reuse the same parser frontend, validation rules, widget names, and property definitions as the live path.

The runtime should not need libyaml when loading `.tbc` assets.

### Runtime Cache / JIT-Like Mode

A full native machine-code JIT is not required for the first implementation. A cached creation plan is usually sufficient:

```text
YAML
  -> parse once
  -> resolve templates and properties
  -> cache Scene IR or a creation plan
  -> instantiate repeatedly
```

A creation plan may contain operations such as:

```text
CREATE Panel
SET Layout.flow = COLUMN
SET Style.color = ...
CREATE Text
SET Text.string = ...
ATTACH child
```

This removes repeated parsing without requiring executable memory or platform-specific JIT support.

## Layered Architecture

### Parser Frontends

A frontend understands one source format:

```text
YAML frontend
TBC reader
Optional Markdown adapter
Optional HTML/CSS adapter
Custom editor or network format
```

YAML may remain Tether's primary authoring format. Markdown and HTML/CSS are examples of optional adapters for convenient content workflows, not separate UI engines. They should lower their supported subset into the same Scene IR.

Each frontend produces the same Scene IR. A frontend must not directly call rendering code or depend on a particular layout implementation.

For example:

```text
Markdown heading  -> Text node with heading properties
HTML element      -> registered widget or content node
CSS declaration   -> registered style/layout property
YAML mapping      -> explicit Tether widget and properties
```

The supported Markdown or HTML subset should be intentional. Unsupported semantics should produce diagnostics rather than silently inventing behavior.

### Scene IR

The Scene IR is the format-neutral representation between parsing and ECS creation.

It should represent:

- Scene roots.
- Widget names.
- Property mappings.
- Child definitions.
- Existing-child content mappings.
- Widget templates.
- Slots.
- Template variables.
- Scene references.
- Event declarations.
- Source locations.

Conceptually:

```c
typedef struct Tether_Scene Tether_Scene;
typedef struct Tether_SceneNode Tether_SceneNode;
```

The first public version should use opaque handles if the IR must become public. Internal node fields should remain private so the IR can change without breaking applications.

### Third-Party Components And Rules

Dynamic ECS components provide storage, not automatic behavior. A component may be registered with a fixed ID and a known size, but it has no effect until a Tether system, widget, or rule understands it.

The extension model should separate:

```text
Component schema:
  name, ID, size, version, serialization rules

Component data:
  bytes stored by the ECS

Component rule/system:
  behavior such as rendering, animation, input, or binding
```

Unknown components may be preserved as opaque IR or asset data, but they should not silently participate in layout or rendering. Layout participation should be an explicit registered capability.

This allows different runtime profiles:

```text
Core runtime:
  engine components only

UI runtime:
  core + standard widgets and text

Application runtime:
  core + UI + application components and rules

Editor runtime:
  application runtime + diagnostics + hot reload
```

Third-party components should be installable through public registration APIs, while their implementation headers and systems remain in the third-party codebase. The parser only needs their registered schema and property names.

Shader extensions follow the same rule. The ECS component stores shader asset references and uniform data; the RHI or a registered render rule owns GPU behavior. Arbitrary executable code should never be loaded from YAML, Markdown, HTML, or TBC data.

### ECS Instantiator

The instantiator consumes Scene IR and uses the widget registry:

```text
Scene IR node
  -> widget registry lookup
  -> widget factory
  -> ECS entity
  -> property application
  -> child instantiation
```

The instantiator owns:

- Parent attachment.
- Widget factory calls.
- Property application.
- ID assignment.
- Template expansion.
- Slot injection.
- Scene composition.
- Event binding lookup.
- Visibility initialization.

It does not own layout or rendering rules.

### ECS and Rendering

After instantiation:

```text
ECS tree
  -> layout system
  -> input system
  -> render system
```

The parser stops at a valid ECS tree. It should not need to know whether the RHI is ThorVG, WebGPU, software rasterization, or another backend.

## Scene Model

A scene file may contain one or more root nodes:

```yaml
- Panel:
    id: "main_panel"

- Text:
    string: "Status"
```

A scene loaded with an invalid parent is attached to the main root by default. A scene loaded with an explicit parent is attached below that entity:

```yaml
Panel:
  Scene: "parts/status.yaml"
```

The `Scene` operation should preserve the parent supplied by the caller. It should not silently create an unrelated main-root tree.

Scene paths should eventually be resolved relative to the file that contains the reference, not only relative to the process working directory. A future resolver should handle:

- Relative paths.
- Search roots.
- Asset package paths.
- Cycle detection.
- Missing-file diagnostics.

## YAML Semantics

### Creation Versus Existing Content

The parser should keep these meanings distinct:

```yaml
children:
  - Text:
      string: "Created now"
```

`children` creates new child entities.

```yaml
content:
  - Text:
      string: "Configure existing child"
```

`content` configures direct children already created by the widget factory. It must not create missing entities.

This is important for native widgets such as `Button`, which may create an internal `Text` child as part of the factory.

### Templates

Templates should expand into Scene IR before ECS instantiation:

```yaml
- Widget:
    name: "StatCard"
    tree:
      - Panel:
          children:
            - Text:
                string: $title
            - Slot: "content"
```

Template expansion should not require a second ECS tree or a special renderer path.

### Events

YAML should refer to registered actions, not arbitrary C symbols:

```yaml
- Button:
    events:
      click:
        call: close_modal
```

The application registers `close_modal` in a controlled action registry. Unknown action names should produce a parser diagnostic.

Built-in declarative actions and application callbacks should remain separate:

```yaml
events:
  click:
    action: hide
```

```yaml
events:
  click:
    call: close_modal
```

The parser never converts untrusted strings directly into function pointers.

## Public And Private Files

### Public Headers

Only stable contracts belong under `include/`:

```text
include/tether/
    tether.h
    core/
        tether_ecs.h
        tether_components.h
    parsers/
        tether_scene.h
        tether_yaml.h
```

A public parser API may eventually look like:

```c
Tether_GUID tether_yaml_load(
    const char* filepath,
    Tether_GUID parent
);

Tether_GUID tether_scene_instantiate(
    const Tether_Scene* scene,
    Tether_GUID parent
);
```

The current convenience loader may keep the simple API:

```c
Tether_GUID tether_yaml_load(const char* filepath, Tether_GUID parent);
```

Internally it can perform:

```text
parse -> validate -> instantiate -> release temporary IR
```

### Private Source Files

Implementation details belong under `src/`:

```text
src/parsers/
    tether_yaml.c
    tether_yaml_ast.c
    tether_yaml_ast.h
    tether_scene_ir.c
    tether_scene_ir.h
    tether_scene_instantiate.c
    tether_scene_instantiate.h
    tether_tbc.c
```

The following should remain private:

- libyaml parser state.
- YAML token and document structures.
- AST allocation details.
- Temporary resolver environments.
- IR storage layout.
- Binary serialization details.
- Parser-only helper functions.

If an AST becomes public later, expose opaque types rather than internal structs:

```c
typedef struct Tether_AST Tether_AST;
typedef struct Tether_ASTNode Tether_ASTNode;
```

Do not expose libyaml types through Tether headers.

## Ownership And Memory

Every parser stage must document ownership.

Recommended ownership rules:

```text
Parser source buffer:
    Owned by parser during parse.

AST / Scene IR:
    Owned by the scene object.

Resolved strings:
    Copied into IR when they must survive parsing.

ECS data:
    Owned by ECS after instantiation.

Temporary parse environment:
    Destroyed after template expansion and instantiation.
```

The live loader may free the IR after successful ECS instantiation. The AOT compiler retains the IR until serialization completes. The editor may retain it for inspection and hot reload.

The parser must never store pointers into a temporary YAML buffer in ECS components.

## Validation And Diagnostics

Validation should happen before ECS mutation whenever possible.

Diagnostics should include:

- Source filename.
- Line and column.
- Property name.
- Widget name.
- Error category.
- Human-readable explanation.

Useful error categories include:

```text
TETHER_PARSE_INVALID_SYNTAX
TETHER_PARSE_UNKNOWN_WIDGET
TETHER_PARSE_UNKNOWN_PROPERTY
TETHER_PARSE_INVALID_VALUE
TETHER_PARSE_MISSING_SCENE
TETHER_PARSE_SCENE_CYCLE
TETHER_PARSE_UNKNOWN_TEMPLATE
TETHER_PARSE_MISSING_SLOT
TETHER_PARSE_UNKNOWN_ACTION
TETHER_PARSE_INSTANTIATION_FAILURE
```

A future public diagnostic API should avoid printing directly from parser internals. Applications should be able to choose logging, collection, or fail-fast behavior.

## Compilation And Build Boundaries

### Development Build

```text
Application
  -> Tether runtime
  -> YAML frontend
  -> libyaml
  -> Scene IR
  -> ECS
```

### Cook Build

```text
Tether cooker tool
  -> YAML frontend
  -> validation
  -> Scene IR
  -> .tbc writer
```

### Production Runtime

```text
Application
  -> Tether runtime
  -> .tbc reader
  -> ECS
```

The AOT cooker should not be linked into the production runtime unless explicitly requested.

## Registration Boundaries

The parser should use existing registries rather than hardcoded widget conditionals:

```text
"Panel"  -> Panel factory
"Text"   -> Text factory
"Button" -> Button factory
```

The parser does not need to know how a button creates its internal label. That belongs to the widget factory and the `content` contract.

Likewise, properties should eventually be registered or described by the owning widget/component layer rather than growing one giant parser `if/else` chain.

The long-term registration flow is:

```text
Third-party package
  -> registers component schema
  -> registers widget or rule
  -> optionally registers layout/render capability
  -> parser resolves names into Tether IR
  -> instantiator asks the registry to install them
```

The parser remains generic. It does not need a new hardcoded branch for every third-party component.

## Migration Plan

### Phase 1: Stabilize Current YAML Loader

- Keep `tether_yaml_load(filepath, parent)` as the compatibility API.
- Keep current AST implementation private.
- Fix scene parent attachment and path resolution.
- Keep `children` and `content` semantics explicit.
- Keep parser errors deterministic.

### Phase 2: Extract Scene IR

- Define internal `Tether_Scene` and `Tether_SceneNode` structures.
- Move YAML parsing from direct ECS mutation into IR creation.
- Add a separate IR-to-ECS instantiator.
- Preserve the existing public loader by composing both stages internally.

### Phase 3: Add Validation

- Validate widgets, properties, templates, slots, scenes, and actions before instantiation.
- Add source locations to IR nodes.
- Add structured diagnostics.

### Phase 4: Add AOT/TBC

- Serialize validated IR into a versioned `.tbc` format.
- Add a binary reader that produces the same IR or directly drives the instantiator.
- Keep the ECS and widget registry independent of the source format.

### Phase 5: Add Caching And Hot Reload

- Cache parsed IR by path and content hash.
- Support replacing a scene subtree without restarting the application.
- Preserve IDs and event bindings according to an explicit reload policy.

### Phase 6: Optional JIT Optimization

- Add cached creation plans first.
- Measure startup and instantiation cost.
- Only consider native machine-code JIT if profiling proves it necessary.
- Keep an interpreter path for platforms that prohibit executable memory.

## Design Decisions To Preserve

- YAML is a source format, not the engine's internal representation.
- Tether IR is the central recipe and extension boundary for UI construction.
- AOT and live loading must share validation and widget semantics.
- Markdown, HTML/CSS, and other formats are optional adapters into Tether IR.
- Dynamic components provide storage; registered systems provide behavior.
- ECS remains the runtime source of truth.
- Widget identity comes from the registry, not parser conditionals.
- The parser does not retain pointers into temporary source buffers.
- Public headers expose contracts; parser implementation details remain private.
- `children` creates entities; `content` configures existing children.
- Scene composition preserves the caller-provided parent.
- Unknown names and malformed values become diagnostics, not silent failures.
- A cached creation plan is preferred before implementing a native-code JIT.
