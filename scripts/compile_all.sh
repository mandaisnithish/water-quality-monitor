#!/usr/bin/env bash
# Compile every sketch in this repo for its target board with arduino-cli.
# Usage: scripts/compile_all.sh [filter]     e.g. scripts/compile_all.sh firmware
# Sketches that include secrets.h are built against a copy of secrets.example.h
# (secrets.h is git-ignored, so a real one is never overwritten or committed).

set -u
cd "$(dirname "$0")/.."

UNO=arduino:avr:uno
NANO=arduino:avr:nano
NODEMCU=esp8266:esp8266:nodemcuv2

SKETCHES="
$UNO      firmware/sensor_node_uno
$NANO     firmware/sensor_node_nano_csv
$NODEMCU  firmware/cloud_upload_simulator_esp8266
$NODEMCU  iterations/firebase_relay_control_esp8266
"

filter="${1:-}"
passed=0
failed=()

while read -r fqbn sketch; do
  [ -z "$fqbn" ] && continue
  case "$sketch" in *"$filter"*) ;; *) continue ;; esac

  if [ -f "$sketch/secrets.example.h" ] && [ ! -f "$sketch/secrets.h" ]; then
    cp "$sketch/secrets.example.h" "$sketch/secrets.h"
  fi

  printf '%-28s %s ... ' "$fqbn" "$sketch"
  if log=$(arduino-cli compile --fqbn "$fqbn" --warnings default "$sketch" 2>&1); then
    echo "ok"
    passed=$((passed + 1))
  else
    echo "FAILED"
    echo "$log" | sed 's/^/    /'
    failed+=("$sketch")
  fi
done <<< "$SKETCHES"

echo
echo "$passed passed, ${#failed[@]} failed"
[ ${#failed[@]} -eq 0 ]
