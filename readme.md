# Sierra Chart Study Toolkit

This repository contains a cross-platform toolchain for building Sierra Chart ACSIL studies from macOS using MinGW-w64, along with several example studies including automated order submission logic.

## Repository Layout
- `src/studies/01` – simple plotting example (`scsf_SimpleExample`).
- `src/studies/02` – volume-based studies:
  - `scsf_HighVolumeMarker` highlights high total-volume bars.
  - `scsf_HighAskBidVolumeMarker` marks bars with large ask/bid volume.
  - `scsf_HighAskBidVolumeTrader` plots the same markers and, when enabled, submits market orders if the thresholds are met and the bar is closed.
- `include/` – ACSIL headers mirrored from Sierra Chart for cross-compilation.
- `bash/build.sh` – convenience wrapper for configuring and compiling DLLs.
- `bash/copy.sh` – copies every `build/*.dll` into the local Sierra Chart `Data` folder. Exposed as `make copy`.
- `build/` – generated CMake artifacts and the resulting Windows DLL.

## Workflow

Every change to a study follows the same three-step loop:

```sh
# 1. Cross-compile the .cpp to a Windows DLL via MinGW-w64.
./bash/build.sh src/<StudyName>/main.cpp <DllName>
#    → build/<DllName>.dll

# 2. Deploy: copy build/*.dll into the Sierra Chart Data directory.
make copy
#    (wraps bash/copy.sh — the destination path is hardcoded to this
#     machine's Whisky-hosted Sierra Chart install; edit copy.sh if yours
#     lives elsewhere.)

# 3. In Sierra Chart, reload custom studies (Analysis → Build Custom
#    Studies DLL → Release Use of DLLs, then re-add the study) or restart
#    Sierra Chart so the updated DLL is picked up.
```

### Two deployment targets

This repo is built once on the Mac and runs in **two** Sierra Chart instances simultaneously:

1. **Local Mac (Whisky / Wine).** `make copy` drops the freshly built DLL into Sierra Chart's `Data` folder inside the Whisky bottle. After step 3 above, the local Sierra Chart instance running under Whisky can load and trade the study directly on the Mac. The exact destination is hardcoded in `bash/copy.sh`:

   ```
   /Users/lukas.kotatko/Library/Containers/com.isaacmarovitz.Whisky/Bottles/<UUID>/drive_c/SierraChart - AMP (...)/Data
   ```

2. **Remote Windows machine (mirrored folder).** The whole repository directory is mirrored to a Windows box that's accessed via the Microsoft "Windows App" remote-desktop client. Because the mirror keeps `build/` in sync, the freshly produced `build/<DllName>.dll` appears on the Windows side automatically — **no `make copy` needed for that machine**. The remote Sierra Chart instance still needs to release/reload custom studies (step 3) to pick up the new binary.

So a single `./bash/build.sh …` produces a DLL that:
- becomes immediately visible inside Windows over the mirror (for the remote SC instance), and
- after `make copy`, is also installed into the local Whisky SC instance.

`make copy` is **not optional for the local instance after a rebuild** — Sierra Chart loads the DLL from its `Data` folder, not from `build/`. Skipping the copy step means the local instance keeps running the previous version even though the build succeeded. The remote instance, by contrast, doesn't need `make copy` because it reads the mirrored `build/` directly (assuming you've pointed its custom-studies DLL path at the mirrored folder, or you copy on the Windows side once).

Concrete example for the IML reversal trader:

```sh
./bash/build.sh src/IML-REV-AOS/main.cpp
make copy
```

## Prerequisites
1. Install the MinGW-w64 toolchain via Homebrew:
   ```sh
   brew install mingw-w64
   ```
2. Ensure the Sierra Chart headers in `include/` match the version you deploy to.

## Building A Study
1. From the repository root, run the helper script with your source file:
   ```sh
   bash/build.sh src/IML-REV-AOS/main.cpp
   ```
   - First argument: path to the `.cpp` study you want to build (default `src/first_test/main.cpp`).
   - Optional second argument: override the DLL name. If omitted, the DLL name defaults to the **source's parent folder** (e.g. `IML-REV-AOS`) — so for the standard `src/<StudyName>/main.cpp` layout you never need to type the name twice. The study's `SCDLLName(...)` must match the resulting DLL filename.
2. The script clears `build/`, configures CMake with `mingw-w64-toolchain.cmake`, and invokes `cmake --build`. The compiled DLL is written to `build/<Name>.dll`.

### Static Linking Note
The CMake configuration links the GNU runtime libraries statically to avoid Windows “Error 126” caused by missing `libwinpthread-1.dll` or similar dependencies on the target machine.

## Loading In Sierra Chart
1. Copy the generated DLL into your Sierra Chart `Data` directory — run `make copy` (or `./bash/copy.sh`). The script copies every `build/*.dll`.
2. Restart Sierra Chart or reload custom studies.
3. Locate the exported study by its function name (e.g., `scsf_HighAskBidVolumeTrader`) and add it to a chart.
4. For trading:
   - Enable the `Enable Trading` input.
   - Set ask/bid volume thresholds and order quantity.
   - Confirm Sierra Chart is connected to a trading service or simulation environment; the study submits market orders when the bar closes and the volume condition is met.

## Optional Auto-Copy
Provide your Sierra Chart Data directory when running CMake to copy the DLL automatically after each build:
```sh
cmake -DSOURCE_FILE=src/studies/02/main.cpp \
      -DDLL_NAME=AboveThreshold \
      -DSIERRACHART_DIR=\"/Volumes/C/SierraChart/Data\" \
      -B build -S .
cmake --build build
```
Set `SIERRACHART_DIR` once in the cache if you always deploy to the same location.

## Troubleshooting
- **DLL fails to load / Error 126**: confirm the DLL name matches the study’s `SCDLLName` and that you rebuilt after updating the name. All runtime dependencies are linked statically, so no extra DLLs should be required.
- **Study does not trade**: verify `Enable Trading` is on, Sierra Chart is not in a full recalculation state, and the bar has closed (conditions are evaluated on the most recent completed bar).
- **Headers out of sync**: replace the files in `include/` with the versions from your Sierra Chart installation to keep structures and enumerations current.

Happy trading! Use Sierra Chart’s Replay mode to validate any strategy before running it live.
