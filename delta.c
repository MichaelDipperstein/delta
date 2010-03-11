/***************************************************************************
*                     Adaptive Delta Encoding Library
*
*   File    : delta.c
*   Purpose : Library providing adaptive delta encoding/decoding functions.
*   Author  : Michael Dipperstein
*   Date    : April 16, 2009
*
****************************************************************************
*   UPDATES
*
*   $Id: delta.c,v 1.1.1.1 2009/04/17 04:35:52 michael Exp $
*   $Log: delta.c,v $
*   Revision 1.1.1.1  2009/04/17 04:35:52  michael
*   Initial release
*
****************************************************************************
*
* Delta: An adaptive delta encoding/decoding library
* Copyright (C) 2009 by
*       Michael Dipperstein (mdipper@alumni.engr.ucsb.edu)
*
* This file is part of the Delta library.
*
* Delta is free software; you can redistribute it and/or modify it under
* the terms of the GNU Lesser General Public License as published by the
* Free Software Foundation; either version 3 of the License, or (at your
* option) any later version.
*
* Delta is distributed in the hope that it will be useful, but WITHOUT ANY
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
* License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************/

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "bitfile.h"

/***************************************************************************
*                                CONSTANTS
***************************************************************************/
/* maximum overflows and underflows before code size change */
#define MAX_OVF 3
#define MAX_UNF 3

/***************************************************************************
*                            GLOBAL VARIABLES
**************************************************************************/

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/
void MakeRange(signed char *min, signed char *max,
    const unsigned char codeSize);

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

/***************************************************************************
*   Function   : DeltaEncodeFile
*   Description: This function reads from the specified input stream and
*                writes a adaptive delta encoded version to the specified
*                output stream.  If input/output stream names are NULL,
*                stdin/stdout will be used.
*   Parameters : inFile - Pointer to a null terminated string containing
*                         the name of the file to be encoded.
*                outFile - Pointer to a null terminated string containing
*                          the name of the file where the encoded output
*                          should be written.
*                codeSize - The number of bits used for code words at the
*                           start of coding.
*   Effects    : A file named with the name provided by outFile will be
*                created and contain the delta encoding of the data
*                in the file named with the name provided by inFile.
*   Returned   : EXIT_SUCCESS for success otherwise EXIT_FAILURE.
***************************************************************************/
int DeltaEncodeFile(const char *inFile, const char *outFile,
    unsigned char codeSize)
{
    FILE *fpIn;
    bit_file_t *bfpOut;
    int c;
    unsigned char overflowCount, underflowCount, buffer;
    signed char prev, delta, min, max;
    
    if ((codeSize < 2) || (codeSize > 8))
    {
        /* code size is out of range */
        return EXIT_FAILURE;
    }

    /* open file to be encoded */
    if (NULL != inFile)
    {
        fpIn = fopen(inFile, "rb");
    }
    else
    {
        fpIn = stdin;
    }

    if (NULL == fpIn)
    {
        perror(inFile);
        return EXIT_FAILURE;
    }

    /* open output file */
    if (NULL != outFile)
    {
        bfpOut = BitFileOpen(outFile, BF_WRITE);
    }
    else
    {
        bfpOut = MakeBitFile(stdout, BF_WRITE);
    }

    if (NULL == bfpOut)
    {
        perror(outFile);
        fclose(fpIn);
        return EXIT_FAILURE;
    }

    /* initialize data */
    MakeRange(&min, &max, codeSize);
    overflowCount = 0;
    underflowCount = 0;

    /* get first value */
    if ((c = fgetc(fpIn)) != EOF)
    {
        BitFilePutChar(c, bfpOut);
        prev = c;
    }
    else
    {
        /* empty input file */
        fclose(fpIn);
        BitFileClose(bfpOut);
        return EXIT_SUCCESS;
    }

    while ((c = fgetc(fpIn)) != EOF)
    {
        if (EOF == c)
        {
            break;
        }

        delta = (signed char)c - prev;
        prev = c;

        if ((delta > max) || (delta <= min))
        {
            /* overflow write min (right justified) followed by the character */
            buffer = (unsigned char)min << (8 - codeSize);
            BitFilePutBits(bfpOut, &buffer, codeSize);
            BitFilePutChar(c, bfpOut);
            overflowCount++;

            if (MAX_OVF < overflowCount)
            {
                if (codeSize < 8)
                {
                    codeSize++;
                    MakeRange(&min, &max, codeSize);
                }

                underflowCount = 0;
                overflowCount = 0;
            }
        }
        else
        {
            /* not an overflow.  right justify and output. */
            buffer = delta << (8 - codeSize);
            BitFilePutBits(bfpOut, &buffer, codeSize);

            if (overflowCount > 0)
            {
                overflowCount--;
            }

            /* check for underflow */
            if ((delta < (max / 2)) && (delta > (min / 2)))
            {
                /* underflow */
                underflowCount++;

                if (MAX_UNF < underflowCount)
                {
                    if (codeSize > 2)
                    {
                        codeSize--;
                        MakeRange(&min, &max, codeSize);
                    }

                    underflowCount = 0;
                    overflowCount = 0;
                }

            }
            else if (underflowCount > 0)
            {
                underflowCount--;
            }
        }
    }

    /* indicate end of stream with an overflow and previous value (EOF) */
    buffer = (unsigned char)min << (8 - codeSize);
    BitFilePutBits(bfpOut, &buffer, codeSize);
    BitFilePutChar(prev, bfpOut);

    fclose(fpIn);
    BitFileClose(bfpOut);
    return EXIT_SUCCESS;
}

