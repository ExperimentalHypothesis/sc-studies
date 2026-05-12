// IML Reversal AOS — Inter-Market Liquidity Reversal Automated Order System
//
// Mean-reversion across ES / YM / NQ / RTY. The "Context" study, added to each
// of the four charts, classifies the current close into a quadrant of that
// market's daily range. The "REV" trading study, added to whichever chart you
// want to trade, reads the four quadrant values via chart-study references and
// fires when the three "other" markets diverge against the "self" market.
//
// Build:  ./bash/build.sh src/IML-REV-AOS/main.cpp

#include "../../include/sierrachart.h"

SCDLLName("IML-REV-AOS")

// ---- Quadrant constants ----------------------------------------------------
namespace iml {
constexpr int BREAK_LOW  = -3;
constexpr int LOW        = -2;
constexpr int LOW_MID    = -1;
constexpr int HIGH_MID   = +1;
constexpr int HIGH       = +2;
constexpr int BREAK_HIGH = +3;

// Persistent-slot layout exported by scsf_IML_Context — the trading study
// reads these by index via sc.GetPersistentDoubleFromChartStudy / float.
constexpr int SLOT_QUADRANT       = 0;   // float
constexpr int SLOT_DAILY_HIGH     = 0;   // double
constexpr int SLOT_DAILY_LOW      = 1;
constexpr int SLOT_MID            = 2;
constexpr int SLOT_DAILY_LOW_MID  = 3;
constexpr int SLOT_DAILY_HIGH_MID = 4;
}

