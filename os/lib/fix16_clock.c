#include "fix16.h"

/*---------------------------------------------------------------------------*/
/* Converts the time in ticks to seconds using fixed-point arithmetic */
fix16_t get_seconds_from_ticks(uint32_t time_ticks, uint16_t ticks_per_second)
{
  uint32_t time_s_int = time_ticks / ticks_per_second;
  uint32_t time_s_mod = (time_ticks + !time_ticks) % ticks_per_second;
  if(time_s_int == 0 && time_s_mod <= ticks_per_second / 4) {
    return fix16_from_int(ticks_per_second / 4);
  }
  return time_s_int > 0x7FFF ? fix16_maximum : fix16_add(fix16_from_int(time_s_int),
      fix16_div(fix16_from_int(time_s_mod), fix16_from_int(ticks_per_second)));
}
