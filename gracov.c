/* *********************************************************//** @file gracov.c
@copyright
Copyright (C) 2020  Richard Michael Thomas <rmthomas@sciolus.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
______________________________________________________________________________
@brief Utility for creating SVG graphs from files created by program cov.
*//* *************************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#ifndef MESS
/** Diagnostic trap: immediate termination */
#define MESS do { \
                fprintf(stderr, "\nTRAP in file %s, line %i, %s()\n", \
                __FILE__,__LINE__,__func__); exit(-32); \
                } while (0)
#endif /*MESS*/

#define MANYIN         (4)  /**< Maximum number of input files               */
#define MANYCOLUMN   (128)  /**< Maximum number of columns in any input file */
#define BFSZ        (8192)  /**< Maximum number of chars in any line
                                                           of any input file */
#define BFSZHDR     (1024)  /**< Maximum number of chars in a column header  */
#define ORDINATE_MAX (128)  /**< Maximum number of chars in a Y label        */

char outdir[FILENAME_MAX];         /**< Path to output directory             */
char infnm[MANYIN][FILENAME_MAX];  /**< Path to input files                  */
char outfnm[FILENAME_MAX];         /**< Path to the file  gracov.out         */
char cmdfnm[FILENAME_MAX];         /**< Path to the file  gnuplot.cmd        */

char title[FILENAME_MAX];          /**< Title to be displayed                */
char ylabel[ORDINATE_MAX];         /**< Y label to be displayed              */

FILE *pfcmd;                       /**< Pointer for gnuplot command file     */

char   wantkolumn[MANYIN];                  /**< Column number  of interest  */
char   want[MANYIN][BFSZ];                  /**< Column heading of interest  */
char   head[MANYIN][MANYCOLUMN][BFSZ];      /**< Column headings             */
double data[MANYIN][MANYCOLUMN];            /**< Column content              */

