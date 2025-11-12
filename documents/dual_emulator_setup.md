# Dual Emulator Setup for LCD Testing

## Overview

When developing watch faces that need to support both classic and custom LCD types, it's useful to run two emulators simultaneously to verify display correctness on both screens. This guide explains how to set up and run parallel emulators for the F-91W (classic) and A158WEA-9 (custom) LCD types.

## Why Two Emulators?

Sensor Watch supports two LCD types with different capabilities:

- **Classic LCD (F-91W)**: 2-character top left, no decimal point
- **Custom LCD (A158WEA-9)**: 3-character top left, has decimal point segment

Watch faces should adapt their display to work correctly on both types. Running both emulators side-by-side allows immediate visual verification of display logic.

## Directory Setup

Create separate directories for each emulator build:

```bash
mkdir -p build-sim-classic
mkdir -p build-sim-custom
```

These directories will hold the emulator builds for each LCD type.

## Building for Both LCD Types

### Important: Clean Between Builds

The build system caches the `DISPLAY` flag, so you **must** run `make clean` between builds for different LCD types, otherwise both builds will use the same LCD type.

### Build Classic LCD Emulator

```bash
emmake make clean
emmake make BOARD=sensorwatch_red DISPLAY=classic
cp build-sim/* build-sim-classic/
```

### Build Custom LCD Emulator

```bash
emmake make clean
emmake make BOARD=sensorwatch_red DISPLAY=custom
cp build-sim/* build-sim-custom/
```

### Verify Different Builds

Confirm that the builds are actually different:

```bash
md5 build-sim-classic/firmware.wasm build-sim-custom/firmware.wasm
```

You should see **different** checksums. If they're identical, you forgot to run `make clean` between builds.

## Running Both Emulators Simultaneously

Start HTTP servers on different ports for each emulator:

```bash
# Terminal 1 - Classic LCD on port 8000
python3 -m http.server -d build-sim-classic 8000

# Terminal 2 - Custom LCD on port 8001
python3 -m http.server -d build-sim-custom 8001
```

Or run both in the background:

```bash
python3 -m http.server -d build-sim-classic 8000 &
python3 -m http.server -d build-sim-custom 8001 &
```

## Accessing the Emulators

Open two browser windows/tabs:

- **Classic LCD**: http://localhost:8000/firmware.html
- **Custom LCD**: http://localhost:8001/firmware.html

Position them side-by-side for easy comparison.

## Testing Workflow

### 1. Make Code Changes

Edit your watch face code (e.g., `watch-faces/complication/tomato_face.c`)

### 2. Rebuild Both Emulators

Use this combined command to rebuild, verify checksums, restart servers:

```bash
emmake make clean && \
emmake make BOARD=sensorwatch_red DISPLAY=classic && \
cp build-sim/* build-sim-classic/ && \
emmake make clean && \
emmake make BOARD=sensorwatch_red DISPLAY=custom && \
cp build-sim/* build-sim-custom/ && \
md5 build-sim-classic/firmware.wasm build-sim-custom/firmware.wasm && \
lsof -ti:8000,8001 | xargs kill 2>/dev/null; \
sleep 1 && \
python3 -m http.server -d build-sim-classic 8000 & \
python3 -m http.server -d build-sim-custom 8001 & \
sleep 2 && \
echo "Emulators rebuilt and running!"
```

### 3. Refresh Browsers

**Important**: You must perform a hard refresh to load the new firmware:

- **Mac**: Cmd+Shift+R
- **Windows/Linux**: Ctrl+Shift+F5
- **Alternative**: Open DevTools, right-click the refresh button, select "Empty Cache and Hard Reload"

A regular refresh (F5 or Cmd+R) will NOT work because browsers cache the WebAssembly files.

### 4. Test Both Displays

Navigate through your watch face and verify:
- Character positions and alignment
- Phase indicators and labels
- Time/duration formatting
- Special characters (Roman numerals, etc.)
- Colon vs. decimal point usage

## Common Issues

### Same Display on Both Emulators

**Symptom**: Both emulators show the same LCD type (usually classic)

**Cause**: Forgot to run `make clean` between builds

**Fix**: Run the full rebuild command above with `make clean` between builds

### Browser Shows Old Firmware

**Symptom**: Changes don't appear after rebuild

**Cause**: Browser cached the old WebAssembly files

**Fix**: Hard refresh (Cmd+Shift+R) or clear browser cache

### Servers Won't Start (Port Already in Use)

**Symptom**: Error "Address already in use"

**Cause**: Previous server processes still running

**Fix**: Kill existing servers:
```bash
lsof -ti:8000,8001 | xargs kill
```

## Example: Testing Phase Preview Display

When testing the tomato_face phase preview, you should see:

**Classic LCD (F-91W)**:
```
F{ 4  25 0
```
- Top left: `F{` (Focus, preset 1 with Roman numeral I)
- Top right: ` 4` (4 cycles)
- Bottom: `25 0` (25 minutes, count 0)

**Custom LCD (A158WEA-9)**:
```
FO1  4  25 0
```
- Top left: `FO1` (Focus, preset 1)
- Top right: ` 4` (4 cycles)
- Bottom: `25 0` (25 minutes, count 0)

## Tips

1. **Keep terminal output visible**: The build process shows compilation errors and the `md5` checksums help verify different builds

2. **Use browser bookmarks**: Save both emulator URLs as bookmarks for quick access

3. **Position windows**: Arrange your IDE, terminal, and both emulator windows in a tiled layout for efficient testing

4. **Check console logs**: Open browser DevTools console (F12) to see `printf()` debug output from your watch face

5. **Test all states**: Don't just test the default state - test all modes, phases, and edge cases on both displays

## Automated Testing Script

Save this as `test-both-lcds.sh` for quick rebuilds:

```bash
#!/bin/bash
set -e

echo "Building classic LCD..."
emmake make clean
emmake make BOARD=sensorwatch_red DISPLAY=classic
cp build-sim/* build-sim-classic/

echo "Building custom LCD..."
emmake make clean
emmake make BOARD=sensorwatch_red DISPLAY=custom
cp build-sim/* build-sim-custom/

echo "Verifying different builds..."
md5 build-sim-classic/firmware.wasm build-sim-custom/firmware.wasm

echo "Restarting servers..."
lsof -ti:8000,8001 | xargs kill 2>/dev/null || true
sleep 1
python3 -m http.server -d build-sim-classic 8000 &
python3 -m http.server -d build-sim-custom 8001 &
sleep 2

echo "✓ Classic: http://localhost:8000/firmware.html"
echo "✓ Custom:  http://localhost:8001/firmware.html"
echo "Remember to hard refresh (Cmd+Shift+R)!"
```

Make it executable:
```bash
chmod +x test-both-lcds.sh
```

Run it:
```bash
./test-both-lcds.sh
```

## See Also

- [Tomato Face Documentation](tomato_face.md) - Main tomato_face documentation
- [CLAUDE.md](../CLAUDE.md) - Project overview and build instructions
- Sensor Watch emulator documentation: https://www.sensorwatch.net/docs/emulator/
