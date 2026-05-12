# Repository Guidelines

## Project Structure & Module Organization
Source code for individual Sierra Chart studies lives under `src/<StudyName>/main.cpp`; create a new folder per study to keep exports isolated. Shared ACSIL headers and examples are under `include/`, so prefer reusing those helpers instead of copying them into each study. Build artifacts land in `build/` and should stay out of version control—treat it as disposable output from CMake.

## Build, Test, and Development Commands
Run `./build.sh` to configure and compile the default `main.cpp` study into `main.dll`. Supply a different source or DLL name as needed, for example `./build.sh RHT_Range/main.cpp RTHRange` to emit `RTHRange.dll`. When iterating in CLion or another IDE, point your run configuration at the script with the same arguments so the CMake cache stays in sync.

## Coding Style & Naming Conventions
Follow the existing C++ layout: four-space indentation, braces on the same line, and early returns inside the `if (sc.SetDefaults)` block. Export functions should use the `scsf_<DescriptiveName>` pattern and meaningful `SCSubgraphRef` names (e.g., `FastMA`, `SellSignal`). Favor descriptive Study and Input labels so Sierra Chart dialogs remain readable; keep RGB color literals and constants grouped with their corresponding subgraphs.

## Testing Guidelines
There is no automated test harness, so validate behavior in Sierra Chart after every build. Use the platform’s Data/Replay features to confirm study outputs, and inspect any CSV files produced (such as the RTH range export) before committing. When adding logic, consider logging intermediate values behind a compile-time flag to aid manual verification.

## Commit & Pull Request Guidelines
Match the existing concise history—short, present-tense messages such as `prepare cmake` or `add rth range inputs`. Each pull request should summarize the study changes, list Sierra Chart versions tested, and note any deployment steps or data files touched. Attach screenshots of chart output when visuals change, and reference linked issues so reviewers understand the trading scenario being addressed.
