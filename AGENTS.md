# AGENTS.md

Guide for AI agents working in this repository. FOORT (Flexible Object Oriented Ray Tracer) is a C++17
general-relativistic ray tracer, configured entirely via `.cfg` files, with a Python post-processing
toolkit in `PostProc/`. See `README.md` for install/build instructions and `docs/Documentation.pdf` for
user-facing documentation.

## Repository layout

```
FOORT/src/            C++ source (see "Core architecture" below)
FOORT/test/           GoogleTest unit tests
FOORT/cfgs/           Example/sample .cfg run configs (most are personal/untracked; some allow-listed)
FOORT/Output/         Sample + generated output (.dat files, images); mostly untracked, some allow-listed
FOORT/bin/            Build output (gitignored)
FOORT/BosonStar/      Numerical (non-rotating) boson star metric data (not committed)
FOORT/RotatingBosonStar/  Numerical rotating boson star metric data (not committed, often large)
PostProc/             Python post-processing: pyFOORT.py, PhotonRingInterferometry.py,
                       ExampleNotebook.ipynb, environment.yml, Doxyfile
docs/                 Committed Doxygen output: docs/html (C++), docs/postproc/html (Python),
                       docs/Documentation.pdf (user docs), docs/latex/refman.pdf
cmake/                Custom CMake modules (Doxygen.cmake, Testing.cmake)
alternative_builds/   Windows .sln, old Makefiles, macOS OpenMP workaround script
Benchmarking/          Scratch analysis notebooks
```

## Build system

- CMake-driven: `cmake -S . -B build && cmake --build build`. Executable lands in `FOORT/bin/`.
- **macOS**: Apple Clang has no OpenMP. If `find_package(OpenMP)` fails, `source alternative_builds/MacOS_workaround.sh`
  (switches `CC`/`CXX` to Homebrew `gcc-14`) and reconfigure.
- `FOORT_BUILD_TESTS` option (default `ON`) gates the GoogleTest `FetchContent` + `FOORT/test` subdirectory —
  turn `OFF` to configure without network access.
- `FOORT/src/CMakeLists.txt` declares ~15 small static libraries with some intentionally circular
  `target_link_libraries` dependencies (CMake resolves this for static libs by repeating them on the link
  line). This is pre-existing and tolerated; don't be alarmed by it, but don't add new cycles gratuitously.
- Tests: `ctest --test-dir build`.
- Docs: `cmake --build build -t doxygen` (C++ → `docs/html`) or run `doxygen` inside `PostProc/` (Python → `docs/postproc/html`).

## Runtime model

Single executable, entirely config-driven: `FOORT/bin/FOORT <path/to/config.cfg>` (libconfig syntax; see
`FOORT/cfgs/*.cfg` for examples). `Config.cpp` parses the file and constructs a `Metric`, `Source`,
`Diagnostics` (bitflag-selected), `Terminations` (bitflag-selected), `Integrator`, and `ViewScreen` (+ `Mesh`);
`Main.cpp` wires these together and runs the integration loop.

## Core architecture (`FOORT/src/`)

| File(s) | Role |
|---|---|
| `Metric.h/.cpp` | Abstract `Metric` base + subclasses: analytic (Kerr-like, Johannsen) and numerical grid-based (`BosonStarMetric`, `RotatingBosonStarMetric`, via `Grid` + `BicubicSplineInterpolator`) |
| `Geodesic.h/.cpp` | Geodesic equation right-hand side |
| `Integrators.h/.cpp` | RK4 / Verlet integration schemes |
| `Diagnostics.h/.cpp`, `DiagnosticsEmission.h/.cpp` | What gets measured per geodesic (four-color screen, equatorial passes/emission, geodesic position, closest radius, ...); fluid-velocity and emission models for disc-style diagnostics |
| `Terminations.h/.cpp` | Stopping conditions (horizon, boundary sphere, timeout, NaN, theta singularity, ...) |
| `ViewScreen.h/.cpp`, `Mesh.h/.cpp` | Maps screen pixels to initial geodesic conditions; `Mesh` handles (adaptive) pixel scheduling/subdivision |
| `InputOutput.h/.cpp` | `ScreenOutput()` (console, level-filtered) and `GeodesicOutputHandler` (writes `.dat` output, splitting across files at `GeodesicsPerFile`) |
| `Grid.h/.cpp`, `Interpolator.h/.cpp` | 2D grid + bicubic spline interpolation, backing the numerical metrics |
| `Config.h/.cpp`, `ConfigReader.h/.cpp` | Config-file parsing (libconfig wrapper) and object construction |
| `Utilities.h/.cpp` | Misc (timer, etc.) |

