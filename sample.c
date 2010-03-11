/***************************************************************************
*                 Sample Program Using Delta Encoding Library
*
*   File    : sample.c
*   Purpose : Demonstrate usage of Delta encoding library
*   Author  : Michael Dipperstein
*   Date    : April 16, 2008
*
****************************************************************************
*   UPDATES
*
*   $Id: sample.c,v 1.1.1.1 2009/04/17 04:35:52 michael Exp $
*   $Log: sample.c,v $
*   Revision 1.1.1.1  2009/04/17 04:35:52  michael
*   Initial release
*
****************************************************************************
*
* SAMPLE: Sample usage of Delta Encoding Library
* Copyright (C) 2009 by
*       Michael Dipperstein (mdipper@alumni.engr.ucsb.edu)
*
* This file is part of the delta library.
*
* The delta library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 3 of the
* License, or (at your option) any later version.
*
* The delta library is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
* General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************/

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "optlist.h"
#include "delta.h"

/***************************************************************************
*                                CONSTANTS
***************************************************************************/
#define DEFAULT_SIZE 6

typedef enum
{
    MODE_ENCODE,
    MODE_DECODE
} modes_t;

/***************************************************************************
*                            GLOBAL VARIABLES
***************************************************************************/

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/
char *RemovePath(char *fullPath);
void ShowUsage(char *progName);

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

/****************************************************************************
*   Function   : main
*   Description: This is the main function for this program, it validates
*                the command line input and, if valid, it will call
*                functions to encode or decode a file using the adaptive
*                delta coding algorithm.
*   Parameters : argc - number of parameters
*                argv - parameter list
*   Effects    : Encodes/Decodes input file
*   Returned   : EXIT_SUCCESS for success, otherwise EXIT_FAILURE.
****************************************************************************/
int main(int argc, char *argv[])
{
    char *inFile, *outFile;
    unsigned char codeSize;
    modes_t mode;
    option_t *optList, *thisOpt;

    /* initialize variables */
    inFile = NULL;
    outFile = NULL;
    codeSize = DEFAULT_SIZE;
    mode = MODE_ENCODE;

    /* parse command line */
    optList = GetOptList(argc, argv, "cds:i:o:h?");
    thisOpt = optList;

    while (thisOpt != NULL)
    {
        switch(thisOpt->option)
        {
            case 'c':       /* compression mode */
                mode = MODE_ENCODE;
                break;

            case 'd':       /* decompression mode */
                mode = MODE_DECODE;
                break;

            case 's':       /* size of starting code word */
                codeSize = atoi(thisOpt->argument);

                if ((codeSize < 2) || (codeSize > 8))
                {
                    fprintf(stderr,
                        "Starting code size must be between 2 and 8.\n\n");
                    ShowUsage(RemovePath(argv[0]));

                    if (inFile != NULL)
                    {
                        free(inFile);
                    }

                    if (outFile != NULL)
                    {
                        free(outFile);
                    }

                    FreeOptList(optList);
                    return EXIT_FAILURE;
                }

                break;

            case 'i':       /* input file name */
                if (inFile != NULL)
                {
                    fprintf(stderr, "Multiple input files not allowed.\n\n");
                    ShowUsage(RemovePath(argv[0]));
                    free(inFile);

                    if (outFile != NULL)
                    {
                        free(outFile);
                    }

                    FreeOptList(optList);
                    return EXIT_FAILURE;
                }
                else if ((inFile =
                    (char *)malloc(strlen(thisOpt->argument) + 1)) == NULL)
                {
                    perror("Memory allocation");

                    if (outFile != NULL)
                    {
                        free(outFile);
                    }

                    FreeOptList(optList);
                    return EXIT_FAILURE;
                }

                strcpy(inFile, thisOpt->argument);
                break;

            case 'o':       /* output file name */
                if (outFile != NULL)
                {
                    fprintf(stderr, "Multiple output files not allowed.\n\n");
                    ShowUsage(RemovePath(argv[0]));
                    free(outFile);

                    if (inFile != NULL)
                    {
                        free(inFile);
                    }

                    FreeOptList(optList);
                    return EXIT_FAILURE;
                }
                else if ((outFile =
                    (char *)malloc(strlen(thisOpt->argument) + 1)) == NULL)
                {
                    perror("Memory allocation");

                    if (inFile != NULL)
                    {
                        free(inFile);
                    }

                    FreeOptList(optList);
                    return EXIT_FAILURE;
                }

                strcpy(outFile, thisOpt->argument);
                break;

            case 'h':
            case '?':
                ShowUsage(RemovePath(argv[0]));
                FreeOptList(optList);
                return EXIT_SUCCESS;
        }

        optList = thisOpt->next;
        free(thisOpt);
        thisOpt = optList;
    }

    if (MODE_ENCODE == mode)
    {
        if(DeltaEncodeFile(inFile, outFile, codeSize) != EXIT_SUCCESS)
        {
            fprintf(stderr, "Failed to Encode %s\n", inFile);
        }
    }
    else if (MODE_DECODE == mode)
    {
        if(DeltaDecodeFile(inFile, outFile, codeSize) != EXIT_SUCCESS)
        {
            fprintf(stderr, "Failed to Decode %s\n", inFile);
        }
    }

    free(inFile);
    free(outFile);
    return EXIT_SUCCESS;
}

/****************************************************************************
*   Function   : RemovePath
*   Description: This is function accepts a pointer to the name of a file
*                along with path information and returns a pointer to the
*                character that is not part of the path.
*   Parameters : fullPath - pointer to an array of characters containing
*                           a file name and possible path modifiers.
*   Effects    : None
*   Returned   : Returns a pointer to the first character after any path
*                information.
****************************************************************************/
char *RemovePath(char *fullPath)
{
    int i;
    char *start, *tmp;                          /* start of file name */
    const char delim[3] = {'\\', '/', ':'};     /* path deliminators */

    start = fullPath;

    /* find the first character after all file path delimiters */
    for (i = 0; i < 3; i++)
    {
        tmp = strrchr(start, delim[i]);

        if (tmp != NULL)
        {
            start = tmp + 1;
        }
    }

    return start;
}

/****************************************************************************
*   Function   : ShowUsage
*   Description: This function sends instructions for using this program to
*                stdout.
*   Parameters : progName - the name of the executable version of this
*                           program.
*   Effects    : Usage instructions are sent to stdout.
*   Returned   : None
****************************************************************************/
void ShowUsage(char *progName)
{
    printf("Usage: %s <options>\n\n", progName);
    printf("Options:\n");
    printf("  -c : encode input.\n");
    printf("  -d : decode input.\n");
    printf("  -s : initial codeword size (2 - 8 bits).\n");
    printf("  -i <filename> : Name of input file.\n");
    printf("  -o <filename> : Name of output file.\n");
    printf("  -h | ?  : Print out command line options.\n\n");
    printf("Default: %s -s%d -c -i stdin -o stdout\n",
        progName, DEFAULT_SIZE);
}
