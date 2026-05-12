#include "../include/sierrachart.h"

SCDLLName("AboveThreshold");

SCSFExport scsf_HighVolumeMarker(SCStudyInterfaceRef sc)
{
    SCSubgraphRef VolumeMarker = sc.Subgraph[0];
    SCInputRef VolumeThreshold = sc.Input[0];

    if (sc.SetDefaults)
    {
        sc.GraphName = "High Volume Marker";
        sc.StudyDescription = "Marks bars where volume exceeds a given threshold";
        sc.AutoLoop = 1;
        sc.GraphRegion = 0;

        VolumeMarker.Name = "High Volume";
        VolumeMarker.DrawStyle = DRAWSTYLE_ARROW_UP;
        VolumeMarker.PrimaryColor = RGB(0, 255, 0);
        VolumeMarker.LineWidth = 2;
        VolumeMarker.DrawZeros = false;

        VolumeThreshold.Name = "Volume Threshold";
        VolumeThreshold.SetInt(10000);
        VolumeThreshold.SetIntLimits(1, INT_MAX);

        return;
    }

    if (sc.Volume[sc.Index] > VolumeThreshold.GetInt())
        VolumeMarker[sc.Index] = sc.Low[sc.Index] - sc.TickSize * 2.0f;
    else
        VolumeMarker[sc.Index] = 0.0f;
}

SCSFExport scsf_HighAskBidVolumeMarker(SCStudyInterfaceRef sc)
{
    SCSubgraphRef AskMarker = sc.Subgraph[0];
    SCSubgraphRef BidMarker = sc.Subgraph[1];

    SCInputRef AskVolumeThreshold = sc.Input[0];
    SCInputRef BidVolumeThreshold = sc.Input[1];

    if (sc.SetDefaults)
    {
        sc.GraphName = "High Ask/Bid Volume Marker";
        sc.StudyDescription = "Marks bars where Ask or Bid volume exceeds threshold";
        sc.AutoLoop = 1;
        sc.GraphRegion = 0;

        AskMarker.Name = "High Ask Volume";
        AskMarker.DrawStyle = DRAWSTYLE_ARROW_UP;
        AskMarker.PrimaryColor = RGB(0, 255, 0);
        AskMarker.LineWidth = 2;
        AskMarker.DrawZeros = false;

        BidMarker.Name = "High Bid Volume";
        BidMarker.DrawStyle = DRAWSTYLE_ARROW_DOWN;
        BidMarker.PrimaryColor = RGB(255, 0, 0);
        BidMarker.LineWidth = 2;
        BidMarker.DrawZeros = false;

        AskVolumeThreshold.Name = "Ask Volume Threshold";
        AskVolumeThreshold.SetInt(5000);
        AskVolumeThreshold.SetIntLimits(1, INT_MAX);

        BidVolumeThreshold.Name = "Bid Volume Threshold";
        BidVolumeThreshold.SetInt(5000);
        BidVolumeThreshold.SetIntLimits(1, INT_MAX);

        return;
    }

    if (sc.AskVolume[sc.Index] > AskVolumeThreshold.GetInt())
        AskMarker[sc.Index] = sc.Low[sc.Index] - sc.TickSize * 2.0f;
    else
        AskMarker[sc.Index] = 0.0f;

    if (sc.BidVolume[sc.Index] > BidVolumeThreshold.GetInt())
        BidMarker[sc.Index] = sc.High[sc.Index] + sc.TickSize * 2.0f;
    else
        BidMarker[sc.Index] = 0.0f;
}

