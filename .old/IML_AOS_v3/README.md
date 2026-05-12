# IML AOS v3 - Inter-Market Liquidity Automated Order System

**Version 3** - Refactored for readability, maintainability, and DRY principles

## Overview

IML AOS is a mean-reversion trading system for Sierra Chart that trades based on inter-market divergence patterns across four major US futures indices: ES, YM, NQ, and RTY.

The strategy identifies when one market (ES) diverges from the others, then enters a position expecting the price to revert to the mean (daily midpoint).

## Trading Strategy

### Core Concept: Inter-Market Liquidity (IML)

The system monitors four major indices simultaneously:
- **ES** (S&P 500 E-mini) - Primary trading instrument
- **YM** (Dow Jones E-mini)
- **NQ** (Nasdaq-100 E-mini)
- **RTY** (Russell 2000 E-mini)

Each market's position is classified into quadrants based on its daily range:
- **+3**: Breaking above daily high (strong bullish)
- **+2**: Upper quadrant (daily_high_mid to daily_high)
- **+1**: Upper-mid quadrant (mid to daily_high_mid)
- **-1**: Lower-mid quadrant (daily_low_mid to mid)
- **-2**: Lower quadrant (daily_low to daily_low_mid)
- **-3**: Breaking below daily low (strong bearish)

### Signal Generation

**LONG Signal** (Buy ES):
- YM, NQ, RTY all bearish (negative quadrant values)
- ES is bullish (positive quadrant value)
- Sum of YM + NQ + RTY ≤ -6
- **Interpretation**: ES diverging upward while others weak → expect ES reversion down to mean

**SHORT Signal** (Sell ES):
- YM, NQ, RTY all bullish (positive quadrant values)
- ES is bearish (negative quadrant value)
- Sum of YM + NQ + RTY ≥ 6
- **Interpretation**: ES diverging downward while others strong → expect ES reversion up to mean

### Entry & Exit

- **Entry**: Market-if-touched order at daily_low_mid (long) or daily_high_mid (short)
- **Target**: Opposite mid-level (mean reversion to daily midpoint region)
- **Stop Loss**: Distance = (daily_high_mid - daily_low_mid)
- **Break-Even**: Moves to break-even at 50% of profit target
- **Time Window**: Only trades during specified hours (default: 8:30 AM - 10:00 AM)
- **Signal Validity**: 10-minute window after IML divergence detected
- **Position Flattening**: Automatic at specified time (default: 10:30 AM)

### Additional Filters

- **Minimum Range**: Requires minimum daily range (in ticks) to trade
- **Correlation Monitoring**: Exits signal if markets realign before entry

## Project Structure

```
IML_AOS_v3/
├── core/
│   ├── iml_constants.h          # System constants, enums, market configs
│   ├── iml_types.h              # Data structures (trade_data, daily_levels, market_state)
│   ├── iml_context.cpp          # Context study: quadrant analysis & DHL tracking
│   └── iml_trading.cpp          # Unified trading logic for all markets
├── utilities/
│   ├── iml_logging.h            # Logging to chart and external files
│   ├── iml_time.h               # Time window validation and utilities
│   ├── iml_drawing.h            # Chart visualization and text overlays
│   └── iml_orders.h             # Order setup and execution
└── README.md                    # This file
```

## File Descriptions

### Core Files

**iml_constants.h**
- Trading thresholds (min/max sum values)
- Quadrant enumeration
- Market configurations (symbol, log filename)

**iml_types.h**
- `trade_data`: Trade information (entry price, range, ATR, volume, datetime)
- `daily_levels`: Daily high/low and calculated midpoint levels
- `market_state`: Quadrant values for all four markets with signal detection methods

**iml_context.cpp**
- Sierra Chart study: `scsf_iml_context_quadrant_analysis`
- Tracks daily high/low levels
- Divides range into quadrants
- Returns quadrant value for other studies to consume
- Detects break-outs of daily high/low

**iml_trading.cpp**
- Core strategy execution function: `execute_strategy()`
- Four Sierra Chart export functions (thin wrappers):
  - `scsf_iml_rev_es` - ES trading
  - `scsf_iml_rev_ym` - YM trading
  - `scsf_iml_rev_nq` - NQ trading
  - `scsf_iml_rev_rty` - RTY trading

### Utility Files

**iml_logging.h**
- Internal log (Sierra Chart message log)
- External log (text files)
- Trade entry logging with formatted output

**iml_time.h**
- Trading window validation
- IML signal validity checking
- Datetime calculations

**iml_drawing.h**
- Market status display
- Trade ready indicators
- IML validity window visualization
- Break notifications
- Quadrant value display

**iml_orders.h**
- Order setup (market-if-touched with attached targets/stops)
- Long/short entry execution
- Position status checking
- Flatten and cancel operations

## Installation & Setup

### 1. Build in Sierra Chart

1. Copy the entire `IML_AOS_v3/` folder to your Sierra Chart `ACS_Source` directory
2. In Sierra Chart, go to **Analysis** → **Build Custom Studies DLL**
3. Select `Remote Build` or `Local Build` depending on your setup
4. The system will compile all `.cpp` files

### 2. Add Studies to Charts

**Step 1: Add Context Studies**
- Open charts for ES, YM, NQ, and RTY
- On each chart, add study: "IML Context: Quadrant Analysis (DHL Break Detection)"
- Configure RTH hours (default: 8:30 AM - 3:15 PM)

