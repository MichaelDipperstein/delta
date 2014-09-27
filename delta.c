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
*                            GLOBAL VARIABLES
**************************************************************************/

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/
static void MakeRange(signed char *min, signed char *max,
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
int DeltaEncodeFile(FILE *inFile, FILE *outFile, unsigned char codeSize)
{
    bit_file_t *bOutFile;
    int c;
    unsigned char buffer;
    signed char prev, delta, min, max;

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
    MakeRange(&min, &max, codeSize);

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

        if ((delta > max) || (delta <= min))
        {
            /* overflow write min (right justified) followed by the character */
            buffer = (unsigned char)min << (8 - codeSize);
            BitFilePutBits(bOutFile, &buffer, codeSize);
            BitFilePutChar(c, bOutFile);
            codeSize = UpdateAdaptiveStatistics(CS_OVERFLOW);
        }
        else
        {
            /* not an overflow.  right justify and output. */
            buffer = delta << (8 - codeSize);
            BitFilePutBits(bOutFile, &buffer, codeSize);

            /* check for underflow */
            if ((delta < (max / 2)) && (delta > (min / 2)))
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
        MakeRange(&min, &max, codeSize);
    }

    /* indicate end of stream with an overflow and previous value (EOF) */
    buffer = (unsigned char)min << (8 - codeSize);
    BitFilePutBits(bOutFile, &buffer, codeSize);
    BitFilePutChar(prev, bOutFile);

    outFile = BitFileToFILE(bOutFile);          /* make file normal again */
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
int DeltaDecodeFile(FILE *inFile, FILE *outFile, unsigned char codeSize)
{
    bit_file_t *bInFile;
    int c;
    unsigned char buffer;
    signed char prev, delta, min, max;

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
    MakeRange(&min, &max, codeSize);

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

        if ((signed char)buffer == min)
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
            if ((delta < (max / 2)) && (delta > (min / 2)))
            {
                codeSize = UpdateAdaptiveStatistics(CS_UNDERFLOW);
            }
            else
            {
                codeSize = UpdateAdaptiveStatistics(CS_OKAY);
            }
        }

        /* update range in case of code size change */
        MakeRange(&min, &max, codeSize);
    }

    inFile = BitFileToFILE(bInFile);            /* make file normal again */
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
static void MakeRange(signed char *min, signed char *max,
    const unsigned char codeSize)
{
    *min = (signed char)(0 - (1 << (codeSize - 1)));    /* -2^(n - 1) */
    *max = (signed char)((1 << (codeSize - 1)) - 1);    /* 2^(n  - 1) - 1 */
}
