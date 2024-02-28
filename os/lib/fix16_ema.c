#include "fix16.h"

/*---------------------------------------------------------------------------*/
/* Exponential Moving Average (EMA) function. */
fix16_t fix16_ema(fix16_t prev_ema, fix16_t new_val, fix16_t time_diff_secs, fix16_t tau)
{
  fix16_t ema_wgt = fix16_exp(fix16_div(-time_diff_secs, tau));
  return fix16_add(fix16_mul(prev_ema, ema_wgt), fix16_mul(new_val, fix16_sub(fix16_one, ema_wgt)));
}
