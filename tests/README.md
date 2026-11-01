# Tether validation

This directory owns correctness tests, controlled workloads, benchmark runners,
and their shared reporting code. The sandbox remains a frontend: it launches an
independent executable and imports its completed report. The workload owns its
engine, graphics context, and measurements. Closing the frontend does not cancel
a detached run. Other applications can still compete for the same hardware.

Three activities have different purposes:

- **Correctness tests** check explicit assertions and return a failing exit code
  when a contract breaks. CTest can run these without the sandbox.
- **Stress runs** increase counts or churn to investigate resource limits. An
  accepted input is not evidence that the requested capacity works.
- **Benchmarks** record timings for a specified workload. Successful completion
  does not establish a performance target or prove every engine feature correct.

## Run validation

From the repository root, with the existing Chitra and libyaml dependencies and
a Windows MSVC development environment:

```powershell
cmake -S . -B build -DTETHER_BUILD_TESTS=ON -DTETHER_BUILD_RUNNERS=ON -DTETHER_BUILD_EXAMPLES=ON
cmake --build build --config Debug --parallel 4
ctest --test-dir build -C Debug -L correctness --output-on-failure --no-tests=error
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release -L correctness --output-on-failure --no-tests=error
```

Graphics checks require an interactive desktop and functioning graphics driver:

```powershell
cmake -S . -B build -DTETHER_REGISTER_RENDER_TESTS=ON
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release -L rendering --output-on-failure --no-tests=error
```

Use `-L 'correctness|rendering'` for the complete registered Tether suite. Dependency
tests may also appear in unfiltered CTest output. Builds never run benchmarks.
The Lab launches configurable workload scenarios; it does not expose every unit
test as a button. CTest is the full-suite entry point.

After initial configuration, build only the target needed for iteration, for
example `cmake --build build --config Release --target tether_sandbox --parallel 8`.
Source fingerprints and Git identity refresh during the build; ordinary source
edits do not require a manual CMake configure step. Changes to CMake options or
compiler settings still require reconfiguration.

## Run a controlled workload

```powershell
./build/Release/tether_bench.exe --scenario render --mode retained --count 10000 --spawn 32 --delete 32 --mutate 100 --frames 120 --warmup 30 --fps 60 --seed 1 --memory-mib 1024
```

Use `tether_test.exe --help` or `tether_bench.exe --help` for all inputs. Frame count
is a hard sample limit; an optional time limit can stop earlier. CPU scenarios
ignore rendering mode, FPS, and warm-up. Output includes JSON samples and a readable
`.json.txt` summary. Import the JSON through Lab & Results to see the value cards.
Without `--output`, the runner creates `report` beside its executable and chooses
a timestamp and process-ID filename. With `--output`, it creates any missing parent
folders and replaces that chosen filename. Relative explicit paths are relative
to the caller's working directory. CLI output prints the saved path and a readable
summary.

The explicit standard matrix runs fixed-count ECS mutation, ECS churn, and a
25%-of-population mutation profile, plus optionally rendering in both modes at
1K, 10K, and 100K objects, three trials each:

```powershell
./tests/benchmarks/run_standard.ps1 -BinaryDirectory build/Release -IncludeRendering
```

Follow your machine's script-execution policy. `-Counts`, `-Trials`, `-Frames`,
`-MemoryMiB`, and output-directory parameters control this script. Results and its
manifest go in a unique subdirectory of `build/Release/report/benchmarks` for the
Release executable. Workbench runs use `build/Release/report/workbench`; registered
correctness and rendering workload reports use `build/Release/report/correctness`
and `build/Release/report/rendering`. Substitute `Debug` when using Debug binaries.
Those CTest filenames are reused and subsequent runs overwrite them. A separate
build directory has its own executable-side `report` directory. Direct CLI runs
can explicitly choose another path. Counts through
10M remain configurable, subject to memory and representation checks; this is
not a verified 10M performance claim.

For isolated Tether layout timing, use `tether_bench --scenario layout --count N
--frames N`. This CPU-only workload builds a nested column/row scene, changes
the root and one child size before each pass, and times only
`tether_layout_process_all`. `count` requests leaf panels; the actual active count
also includes row containers and the root. It does not invoke Vinyasa.
The repeatable layout matrix uses the same script with `-OnlyLayout` and writes
its own manifest under `build/Release/report/benchmarks`.

## Organization and reading order

| Location | Responsibility |
| --- | --- |
| `unit/` | CPU contracts and ECS checks |
| `integration/` | Parser, report integrity, and registered lifecycle scenarios |
| `rendering/` | Desktop-only geometry, clipping, rendering, and mode checks |
| `runners/` | Executable entry point and exit/report lifecycle |
| `workloads/` | Engine setup, controlled operations, sampling, cleanup |
| `support/` | Options, OS/process operations, report reader/writer and metadata |
| `benchmarks/` | Benchmark target and explicitly invoked repeatable matrix |
| `docs/` | Methodology, coverage, verified results, and remaining gaps |

Read [measurement methodology](docs/methodology.md) before interpreting numbers,
then [coverage and verification](docs/verification.md) for what has actually run.
Generated reports stay in build directories; frontend code stays in
`examples/sandbox`. The root `.clang-format` is the single formatting configuration.

For AddressSanitizer, configure a separate build with `TETHER_ENABLE_ASAN=ON`,
`TETHER_BUILD_TESTS=ON`, and shared libraries/examples/runners/install disabled.
Build `tether_test tether_ecs_test tether_behavior_test tether_report_test
tether_yaml_ast_test`, then run the correctness CTest label. This configuration
currently supports MSVC and requires its ASan runtime. It does not enable leak,
undefined-behavior, or thread sanitizers.
