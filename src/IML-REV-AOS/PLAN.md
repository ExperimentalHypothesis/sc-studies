# IML-REV-AOS — clean-room rewrite

## Context

`ACS_Source-IML_AOS/` holds three iterations (`IML_AOS`, `IML_AOS_v1`, `IML_AOS_v2`) of an inter-market mean-reversion strategy plus a `IML_AOS_v3/` directory that has only a README — no code. v1 and v2 work but are duplicated across four per-market `.cpp` files, contain a known correlation-sum bug (`RTY + NQ + NQ` — drops YM, double-counts NQ), and don't fit this repo's "one source → one DLL" build (`bash/build.sh`).

The goal is to **rebuild the strategy from scratch as a single clean `.cpp`** that compiles via the existing toolchain, captures the trading idea correctly, and is parameterized so any of ES/YM/NQ/RTY can be the traded ("self") market. First deployment trades **NQ only**; ES/YM/RTY come for free by adding the same trading study to those charts with different chart-study reference inputs.

## Strategy (the core idea, restated)

Mean-reversion on inter-market divergence among ES/YM/NQ/RTY:

- Each market's last close is classified into a quadrant of its **daily** range using three intra-range levels: `daily_low_mid = low + (high-low)/4`, `mid = low + (high-low)/2`, `daily_high_mid = low + 3*(high-low)/4`. Quadrant values: `-3` close < `low` (recent break-low), `-2` in `[low, daily_low_mid]`, `-1` in `[daily_low_mid, mid]`, `+1` in `[mid, daily_high_mid]`, `+2` in `[daily_high_mid, high]`, `+3` above `high` (recent break-high).
- **Long the self-market** when the other three sum ≤ −6 *and* each of them is < 0 *and* self quadrant > 0.
- **Short the self-market** when the other three sum ≥ +6 *and* each of them is > 0 *and* self quadrant < 0.
- On signal: arm an entry, valid for N minutes. Place a market-if-touched at `daily_low_mid` (long) / `daily_high_mid` (short) of the self market with an attached limit target at the opposite mid level and an attached stop `(daily_high_mid − daily_low_mid)` away. Sierra Chart's built-in move-to-BE fires at 50% of the way to target.
- Only operate inside a trading window; force-flatten + cancel everything at the configured flatten time. Skip days whose daily range is below a tick-count threshold. Disarm the signal if the correlation realigns before fill or if the validity window expires.

## Build & deployment

- New folder: `src/IML-REV-AOS/main.cpp` (single source).
- Build: `./bash/build.sh src/IML-REV-AOS/main.cpp` → `build/IML-REV-AOS.dll`. `SCDLLName("IML-REV-AOS")` inside the file must match.
- Deploy: `make copy` (existing hardcoded Whisky path is fine for now).
- Validate inside Sierra Chart using Replay mode on NQ — there are no automated tests for this codebase.

## File layout — one `.cpp`, two studies

```
src/IML-REV-AOS/main.cpp
├─ SCDLLName("IML-REV-AOS");
├─ namespace iml { …constants, helpers… }
├─ scsf_IML_Context        // added to each of ES/YM/NQ/RTY charts
└─ scsf_IML_REV            // generic trading study, added to whichever chart is "self"
```

Two `SCSFExport` functions in one DLL is standard ACSIL — Sierra Chart lists them as two separate studies in the picker.

### `scsf_IML_Context` — quadrant + daily levels exporter

Per-bar (`AutoLoop = 1`):

1. On RTH-start bar, snapshot `daily_high = daily_low = sc.Close[Index]`. On subsequent bars, expand high/low from the bar's H/L. (Reads RTH start from input.)
2. Recompute `daily_low_mid`, `mid`, `daily_high_mid` from the running daily H/L.
3. Compute quadrant for the last close:
   - If close > `daily_high` within the last `break_window_minutes` → `+3`
   - else if close ≥ `daily_high_mid` → `+2`
   - else if close ≥ `mid` → `+1`
   - else if close > `daily_low_mid` → `-1`
   - else if close > `daily_low` → `-2`
   - else → `-3` (recent break-low)
4. Publish to subgraphs so other studies can read via `sc.GetStudyArrayFromChartUsingID`:
   - `Subgraph[0]` = quadrant value (single number for the current bar)
   - `Subgraph[1..5]` = `daily_low`, `daily_low_mid`, `mid`, `daily_high_mid`, `daily_high`
5. Optional on-chart text via `sc.AddAndManageSingleTextDrawingForStudy` showing the current quadrant + daily range in ticks (gated by a `Display info` input).

Inputs: RTH start, RTH end, break-detection window (minutes), display toggle.

### `scsf_IML_REV` — generic trading study

Per-bar (`AutoLoop = 1`):

1. Read all four context studies via `sc.GetStudyArrayFromChartUsingID` using **chart-study reference inputs**:
   - Input 0: self-market context (e.g. NQ when this study is on the NQ chart)
   - Inputs 1–3: the three other markets' contexts
2. Pull quadrants `q_self`, `q1`, `q2`, `q3` from subgraph 0 of each referenced study, and the self market's `daily_low_mid`/`daily_high_mid` from subgraphs 2 and 4.
3. Compute `others_sum = q1 + q2 + q3` (the **fixed** version of the original `RTY+NQ+NQ` bug).
4. Detect a fresh signal on this bar:
   - **Long**: `others_sum ≤ −6 && q1 < 0 && q2 < 0 && q3 < 0 && q_self > 0`
   - **Short**: `others_sum ≥ +6 && q1 > 0 && q2 > 0 && q3 > 0 && q_self < 0`
   - When detected, store `signal_side` (persistent int) and `signal_time` (persistent `SCDateTime`).
