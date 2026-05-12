---
description: Diagnose Sierra Chart "Error loading … missing SCDLLName line" failures by checking every known cause in order, then reporting the matching fix.
---

The user has hit a Sierra Chart study load error. The error text from Sierra Chart usually looks like:

```
Error loading: C:\...\Data\<something>.dll. File not found or may be missing SCDLLName line.
```

That single message is a catch-all — Sierra Chart prints it for several distinct underlying causes. Your job is to walk through the diagnostic checklist below in order and identify the actual cause, then report it with the specific fix.

**Before you start**: if the user pasted the full error path from Sierra Chart, extract the DLL filename from it. If they didn't, ask once for the exact error text (with the path), then proceed. Do not ask follow-up questions — work the checklist.

## Checklist (run each in order, stop when you find the cause)

### 1. Is the DLL actually in the Sierra Chart Data folder?

The local Whisky Data folder is hardcoded in `bash/copy.sh`. Read that file to get the exact path, then `ls -la "<DATA>/<DllName>.dll"`. If missing: the user forgot to run `make copy` after building. If present, note its size and mtime and continue.

### 2. Are there duplicate DLLs exporting the same `scsf_*` names?

`ls "<DATA>" | grep -i "<stem>"` — look for sibling DLLs with similar names (left over from renames). Then for each candidate, run `x86_64-w64-mingw32-nm --defined-only --extern-only "<DATA>/<file>.dll" | grep scsf_` and see if two files export the same function names. If they do, the alphabetically-earlier filename wins and the other errors. Fix: delete the stale DLL from `Data/` (and from `build/` so `make copy` doesn't re-deploy it). Sierra Chart sorts hyphen `-` (0x2D) before underscore `_` (0x5F).

### 3. Does the DLL contain IPA-SRA clone symbols?

This is the `.isra.0` trap documented in `CLAUDE.md` § Build gotchas. Run:

```sh
x86_64-w64-mingw32-strings "<DATA>/<DllName>.dll" | grep -E '\.isra\.|_ZZ.*scsf'
```

If anything comes back, the build was made without the `-s` linker flag and Sierra Chart is interpreting the mangled clone names as bogus studies. Fix: check that `target_link_options(StudyDLL PRIVATE ... -s)` is still in `CMakeLists.txt` (it is the load-bearing strip flag), then rebuild + `make copy`.

### 4. Does `SCDLLName("…")` match the DLL filename?

```sh
grep -n "SCDLLName" src/<StudyFolder>/main.cpp
```

Compare to the DLL filename on disk. They must match exactly (case-sensitive, hyphen vs. underscore matters). If they differ, decide with the user which to change, then rename the source string or rebuild with the matching name.

### 5. Are there runtime dependencies the target can't resolve?

```sh
x86_64-w64-mingw32-objdump -p "<DATA>/<DllName>.dll" | grep "DLL Name"
```

The Homebrew mingw-w64 toolchain links the Universal CRT — imports will list `api-ms-win-crt-*.dll`. That's expected and works on most environments. **But**: if you see imports that aren't `KERNEL32.dll` / `api-ms-win-crt-*.dll` / `ws2_32.dll` / `winmm.dll`, that's a new dependency the target probably doesn't have. Identify it and either statically link it or document the install step.

### 6. Are there stale chartbook references?

If steps 1–5 are clean but the error persists for a chart the user already had open, the chartbook may have a saved reference to an old DLL filename. Ask the user to:
1. **Analysis → Build Custom Studies DLL → Release Use of DLLs** in Sierra Chart, OR fully restart Sierra Chart.
2. Re-add the study to the chart from the fresh picker.

## Report format

Once you've found the cause, respond with:

- **Root cause**: one sentence.
- **Why it produces this specific error message**: one sentence.
- **Fix**: the exact command(s) or edit(s) to apply.

Do not narrate the steps you didn't need. Skip causes that don't match.
