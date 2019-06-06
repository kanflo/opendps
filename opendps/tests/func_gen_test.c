#include <stdio.h>
#include <stdlib.h>

typedef unsigned int uint32_t;

/**
 * @brief      Compute a square signal as selected by the user
 *
 * @param[in]  time   the current clock in microsecond 
 * @param[in]  period period of the generated signal in microsecond (frequency in range 0 to 5KHz, period is in range 200 to 1<<31)
 * @param[in]  max    the amplitude of the voltage for this generator
 *
 * @retval     int32_t the voltage to apply
 */
static int32_t square_gen(uint32_t time_in_us, uint32_t period_in_us, int32_t max)
{
    if (!period_in_us) return max;
    /** The time is ticking at 1000000Hz so a unit is 1us. The production is 1 for t in [(t%T) ; (t%T+T/2)], 0 otherwise */
    uint32_t t = time_in_us % period_in_us;
    return t <= period_in_us / 2 ? max : 0;
}

/**
 * @brief      Compute a saw signal as selected by the user
 *
 * @param[in]  time   the current clock in microsecond 
 * @param[in]  period period of the generated signal in microsecond (frequency in range 0 to 5KHz, period is in range 200 to 1<<31)
 * @param[in]  max    the amplitude of the voltage for this generator
 *
 * @retval     int32_t the voltage to apply
 */
static int32_t saw_gen(uint32_t time_in_us, uint32_t period_in_us, int32_t max)
{
    if (!period_in_us) return max;
    /** The time is ticking at 1000000Hz so a unit is 1us. The production is (max*(t%T))/T */
    uint32_t t = time_in_us % period_in_us;
    return (t * max) / period_in_us;
}

/*
 * The number of bits of our data type: here 16 (sizeof operator returns bytes).
 */
#define INT16_BITS  (8 * sizeof(int16_t))
#ifndef INT16_MAX
#define INT16_MAX   ((1<<(INT16_BITS-1))-1)
#endif
 
/*
 * "5 bit" large table = 32 values. The mask: all bit belonging to the table
 * are 1, the all above 0.
 */
#define TABLE_BITS  (5)
#define TABLE_SIZE  (1<<TABLE_BITS)
#define TABLE_MASK  (TABLE_SIZE-1)
 
/*
 * The lookup table is to 90DEG, the input can be -360 to 360 DEG, where negative
 * values are transformed to positive before further processing. We need two
 * additional bits (*4) to represent 360 DEG:
 */
#define LOOKUP_BITS (TABLE_BITS+2)
#define LOOKUP_MASK ((1<<LOOKUP_BITS)-1)
#define FLIP_BIT    (1<<TABLE_BITS)
#define NEGATE_BIT  (1<<(TABLE_BITS+1))
#define INTERP_BITS (INT16_BITS-1-LOOKUP_BITS)
#define INTERP_MASK ((1<<INTERP_BITS)-1)
 
/**
 * "5 bit" lookup table for the offsets. These are the sines for exactly
 * at 0deg, 11.25deg, 22.5deg etc. The values are from -1 to 1 in Q15.
 */
static int16_t sin90[TABLE_SIZE+1] = {
  0x0000,0x0647,0x0c8b,0x12c7,0x18f8,0x1f19,0x2527,0x2b1e,
  0x30fb,0x36b9,0x3c56,0x41cd,0x471c,0x4c3f,0x5133,0x55f4,
  0x5a81,0x5ed6,0x62f1,0x66ce,0x6a6c,0x6dc9,0x70e1,0x73b5,
  0x7640,0x7883,0x7a7c,0x7c29,0x7d89,0x7e9c,0x7f61,0x7fd7,
  0x7fff
};
 
/**
 * Sine calculation using interpolated table lookup.
 * Instead of radiants or degrees we use "turns" here. Means this
 * sine does NOT return one phase for 0 to 2*PI, but for 0 to 1.
 * Input: -1 to 1 as int16 Q15  == -32768 to 32767.
 * Output: -1 to 1 as int16 Q15 == -32768 to 32767.
 *
 * See the full description at www.AtWillys.de for the detailed
 * explanation.
 *
 * @param int16_t angle Q15
 * @return int16_t Q15
 */
static int16_t sin1(int16_t angle)
{
  int16_t v0, v1;
  if(angle < 0) { angle += INT16_MAX; angle += 1; }
  v0 = (angle >> INTERP_BITS);
  if(v0 & FLIP_BIT) { v0 = ~v0; v1 = ~angle; } else { v1 = angle; }
  v0 &= TABLE_MASK;
  v1 = sin90[v0] + (int16_t) (((int32_t) (sin90[v0+1]-sin90[v0]) * (v1 & INTERP_MASK)) >> INTERP_BITS);
  if((angle >> INTERP_BITS) & NEGATE_BIT) v1 = -v1;
  return v1;
}


/**
 * @brief      Compute a sin signal as selected by the user
 *
 * @param[in]  time   the current clock in microsecond 
 * @param[in]  period period of the generated signal in microsecond (frequency in range 0 to 5KHz, period is in range 200 to 1<<31)
 * @param[in]  max    the amplitude of the voltage for this generator
 *
 * @retval     int32_t the voltage to apply
 */
static int32_t sin_gen(uint32_t time_in_us, uint32_t period_in_us, int32_t max)
{
    if (!period_in_us) return max;
    /** The time is ticking at 1000000Hz so a unit is 1us. The production is (max/2) * sin((t%T)/T*2*PI) + (max/2) */
    uint32_t t = time_in_us % period_in_us;
    
    return (max/2) * sin1((int16_t)((t * 32768) / period_in_us)) / 32768 + max/2; // There's a small error in amplitude here to avoid dividing by 32767
}


int main(int argc, char ** argv)
{
    uint32_t period = 10000/4;
    FILE * out = fopen("funcgen.csv", "wb");
    if (!out) return 1;

    for (uint32_t i = 0; i < 10000; i++)
    {
        int32_t q = square_gen(i, period, 20000), a = saw_gen(i, period, 20000), n = sin_gen(i, period, 20000);
        fprintf(out, "%d;%d;%d;%d\n", i, q, a, n);
    }
    fclose(out);
    return 0;
}