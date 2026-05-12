#include "../include/sierrachart.h"

#include <cmath>
#include <unordered_set>

SCDLLName("PriceQuickness");

SCSFExport scsf_PriceQuicknessCounter(SCStudyInterfaceRef sc)
{
    SCSubgraphRef UniquePriceCountPlot = sc.Subgraph[0];
    SCSubgraphRef NewPriceFlag = sc.Subgraph[1];

    SCInputRef ResetOnNewSession = sc.Input[0];

    if (sc.SetDefaults)
    {
        sc.GraphName = "Price Quickness Counter";
        sc.StudyDescription = "Counts how many new price levels have traded since the last reset";
        sc.AutoLoop = 0;
        sc.GraphRegion = 1;

        UniquePriceCountPlot.Name = "Unique Price Count";
        UniquePriceCountPlot.DrawStyle = DRAWSTYLE_LINE;
        UniquePriceCountPlot.PrimaryColor = RGB(0, 128, 255);
        UniquePriceCountPlot.DrawZeros = true;

        NewPriceFlag.Name = "New Price Event";
        NewPriceFlag.DrawStyle = DRAWSTYLE_BAR;
        NewPriceFlag.PrimaryColor = RGB(255, 128, 0);
        NewPriceFlag.DrawZeros = false;

        ResetOnNewSession.Name = "Reset On New Session";
        ResetOnNewSession.SetYesNo(true);

        return;
    }

    auto* SeenPrices = static_cast<std::unordered_set<int>*>(sc.GetPersistentPointer(0));
    if (SeenPrices == nullptr)
    {
        SeenPrices = new std::unordered_set<int>();
        sc.SetPersistentPointer(0, SeenPrices);
    }

    int& UniquePriceCount = sc.GetPersistentInt(0);

    if (sc.IsFullRecalculation)
    {
        SeenPrices->clear();
        UniquePriceCount = 0;
        sc.UpdateStartIndex = 0;
    }

    if (sc.TickSize <= 0.0)
        return;

    const int StartIndex = sc.UpdateStartIndex;
    for (int Index = StartIndex; Index < sc.ArraySize; ++Index)
    {
        if (ResetOnNewSession.GetYesNo() && Index > 0 && sc.IsNewTradingDay(Index))
        {
            SeenPrices->clear();
            UniquePriceCount = 0;
        }

        const double Price = sc.Close[Index];
        const int PriceAsTicks = static_cast<int>(std::llround(Price / sc.TickSize));

        const bool WasEmpty = SeenPrices->empty();
        const bool Inserted = SeenPrices->insert(PriceAsTicks).second;

        if (Inserted && !WasEmpty)
        {
            ++UniquePriceCount;
            NewPriceFlag[Index] = 1.0f;
        }
        else
        {
            NewPriceFlag[Index] = 0.0f;
        }

        UniquePriceCountPlot[Index] = static_cast<float>(UniquePriceCount);
    }
}
