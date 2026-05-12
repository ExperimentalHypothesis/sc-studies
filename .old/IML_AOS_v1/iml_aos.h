#pragma once
#include "sierrachart.h"

constexpr int max_sum_short{ -9 };
constexpr int min_sum_short{ -6}; //-6
constexpr int max_sum_long{ 9 };
constexpr int min_sum_long{ 6 }; //6
constexpr int qdr_pos_limit{ 2 };

bool static is_time_to_trade(SCStudyInterfaceRef sc){
    if (sc.BaseDateTimeIn[sc.IndexOfLastVisibleBar].GetTime() > sc.Input[17].GetTime() && sc.BaseDateTimeIn[sc.IndexOfLastVisibleBar].GetTime() < sc.Input[18].GetTime())
        return 1;
    return 0;
}


// SCString log_sc_ym(SCStudyInterfaceRef sc);
//SCString log_sc_nq(SCStudyInterfaceRef sc);
//SCString log_sc_es(SCStudyInterfaceRef sc);
//SCString log_sc_rty(SCStudyInterfaceRef sc);

// void log_txt_ym(SCStudyInterfaceRef sc);
//void log_txt_nq(SCStudyInterfaceRef sc);
//void log_txt_es(SCStudyInterfaceRef sc);
//void log_txt_rty(SCStudyInterfaceRef sc);



