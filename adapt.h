/***************************************************************************
*          Header for Adaptive Delta Encoding and Decoding Library
*
*   File    : adapt.h
*   Purpose : Provides prototypes for functions that compute the code word
*             size for adaptive delta encoding/decoding.
*   Author  : Michael Dipperstein
*   Date    : April 29, 2009
*
****************************************************************************
*   UPDATES
*
*   $Id: adapt.h,v 1.1 2009/05/02 06:14:31 michael Exp $
*   $Log: adapt.h,v $
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

#ifndef _ADAPT_H_
#define _ADAPT_H_

/***************************************************************************
*                                CONSTANTS
***************************************************************************/

/***************************************************************************
*                            TYPE DEFINITIONS
***************************************************************************/
typedef enum
{
    CS_OKAY,
    CS_OVERFLOW,
    CS_UNDERFLOW
} code_word_stat_t;

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/
/* initializes adaptive code size computations */
void InitializeAdaptiveData(const unsigned char codeSize);

/* returns code size for next code word based on fit of current code word */
unsigned char UpdateAdaptiveStatistics(const code_word_stat_t stat);

#endif  /* ndef _ADAPT_H_ */