SCSFExport scsf_HighAskBidVolumeTrader(SCStudyInterfaceRef sc)
{
    SCSubgraphRef BuySignal = sc.Subgraph[0];
    SCSubgraphRef SellSignal = sc.Subgraph[1];

    SCInputRef AskVolumeThreshold = sc.Input[0];
    SCInputRef BidVolumeThreshold = sc.Input[1];
    SCInputRef OrderQuantity = sc.Input[2];
    SCInputRef EnableTrading = sc.Input[3];

    if (sc.SetDefaults)
    {
        sc.GraphName = "High Ask/Bid Volume Trader";
        sc.StudyDescription = "Marks bars and optionally opens trades when Ask/Bid volume exceeds threshold";
        sc.AutoLoop = 1;
        sc.GraphRegion = 0;

        sc.AllowMultipleEntriesInSameDirection = false;
        sc.MaximumPositionAllowed = 1;
        sc.SupportReversals = true;
        sc.AllowOppositeEntryWithOpposingPositionOrOrders = true;
        sc.SupportAttachedOrdersForTrading = false;
        sc.CancelAllOrdersOnEntriesAndReversals = true;
        sc.AllowEntryWithWorkingOrders = false;
        sc.CancelAllWorkingOrdersOnExit = true;
        sc.SendOrdersToTradeService = true;

        BuySignal.Name = "Buy Signal";
        BuySignal.DrawStyle = DRAWSTYLE_ARROW_UP;
        BuySignal.PrimaryColor = RGB(0, 255, 0);
        BuySignal.LineWidth = 2;
        BuySignal.DrawZeros = false;

        SellSignal.Name = "Sell Signal";
        SellSignal.DrawStyle = DRAWSTYLE_ARROW_DOWN;
        SellSignal.PrimaryColor = RGB(255, 0, 0);
        SellSignal.LineWidth = 2;
        SellSignal.DrawZeros = false;

        AskVolumeThreshold.Name = "Ask Volume Threshold";
        AskVolumeThreshold.SetInt(5000);
        AskVolumeThreshold.SetIntLimits(1, INT_MAX);

        BidVolumeThreshold.Name = "Bid Volume Threshold";
        BidVolumeThreshold.SetInt(5000);
        BidVolumeThreshold.SetIntLimits(1, INT_MAX);

        OrderQuantity.Name = "Order Quantity";
        OrderQuantity.SetInt(1);
        OrderQuantity.SetIntLimits(1, 100);

        EnableTrading.Name = "Enable Trading";
        EnableTrading.SetYesNo(false);

        return;
    }

    const bool AskCondition = sc.AskVolume[sc.Index] > AskVolumeThreshold.GetInt();
    const bool BidCondition = sc.BidVolume[sc.Index] > BidVolumeThreshold.GetInt();

    BuySignal[sc.Index] = AskCondition ? sc.Low[sc.Index] - sc.TickSize * 2.0f : 0.0f;
    SellSignal[sc.Index] = BidCondition ? sc.High[sc.Index] + sc.TickSize * 2.0f : 0.0f;

    if (sc.IsFullRecalculation || !EnableTrading.GetYesNo())
        return;

    if (sc.GetBarHasClosedStatus() != BHCS_BAR_HAS_CLOSED)
        return;

    int& LastProcessedIndex = sc.GetPersistentInt(0);
    if (LastProcessedIndex == sc.Index)
        return;
    LastProcessedIndex = sc.Index;

    s_SCPositionData PositionData;
    sc.GetTradePosition(PositionData);

    const bool IsLong = PositionData.PositionQuantity > 0.0;
    const bool IsShort = PositionData.PositionQuantity < 0.0;

    if (AskCondition && !IsLong)
    {
        s_SCNewOrder NewOrder;
        NewOrder.OrderQuantity = static_cast<float>(OrderQuantity.GetInt());
        NewOrder.OrderType = SCT_ORDERTYPE_MARKET;
        sc.BuyEntry(NewOrder);
    }

    if (BidCondition && !IsShort)
    {
        s_SCNewOrder NewOrder;
        NewOrder.OrderQuantity = static_cast<float>(OrderQuantity.GetInt());
        NewOrder.OrderType = SCT_ORDERTYPE_MARKET;
        sc.SellEntry(NewOrder);
    }
}