/***************************************************************************
*   Function   : DeltaDecodeFile
*   Description: This function reads an a adaptive delta encoded file from
*                the specified input stream and writes decoded version to
*                the specified output stream.  If input/output stream names
*                are NULL, stdin/stdout will be used.
*   Parameters : inFile - Pointer to a null terminated string containing
*                         the name of the file to be decoded.
*                outFile - Pointer to a null terminated string containing
*                          the name of the file where the decoded output
*                          should be written.
*                codeSize - The number of bits used for code words at the
*                           start of coding.
*   Effects    : A file named with the name provided by outFile will be
*                created and contain the delta decoding of the data
*                in the file named with the name provided by inFile.
*   Returned   : EXIT_SUCCESS for success otherwise EXIT_FAILURE.
***************************************************************************/
int DeltaDecodeFile(const char *inFile, const char *outFile,
    unsigned char codeSize)
{
    bit_file_t *bfpIn;
    FILE *fpOut;
    int c;
    unsigned char overflowCount, underflowCount, buffer;
    signed char prev, delta, min, max;

    if ((codeSize < 2) || (codeSize > 8))
    {
        /* code size is out of range */
        return EXIT_FAILURE;
    }

    /* open file to be decoded */
    if (NULL != inFile)
    {
        bfpIn = BitFileOpen(inFile, BF_READ);
    }
    else
    {
        bfpIn = MakeBitFile(stdin, BF_READ);
    }

    if (NULL == bfpIn)
    {
        perror(inFile);
        return EXIT_FAILURE;
    }

    /* open output file */
    if (NULL != outFile)
    {
        fpOut = fopen(outFile, "wb");
    }
    else
    {
        fpOut = stdout;
    }

    if (NULL == fpOut)
    {
        perror(outFile);
        BitFileClose(bfpIn);
        return EXIT_FAILURE;
    }

    MakeRange(&min, &max, codeSize);
    overflowCount = 0;
    underflowCount = 0;

    /* get first value */
    if ((c = BitFileGetChar(bfpIn)) != EOF)
    {
        fputc(c, fpOut);
        prev = (signed char)c;
    }
    else
    {
        /* empty input file */
        BitFileClose(bfpIn);
        fclose(fpOut);
        return EXIT_SUCCESS;
    }

    while (BitFileGetBits(bfpIn, &buffer, codeSize) != EOF)
    {
        /* right justify buffer */
        if (buffer & 0x80)
        {
            /* buffer is negative */
            buffer >>= (8 - codeSize);
            buffer |= 0xFF << (codeSize);
        }
        else
        {
            /* buffer is possitive */
            buffer >>= (8 - codeSize);
        }

        if ((signed char)buffer == min)
        {
            /* overflow character */
            c = BitFileGetChar(bfpIn);

            if ((EOF == c) || (prev == c))
            {
                /* overflow without change signals EOF as does real EOF */
                break;
            }

            fputc(c, fpOut);
            prev = (signed char)c;
            overflowCount++;

            if (MAX_OVF < overflowCount)
            {
                if (codeSize < 8)
                {
                    codeSize++;
                    MakeRange(&min, &max, codeSize);
                }

                underflowCount = 0;
                overflowCount = 0;
            }
        }
        else
        {
            /* not an overflow */
            delta = (signed char)buffer;
            prev = prev + delta;
            fputc(prev, fpOut);
    
            if (overflowCount > 0)
            {
                overflowCount--;
            }

            /* check for underflow */
            if ((delta < (max / 2)) && (delta > (min / 2)))
            {
                underflowCount++;

                if (MAX_UNF < underflowCount)
                {
                    /* underflow */
                    if (codeSize > 2)
                    {
                        codeSize--;
                        MakeRange(&min, &max, codeSize);
                    }

                    underflowCount = 0;
                    overflowCount = 0;
                }
            }
            else if (underflowCount > 0)
            {
                underflowCount--;
            }
        }
    }

    BitFileClose(bfpIn);
    fclose(fpOut);
    return EXIT_SUCCESS;
}

/***************************************************************************
*   Function   : MakeRange
*   Description: This function computes the minimum and maximum range
*                limits for a codeSize bit delta encoded symbol.
*   Parameters : min - pointer to variable to contain the minimum value.
*                max - pointer to variable to contain the maximum value.
*                codeSize - The number of bits used for code words at the
*                           start of coding.
*   Effects    : min and max will be assigned the minimum and maximum data
*                values for delta codes of codeSize bits.
*   Returned   : None
***************************************************************************/
void MakeRange(signed char *min, signed char *max, const unsigned char codeSize)
{
    *min = (signed char)(0 - (1 << (codeSize - 1)));    /* -2^(n - 1) */
    *max = (signed char)((1 << (codeSize - 1)) - 1);    /* 2^(n  - 1) - 1 */
}