// ============================================================================
// scsf_IML_Context — daily H/L tracker + quadrant publisher
// ============================================================================
SCSFExport scsf_IML_Context(SCStudyInterfaceRef sc)
{
    SCSubgraphRef sgDailyHigh    = sc.Subgraph[0];
    SCSubgraphRef sgDailyLow     = sc.Subgraph[1];
    SCSubgraphRef sgMid          = sc.Subgraph[2];
    SCSubgraphRef sgDailyLowMid  = sc.Subgraph[3];
    SCSubgraphRef sgDailyHighMid = sc.Subgraph[4];

    SCInputRef inRthStart      = sc.Input[0];
    SCInputRef inBreakMinsBack = sc.Input[1];
    SCInputRef inDisplay       = sc.Input[2];

    if (sc.SetDefaults)
    {
        sc.GraphName    = "IML Context — quadrant & daily levels";
        sc.AutoLoop     = 1;
        sc.GraphRegion  = 0;
        sc.UpdateAlways = 1;
        sc.FreeDLL      = 0;

        sgDailyHigh.Name = "Daily High";
        sgDailyHigh.DrawStyle = DRAWSTYLE_LINE;
        sgDailyHigh.LineStyle = LINESTYLE_DOT;
        sgDailyHigh.PrimaryColor = RGB(0, 255, 0);
        sgDailyHigh.DrawZeros = false;

        sgDailyLow.Name = "Daily Low";
        sgDailyLow.DrawStyle = DRAWSTYLE_LINE;
        sgDailyLow.LineStyle = LINESTYLE_DOT;
        sgDailyLow.PrimaryColor = RGB(255, 0, 0);
        sgDailyLow.DrawZeros = false;

        sgMid.Name = "Mid";
        sgMid.DrawStyle = DRAWSTYLE_LINE;
        sgMid.LineStyle = LINESTYLE_DOT;
        sgMid.PrimaryColor = RGB(255, 255, 255);
        sgMid.DrawZeros = false;

        sgDailyLowMid.Name = "Daily Low Mid";
        sgDailyLowMid.DrawStyle = DRAWSTYLE_LINE;
        sgDailyLowMid.LineStyle = LINESTYLE_DOT;
        sgDailyLowMid.PrimaryColor = RGB(120, 120, 120);
        sgDailyLowMid.DrawZeros = false;

        sgDailyHighMid.Name = "Daily High Mid";
        sgDailyHighMid.DrawStyle = DRAWSTYLE_LINE;
        sgDailyHighMid.LineStyle = LINESTYLE_DOT;
        sgDailyHighMid.PrimaryColor = RGB(120, 120, 120);
        sgDailyHighMid.DrawZeros = false;

        inRthStart.Name = "RTH start";
        inRthStart.SetTime(HMS_TIME(8, 30, 0));

        inBreakMinsBack.Name = "Mins back for break detection";
        inBreakMinsBack.SetInt(10);

        inDisplay.Name = "Display info on chart";
        inDisplay.SetYesNo(true);
        return;
    }

    double& dailyHigh    = sc.GetPersistentDouble(iml::SLOT_DAILY_HIGH);
    double& dailyLow     = sc.GetPersistentDouble(iml::SLOT_DAILY_LOW);
    double& mid          = sc.GetPersistentDouble(iml::SLOT_MID);
    double& dailyLowMid  = sc.GetPersistentDouble(iml::SLOT_DAILY_LOW_MID);
    double& dailyHighMid = sc.GetPersistentDouble(iml::SLOT_DAILY_HIGH_MID);

    auto recomputeLevels = [&]() {
        mid          = (dailyHigh + dailyLow) * 0.5;
        dailyLowMid  = (mid + dailyLow) * 0.5;
        dailyHighMid = (mid + dailyHigh) * 0.5;
    };

    // Reset on the bar at RTH start (or on first bar of a chart recalc).
    const bool firstBar    = (sc.Index == 0);
    const bool rthStartBar = (sc.BaseDateTimeIn[sc.Index].GetTime() == inRthStart.GetTime());
    if (firstBar || rthStartBar)
    {
        dailyHigh = sc.High[sc.Index];
        dailyLow  = sc.Low[sc.Index];
        recomputeLevels();
    }
    if (sc.High[sc.Index] > dailyHigh) { dailyHigh = sc.High[sc.Index]; recomputeLevels(); }
    if (sc.Low[sc.Index]  < dailyLow ) { dailyLow  = sc.Low[sc.Index];  recomputeLevels(); }

    sgDailyHigh   [sc.Index] = static_cast<float>(dailyHigh);
    sgDailyLow    [sc.Index] = static_cast<float>(dailyLow);
    sgMid         [sc.Index] = static_cast<float>(mid);
    sgDailyLowMid [sc.Index] = static_cast<float>(dailyLowMid);
    sgDailyHighMid[sc.Index] = static_cast<float>(dailyHighMid);

    // Classify the current close. Break states only apply when a new H/L
    // was made within the last N minutes AND the close is still on that side
    // of mid (i.e. the break hasn't already reverted).
    const double close = sc.Close[sc.Index];
    int quadrant;
    if      (close >= dailyHighMid) quadrant = iml::HIGH;
    else if (close >= mid)          quadrant = iml::HIGH_MID;
    else if (close >  dailyLowMid)  quadrant = iml::LOW_MID;
    else                            quadrant = iml::LOW;

    const SCDateTime cutoff = sc.BaseDateTimeIn[sc.Index]
                              - SCDateTime::MINUTES(inBreakMinsBack.GetInt());
    const int cutoffIdx = sc.GetNearestMatchForSCDateTime(sc.ChartNumber, cutoff);
    if (cutoffIdx >= 0 && cutoffIdx < sc.Index)
    {
        if (dailyHigh > sgDailyHigh[cutoffIdx] && close > dailyHighMid)
            quadrant = iml::BREAK_HIGH;
        else if (dailyLow < sgDailyLow[cutoffIdx] && close < dailyLowMid)
            quadrant = iml::BREAK_LOW;
    }

    sc.GetPersistentFloat(iml::SLOT_QUADRANT) = static_cast<float>(quadrant);

    if (inDisplay.GetYesNo() && sc.Index == sc.ArraySize - 1)
    {
        s_UseTool t;
        t.Clear();
        t.ChartNumber              = sc.ChartNumber;
        t.DrawingType              = DRAWING_TEXT;
        t.AddMethod                = UTAM_ADD_OR_ADJUST;
        t.UseRelativeVerticalValues = 1;
        t.Region                   = 0;
        t.LineNumber               = 12;
        t.BeginDateTime            = 1;
        t.BeginValue               = 96;
        t.FontSize                 = 8;
        t.FontBackColor            = RGB(0, 0, 0);
        t.Color                    = RGB(255, 255, 255);
        t.Text.Format("%s quadrant: %+d  (range: %.0f t)",
                      sc.GetChartSymbol(sc.ChartNumber).GetChars(),
                      quadrant,
                      (dailyHigh - dailyLow) / sc.TickSize);
        sc.UseTool(t);
    }
}