5. Maintain the armed signal:
   - Clear if `sc.BaseDateTimeIn[Index] > signal_time + validity_minutes`.
   - Clear if correlation realigns (the original cleared on sum crossing zero — keep that).
6. Trade gate:
   - Inside `[trade_from, trade_until]` window?
   - Daily range `(daily_high − daily_low)/sc.TickSize >= min_range_ticks`?
   - No position open and no working order?
7. Place an entry order when armed and gates pass:
   - `s_SCNewOrder o;`
   - `o.OrderQuantity = qty` (input, default 1)
   - `o.OrderType = SCT_ORDERTYPE_MARKET_IF_TOUCHED`
   - `o.Price1 = daily_low_mid` (long) or `daily_high_mid` (short)
   - Attached limit target at the opposite mid via `AttachedOrderTarget1Type = SCT_ORDERTYPE_LIMIT`
   - Attached stop via `AttachedOrderStopAllType = SCT_ORDERTYPE_STOP` at distance `(daily_high_mid − daily_low_mid)` from entry
   - `o.MoveToBreakEven.Type = MOVETO_BE_ACTION_TYPE_OFFSET_TRIGGERED` with `TriggerOffsetInTicks = stop_distance_ticks / 2`, `BreakEvenLevelOffsetInTicks = 0`
   - `sc.BuyEntry(o)` / `sc.SellEntry(o)`
8. Flatten at `time_to_flat`: `sc.FlattenAndCancelAllOrders()`.
9. Logging: `sc.AddMessageToLog(...)` on signal-arm, on order submit, on flatten — Sierra Chart's trade log already records fills, so no external file.

Inputs (numbered for stability across reloads):

| # | Name | Default |
|---|------|---------|
| 0 | Self market context (chart-study ref) | — |
| 1 | Other market 1 context (chart-study ref) | — |
| 2 | Other market 2 context (chart-study ref) | — |
| 3 | Other market 3 context (chart-study ref) | — |
| 4 | Signal validity (minutes) | 10 |
| 5 | Minimal daily range (ticks) | 10 |
| 6 | Correlation threshold (abs) | 6 |
| 7 | Order quantity | 1 |
| 8 | Trade from | 08:30:00 |
| 9 | Trade until | 10:00:00 |
| 10 | Time to flat | 10:30:00 |
| 11 | Display info | Yes |

Persistent state (single block of `sc.GetPersistent*` slots):

- int `signal_side` (0 / +1 / −1)
- SCDateTime `signal_time`
- int `last_session_date_yyyymmdd` for reset detection

## What's intentionally NOT carried over from v1/v2

- External hardcoded "ES trades.txt" file logging — replaced by `AddMessageToLog`.
- Per-market `.cpp` duplication — one parameterized study, used four ways via inputs.
- Position-AveragePrice-based SL math (the v1/v2 file had a Czech "BUG POCITA TO BLBE SL" comment). The rewrite prices the stop off the level, not off the not-yet-known fill price.
- The `RTY + NQ + NQ` bug — fixed to `q1 + q2 + q3` over whichever three are passed as "other markets."

## Critical files

- **New**: `src/IML-REV-AOS/main.cpp` — the entire rewrite.
- **Reference only** (do not modify): `ACS_Source-IML_AOS/IML_AOS_v2/IML_REV_ES.cpp`, `ACS_Source-IML_AOS/IML_AOS_v2/imlcontext.cpp`, `ACS_Source-IML_AOS/IML_AOS_v2/imlconstants.h`. These are the closest-to-correct existing implementations and useful for cross-checking ACSIL API calls (`AttachedOrderTarget1Type`, `MoveToBreakEven`, `GetPersistentFloatFromChartStudy`, etc.).
- **No changes needed** to `CMakeLists.txt`, `bash/build.sh`, `mingw-w64-toolchain.cmake` — the new file fits the existing build flow.

## Verification

1. `./bash/build.sh src/IML-REV-AOS/main.cpp` — confirm clean compile, no warnings beyond the suppressed set.
2. `make copy` — DLL lands in Sierra Chart's Data folder.
3. In Sierra Chart: open four 1-min charts (ES, YM, NQ, RTY), add `IML Context` to each, configure RTH window. Confirm quadrant subgraph plots values in `[-3, +3]` and the daily levels track H/L correctly.
4. On the NQ chart, add `IML REV`, wire Input 0 to the NQ context, Inputs 1–3 to ES/YM/RTY contexts. Use Replay mode on a historical day with a known divergence and verify:
   - Signal-arm message appears in the journal when conditions trigger.
   - MIT order shows up at `daily_low_mid` (long) or `daily_high_mid` (short) on the NQ chart.
   - Target/stop offsets equal `(daily_high_mid − daily_low_mid)` ticks each side.
   - Stop moves to break-even at 50% of the way to target.
   - Position flattens at 10:30:00 even if target/stop wasn't hit.
   - Days with `(daily_high − daily_low)/tick < min_range_ticks` produce no trades.
5. Move the trading study to ES (or YM, RTY) and re-wire Input 0 to that chart's context, Inputs 1–3 to the other three. Confirm the same behavior, with no code changes.
