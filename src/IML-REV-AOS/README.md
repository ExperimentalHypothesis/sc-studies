# IML-REV-AOS — Inter-Market Liquidity Reversal Automated Order System

Mean-reversion futures trader for Sierra Chart. Watches four correlated US index futures (ES, YM, NQ, RTY); when three of them diverge against the fourth, it bets the fourth will revert to the daily midpoint region.

Single-file build: `src/IML-REV-AOS/main.cpp` → `IML-REV-AOS.dll` (one DLL, two studies inside).

## How the system works

### 1. Quadrant model

For each of ES, YM, NQ, RTY, the `IML Context` study tracks the **regular-trading-hours daily high and low** (resets at the configured RTH start time) and derives three intra-range levels:

```
daily_high
daily_high_mid  =  midpoint(mid, daily_high)        ← 75% of range
mid             =  midpoint(daily_low, daily_high)  ← 50%
daily_low_mid   =  midpoint(daily_low, mid)         ← 25%
daily_low
```

Every bar, the current close is classified into a quadrant from -3 to +3:

| Value | Meaning |
|-------|---------|
| `+3`  | `BREAK_HIGH` — close above daily high, and that high was made within the last N minutes |
| `+2`  | `HIGH` — close between `daily_high_mid` and `daily_high` |
| `+1`  | `HIGH_MID` — close between `mid` and `daily_high_mid` |
| `-1`  | `LOW_MID` — close between `daily_low_mid` and `mid` |
| `-2`  | `LOW` — close between `daily_low` and `daily_low_mid` |
| `-3`  | `BREAK_LOW` — close below daily low, made within the last N minutes |

The quadrant value is published to a persistent slot so other studies on other charts can read it.

### 2. Divergence signal

The `IML REV` trading study is given one "self" context (the chart it sits on) and three "other" contexts. It reads four quadrant values per bar: `q_self`, `q1`, `q2`, `q3`.

**The intuition.** Three correlated markets are dumping; one (say NQ) is holding up. Naive sellers see the divergence and short NQ expecting it to "catch down" with the others. Someone on the other side is happy to fill them. NQ then sells off just enough to take out those shorts at a worse price — typically down to the lower quadrant of its own daily range — and then mean-reverts back up. **We want to be the buyer of those forced/late sells.** Same logic mirrored for shorts.

So a signal is armed when three markets are displaced one way and the self market is displaced the other:

- **LONG self**: `q1 + q2 + q3 ≤ -threshold` *and* each of `q1, q2, q3 < 0` *and* `q_self > 0`.  → "Others are weak, self is propped up; expect self to dip to its lower quadrant, then revert up. Buy that dip."
- **SHORT self**: `q1 + q2 + q3 ≥ +threshold` *and* each of `q1, q2, q3 > 0` *and* `q_self < 0`.  → "Others are strong, self is held down; expect self to spike to its upper quadrant, then revert down. Sell that spike."

Default `threshold = 6` — each of the three others must average at least the `±2` quadrant.

An armed signal stays armed for a configurable number of minutes; it disarms early if the correlation sum crosses zero against the armed side (the divergence has resolved without our trade).

**LONG NQ example** — what the four charts look like at the moment of arming:

```
           ES           YM           RTY          NQ (self)
        ┌──────┐     ┌──────┐     ┌──────┐     ┌──────┐
   high │      │     │      │     │      │     │  ●   │  ← NQ here  q=+2
        │      │     │      │     │      │     │      │
    DHM ├──────┤     ├──────┤     ├──────┤     ├══════┤  ← TARGET (long)
        │      │     │      │     │      │     │      │
    mid ├──────┤     ├──────┤     ├──────┤     ├──────┤
        │      │     │      │     │      │     │      │
    DLM ├──────┤     ├──────┤     ├──────┤     ├══════┤  ← MIT BUY here
        │      │     │      │     │      │     │      │
    low │  ●   │     │  ●   │     │  ●   │     │      │  ← others here  q=-2 each
        └──────┘     └──────┘     └──────┘     └──────┘
        q = -2       q = -2       q = -2       q = +2

   others_sum = -6   ≤ -threshold(6)   ✓
   each other  < 0                     ✓
   self        > 0                     ✓
   → ARM LONG NQ
```

Trade execution after arming:

1. Place MIT BUY at NQ's `daily_low_mid` (the "═" line above).
2. Wait — fills only if NQ actually drops to that level (those are the late sellers we want).
3. On fill: limit target at NQ's `daily_high_mid`; stop the same distance below entry. Stop moves to BE at 50% of the way to target.
4. If NQ doesn't trade down to `daily_low_mid` within `Signal validity` minutes, the signal expires unfilled. No trade.

**SHORT NQ** is the mirror: ES/YM/RTY at quadrant `+2`, NQ at `-2`. MIT SELL at NQ's `daily_high_mid`, target `daily_low_mid`, stop the same distance above entry.

The "self" market is whichever chart you put `IML REV` on — wire the other three as Inputs 1/2/3 and the system trades the divergent one automatically. NQ-led divergence on the NQ chart, ES-led divergence on the ES chart, etc.