// ============================================================================
// scsf_IML_REV — generic trading study (works on any of ES/YM/NQ/RTY)
// ============================================================================
SCSFExport scsf_IML_REV(SCStudyInterfaceRef sc)
{
    SCInputRef inSelfContext  = sc.Input[0];
    SCInputRef inOther1       = sc.Input[1];
    SCInputRef inOther2       = sc.Input[2];
    SCInputRef inOther3       = sc.Input[3];
    SCInputRef inValidityMins = sc.Input[4];
    SCInputRef inMinRangeTks  = sc.Input[5];
    SCInputRef inSumThreshold = sc.Input[6];
    SCInputRef inQty          = sc.Input[7];
    SCInputRef inTradeFrom    = sc.Input[8];
    SCInputRef inTradeUntil   = sc.Input[9];
    SCInputRef inFlattenTime  = sc.Input[10];
    SCInputRef inDisplay      = sc.Input[11];

    if (sc.SetDefaults)
    {
        sc.GraphName    = "IML REV — divergence trader";
        sc.AutoLoop     = 1;
        sc.GraphRegion  = 0;
        sc.UpdateAlways = 1;
        sc.FreeDLL      = 0;

        inSelfContext.Name = "Self market context";
        inSelfContext.SetChartStudyValues(sc.ChartNumber, 1);

        inOther1.Name = "Other market 1 context";
        inOther1.SetChartStudyValues(1, 1);
        inOther2.Name = "Other market 2 context";
        inOther2.SetChartStudyValues(2, 1);
        inOther3.Name = "Other market 3 context";
        inOther3.SetChartStudyValues(3, 1);

        inValidityMins.Name = "Signal validity (minutes)";
        inValidityMins.SetInt(10);

        inMinRangeTks.Name = "Minimum daily range (ticks)";
        inMinRangeTks.SetInt(10);

        inSumThreshold.Name = "Correlation threshold (abs)";
        inSumThreshold.SetInt(6);

        inQty.Name = "Order quantity";
        inQty.SetInt(1);

        inTradeFrom.Name = "Trade from";
        inTradeFrom.SetTime(HMS_TIME(8, 30, 0));
        inTradeUntil.Name = "Trade until";
        inTradeUntil.SetTime(HMS_TIME(10, 0, 0));
        inFlattenTime.Name = "Flatten at";
        inFlattenTime.SetTime(HMS_TIME(10, 30, 0));

        inDisplay.Name = "Display info on chart";
        inDisplay.SetYesNo(true);
        return;
    }

    // Route to the broker (server-side OCO for the attached target+stop bracket)
    // when Global Trade Simulation is off; otherwise simulate locally.
    sc.SendOrdersToTradeService                            = !sc.GlobalTradeSimulationIsOn;
    sc.SupportAttachedOrdersForTrading                     = true;
    sc.AllowMultipleEntriesInSameDirection                 = false;
    sc.AllowEntryWithWorkingOrders                         = false;
    sc.AllowOnlyOneTradePerBar                             = true;
    sc.MaximumPositionAllowed                              = 1000;
    sc.CancelAllOrdersOnEntriesAndReversals                = false;
    sc.CancelAllWorkingOrdersOnExit                        = false;

    auto readQuadrant = [&](SCInputRef& ref) {
        return sc.GetPersistentFloatFromChartStudy(
            ref.GetChartNumber(), ref.GetStudyID(), iml::SLOT_QUADRANT);
    };
    auto readSelfLevel = [&](int slot) {
        return sc.GetPersistentDoubleFromChartStudy(
            inSelfContext.GetChartNumber(), inSelfContext.GetStudyID(), slot);
    };

    const int qSelf = static_cast<int>(readQuadrant(inSelfContext));
    const int q1    = static_cast<int>(readQuadrant(inOther1));
    const int q2    = static_cast<int>(readQuadrant(inOther2));
    const int q3    = static_cast<int>(readQuadrant(inOther3));
    const int othersSum = q1 + q2 + q3;

    const double dailyHigh    = readSelfLevel(iml::SLOT_DAILY_HIGH);
    const double dailyLow     = readSelfLevel(iml::SLOT_DAILY_LOW);
    const double dailyLowMid  = readSelfLevel(iml::SLOT_DAILY_LOW_MID);
    const double dailyHighMid = readSelfLevel(iml::SLOT_DAILY_HIGH_MID);

    const double rangePrice  = dailyHigh - dailyLow;
    const double rangeTicks  = rangePrice / sc.TickSize;
    const double stopOffset  = dailyHighMid - dailyLowMid;
    const int    stopTicks   = static_cast<int>(stopOffset / sc.TickSize + 0.5);

    int&         signalSide = sc.GetPersistentInt(0);          // 0 / +1 long / -1 short
    SCDateTime&  signalTime = sc.GetPersistentSCDateTime(0);

    const int threshold = inSumThreshold.GetInt();
    const SCDateTime now  = sc.BaseDateTimeIn[sc.Index];
    const SCDateTime expiry = signalTime + SCDateTime::MINUTES(inValidityMins.GetInt());

    // Arm signal on fresh divergence.
    if (othersSum <= -threshold && q1 < 0 && q2 < 0 && q3 < 0 && qSelf > 0 && signalSide != +1)
    {
        signalSide = +1;
        signalTime = now;
        sc.AddMessageToLog(SCString().Format("IML armed LONG  (sum=%+d, self=%+d)", othersSum, qSelf), 0);
    }
    else if (othersSum >= threshold && q1 > 0 && q2 > 0 && q3 > 0 && qSelf < 0 && signalSide != -1)
    {
        signalSide = -1;
        signalTime = now;
        sc.AddMessageToLog(SCString().Format("IML armed SHORT (sum=%+d, self=%+d)", othersSum, qSelf), 0);
    }

    // Disarm: expired, or correlation realigned against the armed side.
    if (signalSide != 0)
    {
        const bool expired   = (now > expiry);
        const bool realigned = (signalSide == +1 && othersSum > 0)
                            || (signalSide == -1 && othersSum < 0);
        if (expired || realigned) signalSide = 0;
    }

    s_SCPositionData pos;
    sc.GetTradePosition(pos);
    const bool flat = (pos.PositionQuantity == 0);

    const int curTime     = sc.BaseDateTimeIn[sc.Index].GetTime();
    const bool inWindow   = (curTime >= inTradeFrom.GetTime() && curTime < inTradeUntil.GetTime());
    const bool rangeOk    = (rangeTicks >= inMinRangeTks.GetInt());

    // Entry — MIT order at the self-market's mid level, attached limit target
    // at the opposite mid, attached stop the same distance the other side.
    // Break-even moves at 50% of the way to target.
    if (signalSide != 0 && inWindow && rangeOk && flat && stopTicks > 0)
    {
        s_SCNewOrder o;
        o.OrderType                          = SCT_ORDERTYPE_MARKET_IF_TOUCHED;
        o.OrderQuantity                      = inQty.GetInt();
        o.AttachedOrderTarget1Type           = SCT_ORDERTYPE_LIMIT;
        o.AttachedOrderStopAllType           = SCT_ORDERTYPE_STOP;
        o.MoveToBreakEven.Type               = MOVETO_BE_ACTION_TYPE_OFFSET_TRIGGERED;
        o.MoveToBreakEven.TriggerOffsetInTicks    = stopTicks / 2;
        o.MoveToBreakEven.BreakEvenLevelOffsetInTicks = 0;

        if (signalSide == +1)
        {
            o.Price1       = dailyLowMid;            // MIT trigger
            o.Target1Price = dailyHighMid;
            o.Stop1Price   = dailyLowMid - stopOffset;
            if (sc.BuyEntry(o) > 0)
            {
                sc.AddMessageToLog(SCString().Format(
                    "IML LONG submitted at %.2f, tgt %.2f, stop %.2f",
                    o.Price1, o.Target1Price, o.Stop1Price), 0);
                signalSide = 0;
            }
        }
        else
        {
            o.Price1       = dailyHighMid;
            o.Target1Price = dailyLowMid;
            o.Stop1Price   = dailyHighMid + stopOffset;
            if (sc.SellEntry(o) > 0)
            {
                sc.AddMessageToLog(SCString().Format(
                    "IML SHORT submitted at %.2f, tgt %.2f, stop %.2f",
                    o.Price1, o.Target1Price, o.Stop1Price), 0);
                signalSide = 0;
            }
        }
    }

    // End-of-window flatten + cancel.
    if (curTime >= inFlattenTime.GetTime() && (pos.PositionQuantity != 0 || pos.WorkingOrdersExist))
    {
        sc.FlattenAndCancelAllOrders();
        sc.AddMessageToLog("IML flatten at configured time", 0);
    }

    if (inDisplay.GetYesNo() && sc.Index == sc.ArraySize - 1)
    {
        s_UseTool t;
        t.Clear();
        t.ChartNumber              = sc.ChartNumber;
        t.DrawingType              = DRAWING_TEXT;
        t.AddMethod                = UTAM_ADD_OR_ADJUST;
        t.UseRelativeVerticalValues = 1;
        t.Region                   = 0;
        t.LineNumber               = 200;
        t.BeginDateTime            = 1;
        t.BeginValue               = 93;
        t.FontSize                 = 8;
        t.FontBackColor            = RGB(0, 0, 0);
        t.Color                    = RGB(255, 255, 255);
        const char* sideStr = (signalSide == +1) ? "LONG" : (signalSide == -1) ? "SHORT" : "—";
        t.Text.Format("IML  self %+d  others %+d %+d %+d (sum %+d)  armed: %s",
                      qSelf, q1, q2, q3, othersSum, sideStr);
        sc.UseTool(t);
    }
}
