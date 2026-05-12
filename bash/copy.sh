#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$SCRIPT_DIR/build"
DEST="/Users/lukas.kotatko/Library/Containers/com.isaacmarovitz.Whisky/Bottles/91715EEF-0900-49E1-A2F6-36E91D4CB5D9/drive_c/SierraChart - AMP (Denalli + Teton)/Data"

cp "$SRC"/*.dll "$DEST"
echo "Copied DLLs to Sierra Chart Data folder"
