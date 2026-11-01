# Coverage and verification

This record describes executed checks for the technical-preview workbench on the stated machine and build. See [methodology](methodology.md) for metric definitions.

## Correctness inventory

| CTest case | Verified behavior | Boundary |
| --- | --- | --- |
| `ecs_test` | Components, handles, hierarchy deletion | No arbitrary graph fuzzing |
| `behavior_layout` | Tether row/column sizing, gaps, alignment, resize | No exhaustive overflow matrix |
| `behavior_visibility` | Hidden/collapsed state, invalidation | No pixel comparison |
| `behavior_input` | Hit selection, hidden/ignored targets, callbacks | No keyboard/focus simulation |
| `behavior_lifetime` | Stale handles, reuse, repeated destruction | No generation-wrap or allocation-failure injection |
| `behavior_text` | String replacement, empty text, dirty flags | No headless glyph rasterization |
| `yaml_parse_recovery` | Repeated valid/invalid parsing, nesting limits | No automatic file watch |
| `report_import` | Valid, malformed, contradictory measurements | No authenticated reports |
| `lifecycle`, `hierarchy`, `empty_population`, `invalid_count` | Controlled runner work and error paths | Bounded synthetic inputs |
| `ui_contract_retained/volatile` | Real glyph creation, movement, alignment, wrapping, reveal, clipping, manual YAML reload | No pixel-perfect comparison |
| `rendering_retained/volatile` | Render submissions, churn, visibility | No GPU completion timing |
| `sandbox_launch_results` | Separate process launch and report import | Bounded smoke path |
| `sandbox_invalid_report`, `sandbox_failure_display` | Invalid and failed report presentation | Automated geometry/state checks, not visual review |

The text regression reproduced a Chitra origin mismatch: initial glyph vertices included glyph bearings, while position-only updates used the first vertex as the origin. Chitra now tracks the requested logical origin and moves vertices by the origin delta. Both retained and volatile geometry tests pass. The YAML parser regression rejects malformed partial ASTs; tested paths do not prove all allocation failures are handled.

## Local execution on 2026-11-01

| Check | Result |
| --- | --- |
| Clean Release build, CTest | **22/22 passed**, including three libyaml cases |
| Debug CTest | **22/22 passed**, including three libyaml cases |
| MSVC AddressSanitizer Debug, headless | **12/12 passed** |
| Release standard matrix | **45/45 completed**: 3 trials x 3 counts x 5 profiles |
| Release isolated layout matrix | **9/9 completed**: 3 trials x 3 counts |

The Release build used Visual Studio 2022, MSVC 19.38.33145.0, x64 Release `/O2 /Ob2 /DNDEBUG`, Windows 10.0.26300, Intel Core i9-14900HX, 32 logical processors, and 34,045,034,496 bytes of physical RAM. Chitra used Sokol D3D11; libyaml was 0.2.5. The OS reported Intel UHD Graphics as a display adapter; actual rendering device and driver were not queried. Tether base revision was `a3659d971c33b10ecc67d4478adcba25336329b3`; Chitra revision was `50ef8eed13877ba06b869cc4209f3d0f964d9d76`. Tether had uncommitted changes at build time; Chitra was clean. The Tether/tools source fingerprint was `0d03f8ec261df33ba27acda5e25ff64c7da745eaf74bf28541f473b747d06525`.

Reports sit beside the Release executables:

- `build/Release/report/benchmarks/20261101-110959-badc7d2f/manifest.json` and 45 reports.
- `build/Release/report/benchmarks/20261101-111055-9a23b300/manifest.json` and 9 layout reports.

Independent inspection found 120 samples in every report, `passed` status, zero process exit codes, no manifest failures, and positive draw submissions in every rendered sample. The 25% mutation profile performed 30,000, 300,000 and 3,000,000 mutations per run at 1K, 10K and 100K respectively. Each layout step resizes nested Tether elements and times only `tether_layout_process_all`; it does not use Vinyasa.

Observed ranges of the three per-run mean work times, in milliseconds:

| Profile | 1K | 10K | 100K |
| --- | ---: | ---: | ---: |
| ECS fixed mutate | 0.000318-0.000411 | 0.000318-0.000752 | 0.000323-0.000386 |
| ECS churn | 0.003665-0.003910 | 0.003474-0.003806 | 0.003804-0.003973 |
| ECS mutate 25% | 0.000599-0.000609 | 0.004898-0.005539 | 0.050083-0.055395 |
| Render retained | 0.904366-1.317077 | 4.321488-4.885316 | 11.488252-13.646277 |
| Render volatile | 1.178351-1.971221 | 5.069383-7.005655 | 13.396792-14.058979 |
| Nested layout resize | 0.061318-0.062006 | 0.651617-0.740432 | 6.682073-7.139788 |

These are local observations, not capacity or frame-rate guarantees. Fixed ECS mutate timings are too short for strong comparative claims. Rendering covers CPU submission, not GPU execution. Layout timing excludes creation and rendering. The 1K/10K/100K matrix is verified; 1M and 10M remain configurable stress inputs, not verified capacities.

## Remaining checks and deferred work

The sandbox supports manual YAML refresh, not automatic watched-file hot reload. An automatic watch integration test requires a new feature, outside this validation-only phase. Manual reload and parser tests remain.

This record contains no remote GitHub Actions result. [CI](../../.github/workflows/ci.yml) defines Windows Debug, Release and MSVC ASan jobs; [rendering workflow](../../.github/workflows/rendering.yml) requires a provisioned desktop runner. Manual Workbench inspection was reported on Windows. No screenshot is included, and manual appearance checks are separate from the automated desktop geometry tests.

Deferred: GPU timestamps/utilization/VRAM, pixel-perfect regression, allocation-failure injection and fuzzing, broader platforms, and measured 1M/10M stress runs.

The listed local correctness and benchmark checks passed. Remote CI and the deferred checks above have no result in this record. No significant correctness defect was confirmed in the tested paths.
