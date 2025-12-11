#!/usr/bin/env bash
set -euo pipefail

make

echo "Starting Auto Player Server..."
./out/apServ &
APSERV_PID=$!

echo "Starting SackServ..."
./out/sackServ &
SACKSERV_PID=$!

echo "apServ PID: $APSERV_PID"
echo "sackServ PID: $SACKSERV_PID"

cleanup() {
    echo "Stopping servers..."
    trap - SIGINT SIGTERM EXIT

    for pid in "$APSERV_PID" "$SACKSERV_PID"; do
        if kill -0 "$pid" 2>/dev/null; then
            # try graceful termination
            kill "$pid" 2>/dev/null || true
        fi
    done

    sleep 1

    for pid in "$APSERV_PID" "$SACKSERV_PID"; do
        if kill -0 "$pid" 2>/dev/null; then
            kill -9 "$pid" 2>/dev/null || true
        fi
    done

    echo "Servers stopped."
}

trap cleanup INT SIGTERM EXIT

wait "$APSERV_PID" "$SACKSERV_PID" 