### Extension points

Adding a new `Metric` / `Diagnostic` / `Termination` follows an in-source, multi-step pattern marked with
`//// <TYPE> ADD POINT <letter> ////` comments. Grep for `ADD POINT` to find every site needing a matching edit:

- **Metric**: A (`Metric.h`, class declaration) → B (`Config.cpp`, `GetMetric()` dispatch)
- **Diagnostic**: A1/A2 (`Diagnostics.h`) → B (`Diagnostics.h`, bitflag) → C (`Diagnostics.cpp`) → D.1/D.2 (`Config.cpp`)
- **Termination**: A1/A2 (`Terminations.h`) → B1/B2 (`Terminations.h`, bitflag) → C (`Terminations.cpp`) → D.1/D.2 (`Config.cpp`)

## Conventions

- **Console output**: prefer `ScreenOutput(message, OutputLevel::Level_X)` over raw `std::cout`/`std::cerr` for
  new code — it's filtered by the config's `ScreenOutputLevel`. Some pre-existing raw `std::cout`/`std::cerr`
  calls remain (e.g. `Grid.cpp`, `Metric.cpp`, `Interpolator.h`); match `ScreenOutput` for anything new rather
  than following those.
- **Invalid/missing input** (bad config values, missing data files): `throw std::runtime_error(...)`, don't
  fail silently. This is the established convention for `Grid`/`Interpolator` file loading and physics
  parameter validation.
- **Ownership**: dynamically-allocated members use `std::unique_ptr`, not raw `new`/`delete`.
- **Naming**: PascalCase for C++ identifiers and for Python function parameters in `PostProc/*.py`
  (e.g. `FilePrefix`, `Verbose`) — don't introduce snake_case parameter names there.
- **`.gitignore` gotcha**: `FOORT/cfgs/*`, `FOORT/Output/*`, `PostProc/*` etc. are broadly ignored with
  explicit `!`-exceptions carved out for specific tracked sample files. Before assuming a new file under
  one of these paths will be tracked, check for (or add) a matching `!`-exception — otherwise `git add`
  silently refuses it as ignored.
- **Numerical metric data** (`FOORT/RotatingBosonStar/<dataset>/`, `FOORT/BosonStar/*.dat`) is not committed
  (too large / not always publicly shareable). `RotatingBosonStarMetric`/`BosonStarMetric` will throw at
  construction if this data isn't present locally.

## Python post-processing (`PostProc/`)

- `pyFOORT.py` — load raw FOORT `.dat` output into pandas, convert to image grids, plotting helpers.
- `PhotonRingInterferometry.py` — radon transform → complex visibility → visibility amplitudes.
- `ExampleNotebook.ipynb` — worked Kerr-vs-Johannsen example; requires `FOORT/cfgs/KerrExample.cfg` and
  `JohannsenExample.cfg` to have been run first (see README) to populate `FOORT/Output/`.
- `environment.yml` — conda env (`foort-postproc`): `conda env create -f PostProc/environment.yml`.

## Tests

GoogleTest-based, in `FOORT/test/`. Run with `ctest --test-dir build`. Tests requiring non-public
`RotatingBosonStar` data are intentionally excluded from the suite.
