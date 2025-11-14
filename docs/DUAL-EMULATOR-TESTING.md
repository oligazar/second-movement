# Dual Emulator Testing for Second Movement

This document describes how to test watch faces side-by-side on both Classic and Custom LCD displays using the emulator.

## Why Dual Emulator Testing?

Second Movement supports two LCD types:
- **Classic LCD**: Original Sensor Watch display (smaller character set, limited segments)
- **Custom LCD**: Newer display with more segments and better character support

Watch faces should ideally work well on both displays, using `watch_display_text_with_fallback()` to provide appropriate text for each LCD type.

## Quick Start

### 1. Build Both Emulators

```bash
./scripts/build-dual-emulators.sh
```

This script:
- Cleans previous builds
- Builds the custom LCD emulator (sensorwatch_blue)
- Builds the classic LCD emulator (sensorwatch_red)
- Creates `build-sim-custom/` and `build-sim-classic/` directories

### 2. Launch Servers

```bash
./scripts/launch-dual-emulators.sh
```

This script:
- Kills any existing servers on ports 8000-8001
- Starts HTTP servers for both emulators
- Displays URLs to open in browser

### 3. Open in Browser

- **Classic LCD**: http://localhost:8000/firmware.html
- **Custom LCD**: http://localhost:8001/firmware.html

Open both URLs in separate browser tabs/windows to test side-by-side.

## Manual Build Process

If you prefer to build manually:

```bash
# Build custom LCD
emmake make clean BOARD=sensorwatch_blue DISPLAY=custom
emmake make BOARD=sensorwatch_blue DISPLAY=custom
cp -r build-sim build-sim-custom

# Build classic LCD
emmake make clean BOARD=sensorwatch_red DISPLAY=classic
emmake make BOARD=sensorwatch_red DISPLAY=classic
cp -r build-sim build-sim-classic

# Launch servers
python3 -m http.server -d build-sim-classic 8000 &
python3 -m http.server -d build-sim-custom 8001 &
```

## Testing Workflow

1. Make changes to your watch face code
2. Run `./scripts/build-dual-emulators.sh` to rebuild both
3. Hard refresh both browser tabs (Cmd+Shift+R / Ctrl+Shift+R)
4. Test your face on both displays

## Browser Cache Issues

If you don't see your changes after rebuilding:

1. **Hard Refresh** (clears page cache):
   - Chrome/Edge (Mac): Cmd + Shift + R
   - Chrome/Edge (Windows/Linux): Ctrl + Shift + R
   - Safari: Cmd + Option + R
   - Firefox: Ctrl + Shift + R (or Cmd + Shift + R on Mac)

2. **Developer Tools Method**:
   - Open Developer Tools (F12)
   - Right-click the refresh button
   - Select "Empty Cache and Hard Reload"

## Notes

- Build directories (`build-sim-classic/`, `build-sim-custom/`) are gitignored
- Scripts are checked into the fork's `main` branch for backup
- Feature branches remain clean (no dev tool commits)
- Rebuild time: ~30 seconds for incremental builds, ~2 minutes for clean builds
