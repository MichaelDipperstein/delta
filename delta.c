/***************************************************************************
*                     Adaptive Delta Encoding Library
*
*   File    : delta.c
*   Purpose : Library providing adaptive delta encoding/decoding functions.
*   Author  : Michael Dipperstein
*   Date    : April 16, 2009
*
****************************************************************************
*
* Delta: An adaptive delta encoding/decoding library
* Copyright (C) 2009, 2014 by
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
#include "adapt.h"
#include "bitfile.h"

/***************************************************************************
*                                CONSTANTS
***************************************************************************/

/***************************************************************************
*                            TYPE DEFINITIONS
***************************************************************************/
typedef struct
{
    signed char min;
    signed char max;
} range_t;

/***************************************************************************
*                            GLOBAL VARIABLES
**************************************************************************/

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/
static range_t MakeRange(const unsigned char codeSize);

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

/***************************************************************************
*   Function   : DeltaEncodeFile
*   Description: This function reads from the specified input stream and
*                writes an adaptive delta encoded version to the specified
*                output stream.  If input/output streams are NULL, this
*                function exits with a failure.
*   Parameters : inFile - Pointer to a file stream to be encoded.
*                outFile - Pointer to a file where the encoded output should
*                          be written.
*                codeSize - The number of bits used for code words at the
*                           start of coding.
*   Effects    : Data from the inFile stream will be encoded and written to
*                the outFile stream.
*   Returned   : EXIT_SUCCESS for success otherwise EXIT_FAILURE.
***************************************************************************/
int DeltaEncodeFile(FILE *inFile, FILE *outFile, unsigned char codeSize)
{
    bit_file_t *bOutFile;
    int c;
    unsigned char buffer;
    signed char prev, delta;
    range_t range;

    /* verify parameters */
    if ((codeSize < 2) || (codeSize > 8))
    {
        /* code size is out of range */
        return EXIT_FAILURE;
    }

    if (NULL == inFile)
    {
        return EXIT_FAILURE;
    }

    if (NULL == outFile)
    {
        return EXIT_FAILURE;
    }

    bOutFile = MakeBitFile(outFile, BF_WRITE);

    if (NULL == bOutFile)
    {
        perror("Making Output File a BitFile");
        fclose(outFile);
        fclose(inFile);
        return EXIT_FAILURE;
    }

    /* initialize data */
    InitializeAdaptiveData(codeSize);
    range = MakeRange(codeSize);

    /* get first value */
    if ((c = fgetc(inFile)) != EOF)
    {
        BitFilePutChar(c, bOutFile);
        prev = c;
    }
    else
    {
        /* empty input file */
        fclose(inFile);
        BitFileClose(bOutFile);
        return EXIT_SUCCESS;
    }

    while ((c = fgetc(inFile)) != EOF)
    {
        if (EOF == c)
        {
            break;
        }

        delta = (signed char)c - prev;
        prev = c;

        if ((delta > range.max) || (delta <= range.min))
        {
            /* overflow write min (right justified) followed by the character */
            buffer = (unsigned char)range.min << (8 - codeSize);
            BitFilePutBits(bOutFile, &buffer, codeSize);
            BitFilePutChar(c, bOutFile);
            codeSize = UpdateAdaptiveStatistics(CS_OVERFLOW);
        }
        else
        {
            /* not an overflow.  right justify and output. */
            buffer = (unsigned char)delta << (8 - codeSize);
            BitFilePutBits(bOutFile, &buffer, codeSize);

            /* check for underflow */
            if ((delta <= (range.max / 2)) && (delta > (range.min / 2)))
            {
                /* underflow */
                codeSize = UpdateAdaptiveStatistics(CS_UNDERFLOW);
            }
            else
            {
                codeSize = UpdateAdaptiveStatistics(CS_OKAY);
            }
        }

        /* update range in case of code size change */
        range = MakeRange(codeSize);
    }

    /* indicate end of stream with an overflow and previous value (EOF) */
    buffer = (unsigned char)range.min << (8 - codeSize);
    BitFilePutBits(bOutFile, &buffer, codeSize);
    BitFilePutChar(prev, bOutFile);

    outFile = BitFileToFILE(bOutFile);          /* make file normal again */
    return EXIT_SUCCESS;
}

