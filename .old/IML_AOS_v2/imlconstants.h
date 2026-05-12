  
#pragma once
#ifndef _IMLCONSTANTS_
#define _IMLCONSTATNS_

#include "sierrachart.h"

//constants for entering position
constexpr int MAX_SUM_FOR_SHORT = -9;
constexpr int MIN_SUM_FOR_SHORT = -6;
constexpr int MAX_SUM_FOR_LONG = 9;
constexpr int MIN_SUM_FOR_LONG = 6;

constexpr int qdr_pos_limit{ 2 };

enum Quadrants {
	BREAK_LOWEST = -3, 
    LOWEST = -2, 
    SECOND_LOWEST = -1,
    SECOND_HIGHEST = 1,
    HIGHEST = 2, 
    BREAK_HIGHEST = 3
};



bool static is_time_to_trade(SCStudyInterfaceRef sc){
    if (sc.BaseDateTimeIn[sc.IndexOfLastVisibleBar].GetTime() > sc.Input[17].GetTime() && sc.BaseDateTimeIn[sc.IndexOfLastVisibleBar].GetTime() < sc.Input[18].GetTime())
        return 1;
    return 0;
}





#endif