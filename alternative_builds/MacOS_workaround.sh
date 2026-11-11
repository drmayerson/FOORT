# Source this file (`source alternative_builds/MacOS_workaround.sh`) before configuring FOORT with CMake on macOS.
# Apple Clang does not support OpenMP, so this selects the newest GCC installed with Homebrew (`brew install gcc`),
# and points it to the macOS SDK that this GCC was built for. Then `rm -rf build` and reconfigure.

if ! command -v brew > /dev/null 2>&1; then
    echo "MacOS_workaround.sh: Homebrew not found; install it from https://brew.sh and run 'brew install gcc'." >&2
    return 1 2> /dev/null || exit 1
fi

FOORT_BREW_BIN="$(brew --prefix)/bin"
FOORT_GCC_VERSION=$(ls "$FOORT_BREW_BIN"/g++-[0-9]* 2> /dev/null | sed 's/.*g++-//' | sort -n | tail -1)
if [ -z "$FOORT_GCC_VERSION" ]; then
    echo "MacOS_workaround.sh: no Homebrew GCC found in $FOORT_BREW_BIN; run 'brew install gcc'." >&2
    return 1 2> /dev/null || exit 1
fi

export CC="$FOORT_BREW_BIN/gcc-$FOORT_GCC_VERSION"
export CXX="$FOORT_BREW_BIN/g++-$FOORT_GCC_VERSION"
# Homebrew builds GCC against one specific macOS SDK (recorded as --with-sysroot); a newer SDK can contain
# headers that this GCC cannot parse. Use that SDK if it is installed, and only fall back to the active SDK otherwise.
FOORT_SDK=$("$CXX" -v 2>&1 | sed -n 's/.*--with-sysroot=\([^ ]*\).*/\1/p')
if [ -n "$FOORT_SDK" ] && [ -d "$FOORT_SDK" ]; then
    export SDKROOT="$FOORT_SDK"
else
    export SDKROOT="$(xcrun --show-sdk-path)"
    echo "MacOS_workaround.sh: SDK '$FOORT_SDK' that this GCC was built for is not installed; falling back to $SDKROOT." >&2
fi
echo "Using CC=$CC, CXX=$CXX, SDKROOT=$SDKROOT"
unset FOORT_BREW_BIN FOORT_GCC_VERSION FOORT_SDK
