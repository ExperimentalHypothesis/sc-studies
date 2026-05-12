// The top of every source code file must include this line
#include "sierrachart.h"

// For reference, refer to this page:
// https://www.sierrachart.com/index.php?page=doc/AdvancedCustomStudyInterfaceAndLanguage.php

// This line is required. Change the text within the quote
// marks to what you want to name your group of custom studies.
SCDLLName("Custom Study DLL")

    // This is the basic framework of a study function. Change the name 'TemplateFunction' to what you require.
    SCSFExport scsf_IML_TRADE(SCStudyInterfaceRef sc) {

    SCInputRef iml_buffer_validity = sc.Input[4];
    SCInputRef minimal_range = sc.Input[6];
    SCInputRef display_ut = sc.Input[8];

    SCInputRef start_trading = sc.Input[17];
    SCInputRef stop_trading = sc.Input[18];
    SCInputRef time_to_flat_dt = sc.Input[19];

    start_trading.Name = "Trade from: ";
    start_trading.SetTime(HMS_TIME(8, 30, 0));
    stop_trading.Name = "Trade from: ";
    stop_trading.SetTime(HMS_TIME(10, 00, 0));

    display_ut.Name = "Display Logs";
    display_ut.SetYesNo(TRUE);
    time_to_flat_dt.Name = "Time to flat";
    time_to_flat_dt.SetTime(HMS_TIME(10, 30, 0));

    sc.Input[0].SetChartStudyValues(1, 1);
    sc.Input[1].SetChartStudyValues(2, 1);
    sc.Input[2].SetChartStudyValues(3, 1);
    sc.Input[3].SetChartStudyValues(4, 1);

    sc.Input[4].Name = "How many mins the IML is valid?";
    sc.Input[4].SetInt(10);

    sc.Input[5].SetChartStudyValues(sc.ChartNumber, 1);
    
	sc.Input[6].Name = "Minimal range in ticks";
    sc.Input[6].SetInt(22);

    sc.GraphRegion = 0;
    sc.GraphName = "main: IML REV - Get persists and trade";
    sc.AutoLoop = 1;
    sc.UpdateAlways = TRUE;
    return;
}

    sc.AllowMultipleEntriesInSameDirection = true;
    sc.MaximumPositionAllowed = 1000;
    sc.SupportReversals = false;
    sc.SendOrdersToTradeService = false;
    sc.AllowOppositeEntryWithOpposingPositionOrOrders = true;
    sc.SupportAttachedOrdersForTrading = true;
    sc.CancelAllOrdersOnEntriesAndReversals = false;
    sc.AllowEntryWithWorkingOrders = false;
    sc.CancelAllWorkingOrdersOnExit = false;
    sc.AllowOnlyOneTradePerBar = true;

	    // geting the return values from the context study (break dhl)


}
