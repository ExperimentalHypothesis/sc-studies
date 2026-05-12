#include "../include/sierrachart.h"

SCDLLName("My First Study");

SCSFExport scsf_SimpleExample(SCStudyInterfaceRef sc)
{
    SCSubgraphRef SimpleLine = sc.Subgraph[0];

    if (sc.SetDefaults)
    {
        sc.GraphName = "Simple Line";
        sc.AutoLoop = 1;

        SimpleLine.Name = "Close";
        SimpleLine.DrawStyle = DRAWSTYLE_LINE;
        SimpleLine.PrimaryColor = RGB(0, 128, 255);

        return;
    }

    SimpleLine[sc.Index] = sc.Close[sc.Index];
}
