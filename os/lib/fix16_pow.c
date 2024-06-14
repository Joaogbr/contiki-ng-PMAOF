#include "fix16.h"

/*---------------------------------------------------------------------------*/
/* Exponentiation Function. */
fix16_t fix16_pow(fix16_t base, fix16_t exponent)
{
  fix16_t exp;
  if(base <= 0) {
    return 0;
  } /*else if(base < 0) {
    int16_t rounded_base = fix16_to_int(base);
    exp = fix16_mul(exponent, fix16_log(-fix16_from_int(rounded_base)));
    return (rounded_base % 2 ? -fix16_exp(exp) : fix16_exp(exp));
  }*/
  exp = fix16_mul(exponent, fix16_log(base));
  return fix16_exp(exp);
}