**Step 2: Add Trading Studies**
- On your ES chart, add study: "IML REV ES - Unified Trading Logic"
- Configure the following inputs:
  - Input 0: Chart/Study reference for ES context study
  - Input 1: Chart/Study reference for YM context study
  - Input 2: Chart/Study reference for RTY context study
  - Input 3: Chart/Study reference for NQ context study
  - Input 5: Chart/Study reference for ES daily levels (same chart)
  - Inputs 17-19: Trading hours (start, stop, flatten time)

Repeat for YM, NQ, RTY if trading those markets independently.

### 3. Configuration Parameters

**Context Study**:
- `RTH start`: Regular trading hours start time (default: 8:30 AM)
- `RTH end`: Regular trading hours end time (default: 3:15 PM)
- `How many mins back to count the break valid?`: Break detection window (default: 10 min)
- `Display Logs`: Show/hide chart visualizations

**Trading Study**:
- `Trade from`: Start of trading window (default: 8:30 AM)
- `Trade until`: End of trading window (default: 10:00 AM)
- `Time to flat`: Automatic position flatten time (default: 10:30 AM)
- `How many mins the IML is valid?`: Signal validity duration (default: 10 min)
- `Minimal range in ticks`: Minimum daily range required to trade (default: 10 ticks)
- `Display Logs`: Show/hide chart visualizations

## Usage

### Normal Operation

1. Ensure all four context studies are running (ES, YM, NQ, RTY)
2. Enable trading study on desired market (typically ES)
3. The system will:
   - Monitor inter-market correlation
   - Display market status on chart
   - Show trade ready indicators
   - Execute entries automatically when conditions met
   - Manage targets, stops, and break-even
   - Flatten positions at specified time

### Chart Indicators

When visualization is enabled, you'll see:
- Market quadrant values and correlation sum
- "READY TO TRADE IML REV LONG/SHORT" status
- IML validity window with timestamps
- Stop loss offset value
- Break notifications (new daily high/low)

### Log Files

Trade entries are logged to:
- **Internal**: Sierra Chart message log
- **External**: Text files (e.g., "ES trades.txt")

Log format:
```
Cena: 4500.25, Date and Time: 2024-3-15  09:45:30 | Daily range (in ticks): 45.0 | ATR: 1.2345
```

## Key Improvements Over v1/v2

### Code Quality
- **-1000 lines**: Eliminated duplication across 4 market files
- **Organized structure**: Clear separation of concerns
- **Reusable utilities**: Common functions extracted
- **Better naming**: Consistent, descriptive variable/function names
- **Comprehensive comments**: Explained logic and structure

### Maintainability
- **Single source of truth**: Trading logic in one place
- **Easy modification**: Change once, affects all markets
- **Testable components**: Utility functions can be unit tested
- **Clear dependencies**: Explicit includes and namespaces

### Functionality
- **100% preserved**: All original functionality maintained
- **Same behavior**: Identical signals and execution
- **Same configuration**: Compatible input parameters
- **Same output**: Log format and chart visualization unchanged

## Architecture Highlights

### Hybrid Approach
- **Classes for data**: `trade_data`, `daily_levels`, `market_state`
- **Functions for logic**: Stateless utility functions in namespaces
- **Best of both worlds**: Structure without over-engineering

### DRY Principle
- Four market-specific files → One parameterized implementation
- Duplicate logging code → Centralized logging utilities
- Duplicate drawing code → Reusable drawing utilities
- Duplicate time checks → Time utility functions

### Namespace Organization
```cpp
iml::                       // Root namespace
├── constants               // In iml_constants.h
├── trading::               // In iml_trading.cpp
├── logging::               // In iml_logging.h
├── time::                  // In iml_time.h
├── drawing::               // In iml_drawing.h
└── orders::                // In iml_orders.h
```

## Troubleshooting

### Compilation Errors

**"Cannot find sierrachart.h"**
- Ensure files are in the Sierra Chart `ACS_Source` directory
- Check that `default/` folder exists with Sierra Chart headers

**"Undefined reference to..."**
- Make sure all `.cpp` files are in the compilation path
- Verify that inline functions in `.h` files are marked `inline`

### Runtime Issues

**"No trades executing"**
- Verify all four context studies are running
- Check that input references (Inputs 0-3) point to correct charts/studies
- Confirm trading window is active (current time within start/stop)
- Verify minimum range requirement is met
- Check that IML signal validity hasn't expired

**"Signals not appearing"**
- Ensure context studies are returning valid quadrant values
- Check correlation thresholds (sum ≤ -6 for long, ≥ 6 for short)
- Verify all four markets show correct divergence pattern

**"Drawings not visible"**
- Enable "Display Logs" input parameter
- Check that chart has sufficient space for text overlays

## Version History

### v3 (Current)
- Complete refactoring for readability and maintainability
- Eliminated ~1000 lines of code duplication
- Organized into logical modules (core/ and utilities/)
- Implemented DRY principles throughout
- Enhanced documentation

### v2
- Improved naming conventions (UPPERCASE constants)
- Better code formatting
- Enum definitions cleaned up

### v1 (Original)
- Initial complete implementation
- Four separate market files
- Basic structure and functionality

## Credits

**Original Author**: Lukas Kotatko
**Refactored**: 2026
**Platform**: Sierra Chart ACSIL
**Language**: C++

## License

Proprietary - For personal use only