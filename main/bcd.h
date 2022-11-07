#pragma once
#include <stdint.h>
typedef uint8_t bcd8_t;
inline uint8_t bcd8_to_uint8(bcd8_t x)
{
    return (x >> 4) * 10 + (x & 0X0F);
}
/*
 * unspecified if x is greater than 99
 */
inline bcd8_t uint8_to_bcd8(uint8_t x)
{
    return ((x / 10) << 4) + (x % 10);
}
inline void bcd8_to_dchar(char *dst, bcd8_t x)
{
    dst[0] = '0' + (x >> 4);
    dst[1] = '0' + (x & 0X0F);
}
inline bcd8_t bcd8_inc(bcd8_t x)
{
    if (x == 0x99)
        return 0;
    bcd8_t result = x + 1;
    if ((result & 0x0F) > 9)
    {
        result += 6;
    }
    return result;
}
inline bcd8_t bcd8_dec(bcd8_t x)
{
    if (x == 0)
        return 0x99;
    bcd8_t result = x - 1;
    if ((result & 0x0F) > 9)
    {
        result -= 6;
    }
    return result;
}