### 3. Entry / target / stop

On the next bar after arming, while inside the trading window:

- **Entry**: market-if-touched order at the self market's `daily_low_mid` (long) or `daily_high_mid` (short).
- **Target**: limit at the opposite mid level (`daily_high_mid` for longs, `daily_low_mid` for shorts).
- **Stop**: the same distance on the other side of entry, so target distance = stop distance = `daily_high_mid − daily_low_mid`. Risk:reward 1:1 at the level.
- **Break-even**: Sierra Chart's built-in `MoveToBreakEven` moves the stop to entry once price has travelled 50% of the way to target.
- **Flatten**: at the configured flatten time, all positions and working orders are cancelled regardless of P/L.

### 4. Filters

- Daily range must be ≥ `Minimum daily range (ticks)` — skips thin/holiday days.
- Trading only happens between `Trade from` and `Trade until`.
- No multiple entries: while a position or working order exists, new signals don't fire.

## Setup in Sierra Chart

### Step 1 — build & deploy

```sh
./bash/build.sh src/IML-REV-AOS/main.cpp
make copy
```

This compiles `IML-REV-AOS.dll` (the build script defaults the DLL name to the source folder, so no second argument is needed) and copies it into the Sierra Chart `Data` directory. Restart Sierra Chart (or reload custom studies) to pick up the new DLL.

### Step 2 — add Context studies

Open four 1-minute charts: ES, YM, NQ, RTY. On **each** chart, add the study **"IML Context — quadrant & daily levels"**. Same defaults work on all four.

### Step 3 — add the trading study

On whichever chart you want to trade (start with NQ), add **"IML REV — divergence trader"**. Configure:

- **Self market context** → this chart's `IML Context`
- **Other market 1/2/3 context** → the other three charts' `IML Context` studies (order among them doesn't matter)

## Inputs reference

### IML Context (added to ES, YM, NQ, RTY)

| # | Name | Default | What it does |
|---|------|---------|--------------|
| 0 | RTH start | 08:30:00 | Time-of-day at which the daily H/L and quadrant levels reset. Set this to the regular session open for the instrument. |
| 1 | Mins back for break detection | 10 | A new daily high/low within this many minutes promotes the close to `BREAK_HIGH` / `BREAK_LOW` (±3) instead of `HIGH` / `LOW` (±2). |
| 2 | Display info on chart | Yes | Toggles a small text overlay showing the current quadrant and the daily range in ticks. Turn off for cleaner charts. |

### IML REV (added to one chart — the one you want to trade)

| # | Name | Default | What it does |
|---|------|---------|--------------|
| 0 | Self market context | this chart | Chart-study reference to the `IML Context` study on the chart you're trading. |
| 1 | Other market 1 context | chart 1 | Chart-study reference to another `IML Context`. |
| 2 | Other market 2 context | chart 2 | Same — pick the second non-self market. |
| 3 | Other market 3 context | chart 3 | Same — pick the third non-self market. |
| 4 | Signal validity (minutes) | 10 | Once armed, the signal is dropped if no entry triggers within this window. Lower = tighter, higher = more permissive. |
| 5 | Minimum daily range (ticks) | 10 | Below this, no trades for the day. Filters chop and pre-news sessions. |
| 6 | Correlation threshold (abs) | 6 | The absolute value the sum of the three "other" quadrants must reach to arm a signal. Range 3–9: lower = more signals, higher = only extreme divergences. |
| 7 | Order quantity | 1 | Contracts per order. |
| 8 | Trade from | 08:30:00 | Earliest time of day to send entries. |
| 9 | Trade until | 10:00:00 | Latest time of day to send entries. After this, no new entries; existing positions are still managed. |
| 10 | Flatten at | 10:30:00 | At this time the study cancels working orders and flattens any open position. |
| 11 | Display info on chart | Yes | Toggles the on-chart status line showing the four quadrant values, their sum, and the armed side. |

## Trading multiple markets

The `IML REV` study is symmetric — it doesn't care which of ES/YM/NQ/RTY is "self." To run the system on more than one market simultaneously, just drop the same study on each chart you want to trade and wire Inputs 0–3 appropriately. No rebuild needed.

## Validation checklist

Before connecting to live trading, in Sierra Chart Replay mode confirm:

1. The four context studies plot daily H/L lines that reset at RTH start and expand correctly as new highs/lows are made.
2. The on-chart status line on the trading chart shows non-zero quadrant values during typical hours.
3. On a known historical divergence day (eg. a morning where NQ leads while ES/YM/RTY lag), an "IML armed" message appears in the Sierra Chart journal.
4. The MIT order appears at the expected level (`daily_low_mid` for longs, `daily_high_mid` for shorts).
5. Target and stop offsets equal `(daily_high_mid − daily_low_mid)` each side of entry.
6. The stop moves to break-even after price reaches the halfway point.
7. Any open position is flattened at the configured flatten time even if the target / stop never hit.

## Files

- `main.cpp` — the entire implementation (Context + REV studies).
- `PLAN.md` — design notes for the rewrite from the v1/v2 legacy code.
- `README.md` — this file.