/***************************************************************************
*   Function   : DeltaDecodeFile
*   Description: This function reads from the specified adaptive delta
*                encoded input stream and writes a decoded version to the
*                specified output stream.  If input/output streams are NULL,
*                this function exits with a failure.
*   Parameters : inFile - Pointer to the adaptive delta encoded file stream
*                         to be encoded.
*                outFile - Pointer to a file where the decoded output should
*                          be written.
*                codeSize - The number of bits used for code words at the
*                           start of coding.
*   Effects    : Data from the inFile stream will be decoded and written to
*                the outFile stream.
*   Returned   : EXIT_SUCCESS for success otherwise EXIT_FAILURE.
***************************************************************************/
int DeltaDecodeFile(FILE *inFile, FILE *outFile, unsigned char codeSize)
{
    bit_file_t *bInFile;
    int c;
    unsigned char buffer;
    signed char prev, delta;
    range_t range;

    /* verify parameters */
    if ((codeSize < 2) || (codeSize > 8))
    {
        /* code size is out of range */
        return EXIT_FAILURE;
    }

    if (NULL == inFile)
    {
        return EXIT_FAILURE;
    }

    if (NULL == outFile)
    {
        return EXIT_FAILURE;
    }

    bInFile = MakeBitFile(inFile, BF_WRITE);

    if (NULL == bInFile)
    {
        perror("Making Input File a BitFile");
        fclose(outFile);
        fclose(inFile);
        return EXIT_FAILURE;
    }

    InitializeAdaptiveData(codeSize);
    range = MakeRange(codeSize);

    /* get first value */
    if ((c = BitFileGetChar(bInFile)) != EOF)
    {
        fputc(c, outFile);
        prev = (signed char)c;
    }
    else
    {
        /* empty input file */
        BitFileClose(bInFile);
        fclose(outFile);
        return EXIT_SUCCESS;
    }

    while (BitFileGetBits(bInFile, &buffer, codeSize) != EOF)
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

        if ((signed char)buffer == range.min)
        {
            /* overflow character */
            c = BitFileGetChar(bInFile);

            if ((EOF == c) || (prev == c))
            {
                /* overflow without change signals EOF as does real EOF */
                break;
            }

            fputc(c, outFile);
            prev = (signed char)c;
            codeSize = UpdateAdaptiveStatistics(CS_OVERFLOW);
        }
        else
        {
            /* not an overflow */
            delta = (signed char)buffer;
            prev = prev + delta;
            fputc(prev, outFile);

            /* check for underflow */
            if ((delta <= (range.max / 2)) && (delta > (range.min / 2)))
            {
                codeSize = UpdateAdaptiveStatistics(CS_UNDERFLOW);
            }
            else
            {
                codeSize = UpdateAdaptiveStatistics(CS_OKAY);
            }
        }

        /* update range in case of code size change */
        range = MakeRange(codeSize);
    }

    inFile = BitFileToFILE(bInFile);            /* make file normal again */
    return EXIT_SUCCESS;
}

/***************************************************************************
*   Function   : MakeRange
*   Description: This function computes the minimum and maximum range
*                limits for a codeSize bit delta encoded symbol.
*   Parameters : codeSize - The number of bits used for code words at the
*                           start of coding.
*   Effects    : None
*   Returned   : Range minimum and maximum data values for delta codes of
*                codeSize bits.
***************************************************************************/
static range_t MakeRange(const unsigned char codeSize)
{
    range_t range;
    range.min = (signed char)(0 - (1 << (codeSize - 1)));   /* -2^(n - 1) */
    range.max = (signed char)((1 << (codeSize - 1)) - 1);   /* 2^(n  - 1) - 1 */

    return range;
}
