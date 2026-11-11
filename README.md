# Welcome To FOORT!

The code files are all in the `FOORT/src` subfolder.

## QUICK INSTALL GUIDE

### LINUX

#### Requirements

- CMake >= 3.21
- A C++17 compiler whose standard library provides `<filesystem>`, e.g. GCC >= 8
- OpenMP
- Network access at configure time to fetch GoogleTest, or configure with `-DFOORT_BUILD_TESTS=OFF`

#### CMake

The executable can be built with CMake, which should be installed first (for regular Makefiles, see further). The advantage of this method it that, in principle, CMake
will automatically detect your relevant compilers. Furthermore, it will enable you to generate the automated documentation and run the tests, if needed.

One can then run

```
cmake -S . -B build
cmake --build build
```

which will cause the executable to be stored in the `FOORT/bin` subdirectory. CMake can be cleaned by simply removing the relevant folders

```
rm -rf build
```

#### CMake on clusters / supercomputers

Typically, compiling with CMake on an HPC-cluster requires loading the relevant CMake module. This code has been succesfully compiled on CSD3 (Cambridge) and Cosma7 (Durham) using the latest CMake modules.

One could get an error if the version of CMake is not recent enough. Likewise, the default compiler on a cluster is often too old (e.g. `gcc` 7.5 lacks the `<filesystem>` header); CMake will then stop at configure time with an explanatory error. In that case, load a newer compiler module (GCC >= 8) and reconfigure in a fresh build directory (`rm -rf build`), since CMake caches the compiler it first detected.

On LUMI, for example, the following works (loading `gcc-native` alone is not enough, as CMake still picks up the system compiler unless pointed to the newer one explicitly):

```
ml LUMI/25.03 buildtools gcc-native
cmake -S . -B build -DCMAKE_CXX_COMPILER=/opt/cray/pe/gcc-native/14/bin/g++
cmake --build build
```

As a last resort, if no compiler with `<filesystem>` support is available (e.g. only GCC 7), the code can still be built by replacing `#include <filesystem>` with `#include <experimental/filesystem>` (and `std::filesystem` with `std::experimental::filesystem`) in `FOORT/src/InputOutput.cpp`, linking against `stdc++fs`, and configuring with `-DFOORT_SKIP_FILESYSTEM_CHECK=ON`. This is not supported and may need further manual adjustments.

#### Old MakeFiles

Alternatively, old Makefiles can be found in `alternative_builds`. These can be system dependent, however, but should be adaptable to your needs. If the user wants to use these, the relevant makefile should be copied into the `src` folder, in which

```
make -f <makefile>
```

should be run, where `<makefile>` should be replaced with the name of the relevant file.

### WINDOWS

FOORT builds natively on Windows with the MinGW-w64 GCC compiler from MSYS2
(no Visual Studio or WSL needed). The steps below use VS Code, but any
terminal works as well.

