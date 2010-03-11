/***************************************************************************
*                  Adaptive Delta Code Size Computations
*
*   File    : adapt.c
*   Purpose : Module containing calculations used to adapt the code word
*             size used in adaptive delta encoding/decoding.  This is where
*             tweaks may be made to add intelligence to the adjustment of
*             the code word size.
*   Author  : Michael Dipperstein
*   Date    : April 29, 2009
*
****************************************************************************
*   UPDATES
*
*   $Id: adapt.c,v 1.1 2009/05/02 06:14:31 michael Exp $
*   $Log: adapt.c,v $
*   Revision 1.1  2009/05/02 06:14:31  michael
*   Refactor for easy changing of the rules for adjusting code size.
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
#include "adapt.h"

/***************************************************************************
*                                CONSTANTS
***************************************************************************/
/* maximum overflows and underflows before code size change */
#define MAX_OVF 3
#define MAX_UNF 3

/***************************************************************************
*                            TYPE DEFINITIONS
***************************************************************************/
typedef struct
{
    unsigned char codeSize;
    unsigned char overflowCount;
    unsigned char underflowCount;
} adaptive_data_t;

/***************************************************************************
*                            GLOBAL VARIABLES
**************************************************************************/
static adaptive_data_t adaptive;        /* adaptive code size data */

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

/***************************************************************************
*   Function   : InitializeAdaptiveData
*   Description: This function initializes the static data used to track
*                encoding/decoding statistics and detertime how the code
*                word size should be adapted.
*   Parameters : codeSize - The number of bits used for code words at the
*                           start of coding.
*   Effects    : The static data used to track encoding/decoding statistics
*                is initialized.
*   Returned   : None
***************************************************************************/
void InitializeAdaptiveData(const unsigned char codeSize)
{
    adaptive.codeSize = codeSize;
    adaptive.overflowCount = 0;
    adaptive.underflowCount = 0;
}

/***************************************************************************
*   Function   : UpdateAdaptiveStatistics
*   Description: This function initializes the static data used to track
*                encoding/decoding statistics and detertime how the code
*                word size should be adapted.
*   Parameters : stat - an indication of overflow, underflow, or neither
*                       used to determine the size of the next code word.
*   Effects    : Statistical counters are updated and a new code word
*                length may be determined.
*   Returned   : The number of bits to be used for the next code word.
***************************************************************************/
unsigned char UpdateAdaptiveStatistics(const code_word_stat_t stat)
{
    switch(stat)
    {
        case CS_OKAY:
            if (adaptive.overflowCount > 0)
            {
                adaptive.overflowCount--;
            }
            
            if (adaptive.underflowCount > 0)
            {
                adaptive.underflowCount--;
            }
            break;

        case CS_OVERFLOW:
            if (adaptive.underflowCount > 0)
            {
                adaptive.underflowCount--;
            }

            adaptive.overflowCount++;

            if (MAX_OVF < adaptive.overflowCount)
            {
                if (adaptive.codeSize < 8)
                {
                    adaptive.codeSize++;
                }

                adaptive.underflowCount = 0;
                adaptive.overflowCount = 0;
            }
            break;

        case CS_UNDERFLOW:
            if (adaptive.overflowCount > 0)
            {
                adaptive.overflowCount--;
            }

            adaptive.underflowCount++;

            if (MAX_UNF < adaptive.underflowCount)
            {
                if (adaptive.codeSize > 2)
                {
                    adaptive.codeSize--;
                }

                adaptive.underflowCount = 0;
                adaptive.overflowCount = 0;
            }
            break;
            
        default:
            break;
    }

    return adaptive.codeSize;
}
