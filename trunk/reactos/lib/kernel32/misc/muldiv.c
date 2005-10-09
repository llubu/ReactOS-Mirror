/* $Id$
 *
 */
#include <k32.h>


/***********************************************************************
 *           MulDiv   (KERNEL32.@)
 * RETURNS
 *      Result of multiplication and division
 *      -1: Overflow occurred or Divisor was 0
 * FIXME! move to correct file
 *
 * @implemented
 */
INT
WINAPI
MulDiv(INT nNumber,
       INT nNumerator,
       INT nDenominator)
{
    LARGE_INTEGER Result;
    LONG Negative;
 
    /* Find out if this will be a negative result */
    Negative = nNumber ^ nNumerator ^ nDenominator;
 
    /* Turn all the parameters into absolute values */
    if (nNumber < 0) nNumber *= -1;
    if (nNumerator < 0) nNumerator *= -1;
    if (nDenominator < 0) nDenominator *= -1;
 
    /* Calculate the result */
    Result.QuadPart = Int32x32To64(nNumber, nNumerator) + (nDenominator / 2);
 
    /* Now check for overflow */
    if (nDenominator > Result.HighPart)
    {
        /* Divide the product to get the quotient and remainder */
        Result.LowPart = RtlEnlargedUnsignedDivide(*(PULARGE_INTEGER)&Result,
                                                   (ULONG)nDenominator,
                                                   &Result.HighPart);
 
        /* Do the sign changes */
        if ((LONG)Result.LowPart >= 0)
        {
            return (Negative >= 0) ? Result.LowPart : -(LONG)Result.LowPart;
        }
    }
 
    /* Return overflow */
    return - 1;
}