1. Install Git for Windows (https://git-scm.com), accepting the defaults.

2. Install MSYS2 (https://www.msys2.org) in the default location `C:\msys64`.
   Open "MSYS2 UCRT64" from the Start menu and run:

   ```bash
   pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-gdb
   ```

   Press Enter to confirm. If downloads fail with error 429 (rate limiting),
   wait a few minutes and run the command again. If it persists, run the
   following and retry:

   ```bash
   sed -i 's/^#\?ParallelDownloads.*/ParallelDownloads = 1/' /etc/pacman.conf
   ```

3. Add `C:\msys64\ucrt64\bin` to your PATH. This is needed both to find the
   compiler and for `FOORT.exe` to find its runtime DLLs (`libgomp-1.dll`,
   `libstdc++-6.dll`). In PowerShell:

   ```powershell
   [Environment]::SetEnvironmentVariable('Path', [Environment]::GetEnvironmentVariable('Path','User') + ';C:\msys64\ucrt64\bin', 'User')
   ```

   Open a new PowerShell window and check that `g++ --version` and
   `cmake --version` work.

4. Install VS Code with the "C/C++" and "CMake Tools" extensions.

5. Clone the repository (Source Control → Clone Repository) and open its
   top-level folder (the one containing this README).

6. When CMake Tools asks for a kit, choose
   "GCC ... x86_64-w64-mingw32 (ucrt64)". Then build with F7, or with
   "CMake: Build" from the Command Palette (Ctrl+Shift+P). The executable is
   written to `FOORT\bin\FOORT.exe`.

7. Run from the `FOORT` subfolder, for example:

   ```powershell
   cd FOORT
   .\bin\FOORT.exe cfgs\KerrExample.cfg
   ```

   Run the tests with `ctest --test-dir build` (from the top-level folder).

**Troubleshooting**

- "libgomp-1.dll was not found": `C:\msys64\ucrt64\bin` is not on PATH, or the
  terminal/VS Code was opened before PATH was changed. Restart it.
- CMake errors mentioning "NMake" or "Visual Studio" generators: select the
  ucrt64 GCC kit, make sure Ninja is installed, and run
  "CMake: Delete Cache and Reconfigure".


### MAC

The same requirements and CMake commands as for Linux apply. However, Apple Clang (the default compiler on macOS) does not support OpenMP, so a GCC from Homebrew is needed. If CMake is run with Apple Clang, configuration stops with an error that OpenMP was not found; the workaround below resolves it.

#### MacOS Troubleshooting

Install GCC with Homebrew (`brew install gcc`), then point CMake to it and to the macOS SDK that this GCC was built for
(see also [this note](https://gist.github.com/scivision/d69faebbc56da9714798087b56de925a)):

```
export CC=$(brew --prefix)/bin/gcc-15      # use the version you installed, e.g. gcc-14
export CXX=$(brew --prefix)/bin/g++-15
$CXX -v 2>&1 | grep -o 'with-sysroot=[^ ]*'  # the SDK this GCC was built for, e.g. .../MacOSX15.sdk
export SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX15.sdk
```

Use the SDK that GCC reports rather than the newest one (`xcrun --show-sdk-path`): after a Command Line Tools update, the newest SDK can contain headers that an older GCC cannot parse, giving errors such as `expected constructor, destructor, or type conversion before '(' token` in `mach/message.h`. If that SDK is not installed, upgrade GCC (`brew upgrade gcc`).

For convenience, `alternative_builds/MacOS_workaround.sh` does this automatically, picking the newest Homebrew GCC and its matching SDK. From the top-level folder, run

```
source alternative_builds/MacOS_workaround.sh
rm -rf build
cmake -S . -B build && cmake --build build
```

The `rm -rf build` is needed if CMake was previously configured with Apple Clang. If this does not work, one can also revert to the `makefile_mac` in `alternative_builds` (see Old MakeFiles above).

## OUTPUT

FOORT writes one plain-text file per diagnostic (e.g. `Output/KerrExample_FourColorScreen.dat`), split over several numbered files (`..._2.dat`, ...) if there are more than `GeodesicsPerFile` geodesics. If `FirstLineInfo` is on, the first line describes the run (metric, diagnostics, terminations, screen); every following line is one geodesic, `x y` (its pixel on the screen) followed by that diagnostic's output values. The Python package in `PostProc` (`pyFOORT.py` and `pyFOORTInterferometry.py`) loads these files, turns them into images, and computes visibility amplitudes; the example notebook below shows how to use it.

### Example Notebook

A small example notebook, `PostProc/ExampleNotebook.ipynb`, visualizes the Kerr and Johannsen examples and compares the Kerr shadow with the analytic critical curve. These examples first need to be run: first, compile the FOORT binary as per the instructions above. Then, from the root directory, do

```
cd FOORT
bin/FOORT cfgs/KerrExample.cfg
bin/FOORT cfgs/JohannsenExample.cfg
bin/FOORT cfgs/KerrCriticalCurve.cfg
```

`KerrExample.cfg` and `JohannsenExample.cfg` each integrate about 617,622 geodesics, which takes about a minute or two on an 8-core laptop (about 5-10 CPU-minutes, so longer on fewer cores); `KerrCriticalCurve.cfg` takes a few seconds. For a quick check, `cfgs/KerrExample-lowres.cfg` runs the Kerr example on a 100 x 100 grid in a few seconds (see the notebook for how to use its output).

This populates the necessary output files in the Output subfolder. A Python environment for the post-processing scripts and notebook is provided in `PostProc/environment.yml`. Create and activate it, e.g., using conda

```
cd ../PostProc
conda env create -f environment.yml
conda activate foort-postproc
```

## Documentation

See the [Documentation](./docs/Documentation.pdf) for more information on configuration options, how to add your own Metric, Diagnostic, or Termination, and how to process output.

You can consult the `doxygen` created documentation to find out more about the structure of the code.
If you have downloaded the repository, you can achieve this by opening `docs/html/index.html` in a browser.
A PDF version of this documentation is also available in `docs/latex`.

The doxygen documentation can be generated by running

```
cmake --build build -t doxygen
```
This updates the html documentation right away. To update the LaTeX documentation, one also need to 
```
cd docs/latex
make
```

### Documentation of post-processing tools

The doxygen documentation for the files `pyFOORT.py` and `pyFOORTInterferometry.py` can be generated by running `doxygen` in the `PostProc` subfolder. This will only generate html documentation, in `docs/postproc/html`, which can be consulted by opening `docs/postproc/html/index.html` in a browser.

## Tests

The tests can be run with

```
ctest --test-dir build
```

and should all pass.
