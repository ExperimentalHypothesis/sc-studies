# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

A cross-compilation toolchain for Sierra Chart ACSIL studies. Source is C++ that targets Windows DLLs, but is built **on macOS** via MinGW-w64. The compiled `.dll` is loaded by Sierra Chart (which itself runs on Windows, or under Whisky/Wine on this machine).

There is no automated test harness — behavior must be validated inside Sierra Chart, typically using Replay mode against historical data.

## Build

The canonical entry point is `bash/build.sh`. It wipes `build/`, runs CMake with the MinGW toolchain, and compiles **one** study source into **one** DLL.

```sh
./bash/build.sh src/02/main.cpp AboveThreshold   # → build/AboveThreshold.dll
./bash/build.sh src/PriceQuickness/main.cpp      # DLL name defaults to source basename
```

`make build NAME=Foo SRC=src/Foo/main.cpp` is equivalent. `make clean` removes `build/`.

Important constraint: **the DLL name passed to `build.sh` must match the `SCDLLName("…")` string inside the `.cpp` file.** Mismatches cause Windows Error 126 when Sierra Chart loads the DLL.

### Deploy to Sierra Chart

`bash/copy.sh` (also `make copy`) copies every `build/*.dll` into the user's Whisky-hosted Sierra Chart `Data` folder. The destination path is hardcoded to this machine's Whisky bottle — edit `bash/copy.sh` if the bottle UUID or chart install changes.

Alternative: pass `-DSIERRACHART_DIR=<path>` when configuring CMake to enable auto-copy as a post-build step (`CMakeLists.txt:65-73`).

### Toolchain expectations

- `brew install mingw-w64` must be present; `mingw-w64-toolchain.cmake` hardcodes `/opt/homebrew/opt/mingw-w64` as the find root.
- CMake links `libgcc`, `libstdc++`, and `winpthread` **statically** (`-static -static-libgcc -static-libstdc++`). Do not switch to dynamic linking — missing runtime DLLs on the Windows side surface as Error 126.

### Build gotchas — do not regress

- **Keep `-s` in `target_link_options` in `CMakeLists.txt`.** It is load-bearing, not cosmetic. At `-O2`, GCC's IPA-SRA / IPA-cp optimization clones (e.g. lambdas inside an `SCSFExport` function) get globally-named mangled COFF symbols like `_ZZ12scsf_Foo...isra.0`. Those are **not** in the PE export table, but Sierra Chart's loader scans the COFF symbol table for any `scsf_*` prefix and tries to register them as studies — producing garbage "Error loading: …\<mangled-name>.isra.dll" errors. `-s` strips the COFF symbol table so only the four real PE exports remain visible.
- **DLL name defaults to the source's parent folder.** `./bash/build.sh src/<StudyName>/main.cpp` (no second arg) produces `build/<StudyName>.dll`. The `SCDLLName("…")` literal inside the `.cpp` must equal `<StudyName>` exactly — Sierra Chart reports the mismatch as "missing SCDLLName line."
- **One DLL per study in `Data/`.** Two DLLs that export the same `scsf_*` function names — even with different filenames — collide in Sierra Chart's loader. If a rename leaves an old DLL behind in the Whisky/Wine `Data` folder, delete it; otherwise the alphabetically-earlier one wins and the newer one shows as "Error loading."

## Study architecture

Each study is a self-contained `src/<StudyName>/main.cpp` that includes `../include/sierrachart.h` and exports one or more `scsf_<Name>` functions. The build only ever sees a single source file at a time, so studies cannot share code across folders via the build — copy/paste or extend headers under `include/` if reuse is needed.

Every ACSIL function follows the same shape:

```cpp
SCSFExport scsf_Name(SCStudyInterfaceRef sc) {
    SCSubgraphRef Plot = sc.Subgraph[0];
    SCInputRef   Threshold = sc.Input[0];

    if (sc.SetDefaults) {
        // configure GraphName, AutoLoop, subgraph styles, input defaults
        return;   // early return is required
    }

    // per-bar evaluation runs after SetDefaults
}
```

`sc.AutoLoop = 1` runs the function once per bar with `sc.Index` set; `AutoLoop = 0` (used by `PriceQuickness`) means the function manages its own loop over `sc.ArraySize` and persistent state lives in `sc.GetPersistent*` slots.

`include/` is a mirrored copy of the Sierra Chart ACSIL SDK. Replace its contents with the headers from the target Sierra Chart install if structures drift after a Sierra Chart upgrade.

## Conventions (from AGENTS.md)

- 4-space indentation, braces on the same line.
- Exported entry points use `scsf_<DescriptiveName>`; subgraphs and inputs get meaningful names (`FastMA`, `SellSignal`, `VolumeThreshold`) because those strings appear in the Sierra Chart UI.
- Keep RGB color literals next to the subgraph they style.
- Commits are short and present-tense (e.g. `add rth range inputs`, `prepare cmake`).

## Per-study README

Every study folder under `src/<StudyName>/` must ship a `README.md` alongside `main.cpp`. The README is the operating manual for using the DLL inside Sierra Chart — without it the inputs are opaque numbers and the trading logic is buried in C++.

Required sections, in this order:

1. **What it does** — one paragraph: the trading idea or analytical purpose, in plain English. No code.
2. **How it works** — the model / signal logic in enough detail that a reader can sanity-check a trade by hand. Include the math for any derived levels, the exact signal conditions, and the entry/exit rules.
3. **Setup in Sierra Chart** — build & deploy commands (`./bash/build.sh …`, `make copy`), which charts to add which studies to, and chart-study references that need wiring.
4. **Inputs reference** — one table per `SCSFExport` study in the DLL, columns `# | Name | Default | What it does`. Every input gets a row. The "What it does" cell explains what the parameter controls and, when relevant, the tradeoff in raising or lowering it.
5. **Validation checklist** — concrete things to verify in Replay mode before going live (or, for non-trading studies, before trusting the output). Numbered items, each independently checkable.
6. **Files** — short list of what's in the folder.

Keep the language operator-focused, not implementation-focused — the reader is the person clicking through Sierra Chart's "Add Study" dialog, not the person editing the `.cpp`. Implementation notes belong in `PLAN.md` or code comments.

See `src/IML-REV-AOS/README.md` for the canonical example.
