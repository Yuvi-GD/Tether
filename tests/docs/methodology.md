# Measurement methodology

## Workload and reproduction

Each invocation uses one standalone process. Reports record requested inputs,
actual populations, per-step samples, final status and termination reason, Git
revisions, an actual-source fingerprint, dirty-tree flags, compiler/configuration,
C flags, sanitizer state, libyaml version, OS, CPU and memory information.
Source revision, dirty flags and the source fingerprint are refreshed at build
time. Compiler settings are captured at CMake configure time. Rebuild after
changing source. The timestamp is report-writing UTC time. A dirty commit hash alone is
not sufficient to reproduce a run: retain the matching source snapshot as well.

ECS workloads hold a small component value per entity. Rendering workloads use
colored panels on a fixed anchor grid, with a real Chitra graphics backend.
Each step deletes from the tail, appends new entities, then mutates a prefix.
The seed controls colors, not a randomized operation schedule. The fixed-count
matrix profiles use 100 mutations per step; churn also deletes and spawns 32.
The `ecs-fraction-25` profile mutates `ceil(0.25 * requested_count)` objects per
step, with no spawn/deletion. Both requested and actual mutation counts are in
its manifest and report. This profile scales the amount of measured work with
population, unlike the fixed-count profiles.

The isolated layout scenario uses Tether's own layout algorithm. Its scene is a
root column containing rows of up to 25 fixed-size leaf panels. Each step changes
the root width and one leaf width before calling `tether_layout_process_all`.
`layout_ms` sums only those processing calls; scene creation, resize/invalidation
setters, checks and cleanup fall outside the isolated timer. The requested count
is leaf panels, while active count includes rows and root. No Chitra rendering or
Vinyasa work is included.

Rendered runs force backend presentation in both modes. Retained invalidation
remains selective; this is not an idle-window power benchmark. The grid is based
on planned capacity and becomes subpixel at large populations. A million allocated
panels does not mean a million separately discernible screen elements.

Run Release without sanitizers for performance comparisons. Keep machine, power
mode, display/viewport, driver, workload parameters and background activity stable.
Prefer closing the launcher after starting a run. Repeat independent processes;
retain all trials, failures and raw samples. Do not choose only the fastest run.
For CPU workloads whose samples approach clock overhead, increase work per step
and duration before making comparative claims. No timing overhead is subtracted.

## Timing and counters

| Report field | Meaning and boundary |
| --- | --- |
| `phases.setup_ms` | Preflight, runner allocation, engine/backend startup and initial objects |
| `metrics.initial_create_ms` | Initial object creation, a subset of setup |
| `phases.warmup_ms` | Render initialization completion to first measured update; actual warm-up frames are also recorded |
| `samples.work_ms` | Update callback entry through post-frame callback entry: mutations, engine work and graphics submission where applicable; excludes following pacing |
| `samples.interval_ms` | Consecutive measured update-start spacing, including intervening pacing; first sample is zero; hierarchy leaves this unmeasured at zero |
| `metrics.frame_interval_mean_ms`, `frame_interval_p95_ms` | Mean and nearest-rank P95 of rendered `interval_ms` values after the first sample; `null` for CPU or single-step runs |
| `metrics.elapsed_ms` | First measured update through final sample accounting; includes intervening pacing and instrumentation |
| `metrics.process_cpu_ms` | Process user plus kernel CPU-time delta across the measured interval; not utilization percentage |
| `phases.validation_ms` | Object-validation passes; correctness passes overlap elapsed time; final benchmark validation follows measured work |
| `samples.spawn_ms/delete_ms/mutate_ms` | CPU wall time for those operation loops, already inside work time |
| `metrics.teardown_ms` | Workload object destruction, inside shutdown |
| `phases.shutdown_ms` | Workload cleanup and engine/runtime shutdown |
| `phases.total_run_ms` | Overall workload invocation, excluding report serialization |
| `rss_start_bytes`, `rss_end_bytes`, `rss_sampled_peak_bytes` | Process working set/resident memory at sampled boundaries |
| `samples.rss_bytes` | Resident memory sampled after each step; not allocator ownership or VRAM |
| `samples.draw_calls` | Observed RHI `draw_batch` calls, including scene background |
| `samples.upload_bytes` | Vertex/index upload byte arguments observed at the RHI boundary |
| `gpu_ms` | `null`: GPU execution time is not measured |
| `layout_ms` | Sum of isolated Tether layout calls in the layout scenario; `null` in other scenarios |

Phases overlap as described; **do not add them together**. CPU clock resolution
can produce zero for short intervals. CPU or resident-memory query failures are
written as JSON `null` with `measurement_state` set to `query_failed`; valid zero
remains numeric zero. Unmeasured fields use `not_measured`. The Workbench displays
**Query failed** or **Not measured** accordingly. Resident peak is sampled, not an
OS high-water mark. GPU adapter description
does not establish the active GPU or driver version. Draw/upload counters prove
submission requests, not successful GPU completion, transferred bytes, or pixels.

Mean is the arithmetic mean of `work_ms`. P50 is the lower middle sorted sample;
P95 uses nearest rank `ceil(0.95*N)`. Neither is a confidence interval. Frame-start
spacing and submission work are different measurements; neither is a GPU timer.
Frame interval describes pacing between measured update starts; it is not verified
display presentation time or an isolated GPU duration.
Older reports produced before these aggregate fields were added remain readable;
the Workbench shows their frame interval cards as **Not measured** even when raw
`samples.interval_ms` values are present.

Hierarchy correctness cycles include creation, detachment, destruction and
assertions in work time. Their operation sub-timers remain zero. They are not
comparable to the ECS microbenchmark's operation timings.

## Population, ownership and failures

`parameters.count` is the request. `initial_active`, `final_active`, `peak_active`
and sample `alive` describe workload entities, excluding engine infrastructure.
Hierarchy cycles end empty; their peak includes the temporary parent.
`logical_visible` counts workload panels with visible state. It is zero for CPU
scenarios and does not establish rasterization. `rendered_elements` is `null`.

Runner sample/item buffers are owned by one invocation and released on completion;
the executable frees samples after writing. Engine/backend allocations belong to
that process. The workload uses global callback state and is not reentrant or
thread-safe. Run separate processes for concurrency. The importer owns a bounded
snapshot and retains no pointers into parser storage or sample arrays.

Preflight estimates memory conservatively and checks representable capacity and
available RAM. It is not a hard memory quota or an allocation-failure guarantee.
Engine allocation-failure injection is not covered. Early window closure produces
`cancelled`; resource rejection produces `rejected`; assertion failures produce
`failed`. A time or step limit can complete successfully. Exit codes are 0 for
completion, 1 for assertion failure, 2 for input/resource/report errors, and 3 for
early closure. A crash may leave only an unfinished report.

The writer publishes a temporary file through replacement. Import validates the
entire stream, schema/project/kind, required counts, sequential sample indices,
sample count agreement, and finite nonnegative displayed numbers. It rejects
unfinished, truncated and contradictory success reports. Imports are limited to
64 MiB and remain synchronous. This is integrity checking, not authentication or
complete schema validation. When the launcher knows an unsuccessful OS exit code,
it rejects a report claiming success. Manual import cannot recover an exit code.

The shared JSON status `passed` means completed for a benchmark; the frontend
labels it **COMPLETED**. Correctness reports use **PASSED**. JSON is the detailed
record; the text file and result cards are summaries, not replacements for inputs,
environment and raw samples.
