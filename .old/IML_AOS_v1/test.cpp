// The top of every source code file must include this line
#include "sierrachart.h"

// For reference, refer to this page:
// https://www.sierrachart.com/index.php?page=doc/AdvancedCustomStudyInterfaceAndLanguage.php

// This line is required. Change the text within the quote
// marks to what you want to name your group of custom studies.
SCDLLName("Testing Custom Study DLL")

    // This is the basic framework of a study function. Change the name 'TemplateFunction' to what you require.
    SCSFExport scsf_GetValuesFromTheSameChart(SCStudyInterfaceRef sc) {
    SCInputRef Input_StudySubgraphReference = sc.Input[0];

    if (sc.SetDefaults) {
        sc.UpdateAlways = 1;
        sc.GraphRegion = 0;

        Input_StudySubgraphReference.Name = "Study and Subgraph to Display";
        Input_StudySubgraphReference.SetStudySubgraphValues(7, 1);
        return;
    }

    SCFloatArray StudyReference;
    sc.GetStudyArrayUsingID(Input_StudySubgraphReference.GetStudyID(), Input_StudySubgraphReference.GetSubgraphIndex(), StudyReference);
    float StudyValue = StudyReference[sc.IndexOfLastVisibleBar];

    s_UseTool t;
    t.Clear();
    t.ChartNumber = sc.ChartNumber;
    t.DrawingType = DRAWING_TEXT;
    t.FontBackColor = RGB(0, 0, 0);
    t.FontSize = 20;
    t.FontBold = false;
    t.AddMethod = UTAM_ADD_OR_ADJUST;
    t.UseRelativeVerticalValues = 1;
    t.Color = RGB(255, 255, 255);
    t.Region = 0;
    t.Text.Format("t: %.03f", StudyValue);
    t.LineNumber = 20;
    t.BeginDateTime = 1;
    t.BeginValue = 70;
    sc.UseTool(t);
}

SCSFExport scsf_GetValuesFromAnotherCharts(SCStudyInterfaceRef sc) {
    SCInputRef Input_StudySubgraphReference = sc.Input[0];

    if (sc.SetDefaults) {
        Input_StudySubgraphReference.Name = "Study And Subgraph To Display";
        Input_StudySubgraphReference.SetChartStudySubgraphValues(3, 4, 0);
        return;
    }

    SCFloatArray StudyReference;
    sc.GetStudyArrayFromChartUsingID(Input_StudySubgraphReference.GetChartNumber(), Input_StudySubgraphReference.GetStudyID(), Input_StudySubgraphReference.GetSubgraphIndex(), StudyReference);
    float StudyValue = StudyReference[StudyReference.GetArraySize() - 1];

    s_UseTool t;
    t.Clear();
    t.ChartNumber = sc.ChartNumber;
    t.DrawingType = DRAWING_TEXT;
    t.FontBackColor = RGB(0, 0, 0);
    t.FontSize = 8;
    t.FontBold = false;
    t.AddMethod = UTAM_ADD_OR_ADJUST;
    t.UseRelativeVerticalValues = 1;
    t.Color = RGB(255, 255, 255);
    t.Region = 0;
    t.Text.Format("t: %.03f", StudyValue);
    t.LineNumber = 20;
    t.BeginDateTime = 1;
    t.BeginValue = 70;
    sc.UseTool(t);
}
