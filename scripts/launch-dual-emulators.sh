#!/bin/bash
# Launch HTTP servers for both emulators
# Usage: ./scripts/launch-dual-emulators.sh

# Kill any existing servers on these ports
lsof -ti:8000 -ti:8001 | xargs kill 2>/dev/null || true

# Check if builds exist
if [ ! -d "build-sim-classic" ] || [ ! -d "build-sim-custom" ]; then
    echo "Error: Emulator builds not found."
    echo "Please run: ./scripts/build-dual-emulators.sh"
    exit 1
fi

echo "Starting emulator servers..."
echo

# Launch servers in background
python3 -m http.server -d build-sim-classic 8000 &
CLASSIC_PID=$!
python3 -m http.server -d build-sim-custom 8001 &
CUSTOM_PID=$!

sleep 1

echo "✓ Emulator servers running:"
echo "  Classic LCD (port 8000): http://localhost:8000/firmware.html"
echo "  Custom LCD  (port 8001): http://localhost:8001/firmware.html"
echo
echo "Server PIDs: $CLASSIC_PID (classic), $CUSTOM_PID (custom)"
echo "Press Ctrl+C to stop both servers"
echo

# Wait for both processes
wait $CLASSIC_PID $CUSTOM_PID