/**************************************************************************//**
@brief   Create a gnuplot command file from plain-text files comprising
         columns of numerical data.  The output file is always ./gnuplot.cmd.

USAGE:   ./gracov  filename1  headname1 [filename2  headname2 [...]]         \n
         where  filename1  is the full path to an input file in which there
         is a column of data indentified by string  headname1  on the very
         first line of said file.

         The permitted number of filename-headname pairs is determined at
         compile-time by the paramater MANYIN.  The size of records in the
         file are determined at compile-time by the parameters
         MANYCOLUMN, BFSZ and BFSZHDR.
  
@return         Zero unless error.
*//* *************************************************************************/
int
 main(int argc, char *argv[])
{
struct tm tm0, *ptm0; time_t t0;
FILE *pfin[MANYIN], *pfout;
struct stat stat0;
int manyin, manykolumn[MANYIN], blank[MANYIN];
int isbad, m, n, kolumn, kolout, lineout, line[MANYIN];
int latch;
char bf[MANYIN][BFSZ], natname[MANYIN][BFSZHDR];
char datetimestr[32];
char c, *p1, *p2;
int rc;

rc = 0;
if ( 0 > ( t0 = time(NULL) ) ) MESS;
if ( NULL == ( ptm0 = gmtime( &t0 ) ) ) MESS;  tm0 = *ptm0;
if ( ! strftime(&datetimestr[0], 32, "%Y-%m-%d-%H%M", &tm0)) MESS;
/*-----------------------------------------------------------------------------
PROCESS THE COMMAND LINE
-----------------------------------------------------------------------------*/
isbad = 0;
if ((argc - 1) % 2) isbad++;
manyin = (argc - 1)/2;
if (0 >= manyin) isbad++;
if (( ! isbad) && (MANYIN < manyin))
  {
  fprintf(stderr,"ERROR: too many input files\n"); isbad++;
  }
if (isbad)
  {
  fprintf(stderr,"USAGE: ./gracov  file1  hdr1 [file2  hdr2 [...]]\n");
  fprintf(stderr,"       for 1, 2, 3 or 4 filename-headername pairs\n");
  return -1;
  }
isbad = 0;
for (n = 1; n < argc; n += 2)
  {
  strncpy(&infnm[n/2][0], argv[n], FILENAME_MAX-2);
  if (strcmp(&infnm[n/2][0], argv[n]))
    {
    fprintf(stderr,"ERROR: bad command-line token: %s\n", argv[n]); isbad++;
    }
  if (stat(&infnm[n/2][0], &stat0))
    {
    fprintf(stderr,"ERROR: missing input file: %s\n", &infnm[n/2][0]); isbad++;
    }
  strncpy(&want[n/2][0], argv[n+1], BFSZ-2);
  if (strcmp(&want[n/2][0], argv[n+1]))
    {
    fprintf(stderr,"ERROR: bad command-line token: %s\n", argv[n+1]); isbad++;
    }
  }
if (isbad) return -1;
p1 = strncpy(&outfnm[0], "gracov.out", FILENAME_MAX-2);
if (strcmp(p1, &outfnm[0])) MESS;
p1 = strncpy(&cmdfnm[0], "gnuplot.cmd", FILENAME_MAX-2);
if (strcmp(p1, &cmdfnm[0])) MESS;

isbad = 0;
for (n = 0; n < manyin; n++)
  {
  if ( ! (pfin[n] = fopen(&infnm[n][0], "r")))
    {
    fprintf(stderr, "ERROR: cannot open input file: %s\n", &infnm[n][0]);
    isbad++;
    }
  }
if (isbad) return -1;
if ( ! (pfout = fopen(&outfnm[0], "w")))
  {
  fprintf(stderr, "ERROR: cannot open output file: %s\n", &outfnm[0]);
  return -1;
  }

for (n = 0; n < manyin; n++)
  {
  manykolumn[n] = 0; bf[n][BFSZ-4] = 0; line[n] = 0; blank[n] = 0; 
  wantkolumn[n] = -1;
  }
lineout = 0;
while (1)
  {
  kolout = 0;
  for (n = 0; n < manyin; n++)
    {
    if ( ! blank[n])
      {
      if (NULL == (p1 = fgets(&bf[n][0], BFSZ-2, pfin[n]))) blank[n] = 1;
      else
        {
        (line[n])++;
        if (bf[n][BFSZ-4])
          {
          fprintf(stderr, "ERROR: line %i too long in file %s\n", \
                                            line[n], &infnm[n][0]); return -1;
          }
        p1 = &bf[n][0]; while ( *p1 && ('\r' != *p1) && ('\n' != *p1)) {p1++; }
        *p1 = 0;
        }
      }
    }
  for (n = 0; n < manyin; n++) { if ( ! blank[n]) break; }
  if (n >= manyin) break;
  for (n = 0; n < manyin; n++)
    {
    latch = 0;
    if (blank[n])
      {
      if ( ! kolout) fprintf(pfout,"        ");
      else           fprintf(pfout,",       ");
      latch++; kolout++;
      fprintf(pfout, ",          ");
      latch++; kolout++;
      continue;
      }
    p1 = &bf[n][0]; kolumn = 0;
    while (*p1)
      {
      while (isspace((int) *p1 )) { p1++; }
      if ( ! *p1) break;
      p2 = p1;

      while (*p2 && ( ! isspace((int)*p2))) { p2++; }
      c = *p2; *p2 = 0;
/*-----------------------------------------------------------------------------
CAPTURE THE COLUMN HEADINGS APPEARING ON THE FIRST LINE OF THE INPUT FILES, ...
-----------------------------------------------------------------------------*/
      if (1 == line[n])
        {
        strcpy(&head[n][manykolumn[n]][0], p1); (manykolumn[n])++;
        if ( ! strcmp(&want[n][0], p1)) wantkolumn[n] = kolumn;
        }
/*-----------------------------------------------------------------------------
... AND CAPTURE THE DATA FROM THE REMAINING LINES.
-----------------------------------------------------------------------------*/
      else
        {
        char *pend;

        if (2 > line[n]) MESS;
        if (MANYCOLUMN <= kolumn) MESS;
        if (manykolumn[n] <= kolumn) MESS;
        if (0 > wantkolumn[n])
          {
          fprintf(stderr, "ERROR: heading not found: %s\n", &want[n][0]);
          return -1;
          }
        data[n][kolumn] = strtod(p1, &pend);
        if ((ERANGE == errno) || ((0. == data[n][kolumn]) && (p1 == pend)))
          {
          fprintf(stderr, "ERROR: cannot understand data at column %i, " \
             "line %i in file %s\n", kolumn, line[n], &infnm[n][0]); return -1;
          }
        if ( ! kolumn)
          {
          if ( ! kolout) fprintf(pfout, " %7.0f", data[0][0]);
          else           fprintf(pfout, ",%7.0f", data[n][0]);
          latch++; kolout++;
          }
        if (wantkolumn[n] == kolumn)
          {
          fprintf(pfout, ",%10.2e", data[n][kolumn]);
          latch++; kolout++;
          }
        }
      *p2 = c; p1 = p2; if (*p1) p1++;
/*---------------------------------------------------------------------------*/
      kolumn++;
      }
    if ((1 != line[n]) && (2 != latch))
      {
      fprintf(stderr, "ERROR: insufficient data at line %i in file %s\n", \
                                             line[n], &infnm[n][0]); return -1;
      }
    }
  if (1 == line[0])
    {
    for (n = 1; n < manyin; n++) { if (1 != line[n]) break; }
    if (n < manyin)
      {
      fprintf(stderr, "ERROR: at least one input file has no first line\n");
      return -1;
      }
    }
  else fprintf(pfout, "\n");
  lineout++;
  }
/*-----------------------------------------------------------------------------
WRITE THE OUTPUT FILE  ./gnuplot.cmd
-----------------------------------------------------------------------------*/
snprintf(&natname[0][0], BFSZHDR-2, "%s", "BaseCase");
snprintf(&natname[0][0], BFSZHDR-2, "%s", "BaseCase");
for (n = 0; n < manyin; n++)
  {
  p1 = &infnm[n][0];
  p2 = p1;
  while (*p2) { if ('/' == *p2) p1 = p2; p2++; }
  if ('/' == *p1) p1++;
  sprintf(&natname[n][0], "%s", p1);
  }
if ( ! (pfcmd = fopen(&cmdfnm[0], "w")))
  {
  fprintf(stderr, "ERROR: cannot open output file: %s\n", &cmdfnm[0]);
  return -1;
  }
fprintf(pfcmd,
    "#!/usr/bin/gnuplot\n"
    "#\n"
    "#\n"
    "# gnuplot.cmd\n"
    "#\n"
    "#\n"
    "# WRITTEN BY: %s\n"
    "#-------------------------------------------------------\n", argv[0]);

if (1)
  {
  title[0] = 0;
  sprintf(&title[0], "RUN  %s", &datetimestr[0]);

#if defined(TITLE)
  for (n = 1; n < manyin; n++)
    {
    if (strcmp(&infnm[0][0], &infnm[n][0]))
      {
      strcpy(&title[0], &datetimestr[0]); break;
      }
    }
  if ( ! title[0])
    {
    p1 = &infnm[0][0];
    p2 = p1; while (*p2) { if ('/' == *p2) p1 = p2; p2++; }
    p2 = &title[0]; if ( '/' == *p1) p1++;
    strcpy(&title[0], p1);
    }
#endif /*TITLE*/
  fprintf(pfcmd,
    "set title \"%s\"\n", &title[0] );
  fprintf(pfcmd,
    "set xlabel \"%s\"\n", "DAYS" );
  fprintf(pfcmd,
    "set key tmargin left autotitle  columnheader nobox\n"
    "set style fill solid 0.2\n"
    "set terminal svg enhanced background rgb 'white' \\\n"
         "dashed font \"/usr/share/fonts/truetype/liberation/"
         "LiberationSans-Regular.ttf,12\"\n"
    "set palette gray\n");

/*WAS:
    "set terminal svg dashed font \\\n"
*/

  ylabel[0] = 0;
  for (n = 0; n < manyin; n++)
    {
    int jack;

    jack = 0;
    for (m = 0; m < n; n++)
      {
      if (strcmp(&want[m][0], &want[n][0])) { jack = 1; break; }
      }
    if ( ! jack)
      {
      if (n) strcat(&ylabel[0], ", ");
      if ((ORDINATE_MAX - 8) > (strlen(&ylabel[0]) + strlen(&want[n][0])))
        {
        strcat(&ylabel[0], &want[n][0]);
        }
      else { strcat(&ylabel[0], "..."); break; }
      }
    }
  fprintf(pfcmd,
    "set ylabel \"%s\"\n", &ylabel[0]);

  fprintf(pfcmd,
    "set output \"%s.svg\"\n"
    "plot \'./gracov.out\' \\\n", &want[0][0]);
  for (n = 0; n < manyin; n++)
    {
    if (n) fprintf(pfcmd, "'' ");
    else   fprintf(pfcmd, "   ");
    fprintf(pfcmd, "  using %i:%i title \"%s\" with points",
                              (2*n)+1, (2*n)+2, &natname[n][0]);
    if ( (manyin - 1) != n ) fprintf(pfcmd, ", \\");
    fprintf(pfcmd, "\n");
    }
  }
fprintf(pfcmd, "quit\n");
if ( 0 != fclose(pfcmd) )
  {
  fprintf(stderr, "ERROR: cannot close file %s\n", &cmdfnm[0]); return -1;
  }
isbad = 0;
for (n = 0; n < manyin; n++)
  {
  if (ferror(pfin[n]))
    {
    fprintf(stderr, "ERROR: cannot  read input file: %s\n", &infnm[n][0]);
    isbad++;
    }
  if (0 != fclose(pfin[n]))
    {
    fprintf(stderr, "ERROR: cannot close file %s\n", &infnm[n][0]); isbad++;
    }
  }
if (isbad) return -1;

if ( 0 != fclose(pfout) )
  {
  fprintf(stderr, "ERROR: cannot close file %s\n", &outfnm[0]); return -1;
  }

/*----------------------------------------------------------------------------
IF POSSIBLE, PRESENT THE IMAGE FOR VIEWING.  THIS LINE SHOULD BE COMMENTED
OUT AND THE PROGRAM RE-BUILT IF  gnuplot  OR  ristretto  IS UNAVAILABLE.
----------------------------------------------------------------------------*/
if (0 == system("gnuplot gnuplot.cmd")) system ("ristretto Infected.svg"); 

return rc;
}
/* ***************************************************************************/
