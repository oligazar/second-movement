#!/bin/bash
# Build both classic and custom LCD emulators for side-by-side testing
# Usage: ./scripts/build-dual-emulators.sh

set -e  # Exit on error

echo "Building dual emulators for Second Movement..."
echo

# Clean previous builds
echo "Cleaning previous builds..."
rm -rf build-sim build-sim-classic build-sim-custom

# Build custom LCD emulator
echo
echo "Building custom LCD emulator (sensorwatch_blue)..."
emmake make clean BOARD=sensorwatch_blue DISPLAY=custom
emmake make BOARD=sensorwatch_blue DISPLAY=custom
cp -r build-sim build-sim-custom
echo "✓ Custom LCD emulator built: build-sim-custom/"

# Build classic LCD emulator
echo
echo "Building classic LCD emulator (sensorwatch_red)..."
emmake make clean BOARD=sensorwatch_red DISPLAY=classic
emmake make BOARD=sensorwatch_red DISPLAY=classic
cp -r build-sim build-sim-classic
echo "✓ Classic LCD emulator built: build-sim-classic/"

echo
echo "=========================================="
echo "✓ Both emulators built successfully!"
echo "=========================================="
echo
echo "To launch servers:"
echo "  Classic LCD:  python3 -m http.server -d build-sim-classic 8000"
echo "  Custom LCD:   python3 -m http.server -d build-sim-custom 8001"
echo
echo "Then open in browser:"
echo "  Classic LCD:  http://localhost:8000/firmware.html"
echo "  Custom LCD:   http://localhost:8001/firmware.html"
echo
