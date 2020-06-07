/* **********************************************************//** @file demo.c
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
@brief Utility for performing multiple runs of the cov program.
*//* *************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>

/** All data contained in a single line of the input file */
struct w
  {
  char     a[9][3][32];                                /**< Character fields */
  char     outdir[1024];                               /**< Output directory */
  int      j[9][3];                                    /**< Integer data     */
  double   d[9][3];                                    /**< Float data       */
  } w;            /**< All data contained in a single line of the input file */

/* ************************************************************************//**
@brief Main program: demo

USAGE: ./demo full_path_to_agenda_file
*//* *************************************************************************/
int
 main(int argc, char *argv[])
{
FILE *pFILE;
struct stat stat0;
char infnm[1024], cmd[4096], bf[1024], *p1, *p2;
size_t sz;
int jack, line, kase, katch, latch[3], n, rc;
int manynode, halfdegree, incubation, recovery;
double beta, chance, inert;

katch = 0; if (2 != argc) katch = 1;
if ( ! katch) { if (NULL == argv[1]) katch = 1; }
if ( ! katch) { if (stat(argv[1], &stat0)) katch = 1; }
if (katch)
  {
  fprintf(stderr, "USAGE: ./demo full_path_to_agenda_file\n"); return -1;
  }
katch = 0;
if ('/' != *(argv[1]))
  {
  if (NULL == getcwd(infnm, 1024 - 2)) katch = 1;
  if ( ! katch)
    {
    if (1024 <= strlen(infnm) + strlen(argv[1])) katch = 1;
    }
  if (katch)
    {
    fprintf(stderr, "ERROR: bad input filename, or path name too long\n");
    return -1;
    }
  p1 = argv[1]; p2 = p1; while (*p2) { if ('/' == *p2) p1 = p2; p2++; }
  if ('/' == *p1) p1++;
  strcat(infnm, "/"); strcat(infnm, p1);
  }
else strcpy(infnm, argv[1]);
errno = 0;
if (NULL == (pFILE = fopen(infnm, "r")))
  {
  fprintf(stderr, "ERROR: cannot open input file: %s\n", infnm);
  perror(NULL);  return -1;
  }
line = 0;
while (NULL != fgets(bf, 1020, pFILE))
  {
  line++;
  p1 = bf; jack = 1;
  while (*p1)
    {
    if (('\n' == *p1) || ('\r' == *p1)) { *p1 = 0; break; }
    if ( ! isspace(*p1)) jack = 0;
    p1++;
    }
  if (jack) continue;
  while ((bf < p1) && (isspace(*(p1 - 1)))) { p1--; } *p1 = 0;
  memset(&w, 0, sizeof(struct w));
  p1 = bf; kase = 0; katch = 0; jack = 0;
  while (*p1)
    {
    while (isspace(*p1)) { p1++; }
    if (( ! *p1) || ('#' == *p1)) { if (10 > kase) jack = 1; break; }
    p2 = p1; latch[0] = -1; latch[1] = -1;
    while (*p2 && ( ! isspace(*p2)))
      {
      if (':' == *p2)
        {
        if      (0 > latch[0]) latch[0] = (int)(p2 - p1);
        else if (0 > latch[1]) latch[1] = (int)(p2 - p1);
        else    { fprintf(stderr, "ERROR: too many colons\n"); katch = 1; }
        }
      p2++;
      }
    if ((0<=latch[0]) && ((0 == kase) || (1 == kase) || (9 == kase)))
      {
      fprintf(stderr, "ERROR: unexpected colons\n"); katch = 1;
      }
    if ((0>latch[0])  &&  (0 != kase) && (1 != kase) && (9 != kase))   
      {
      fprintf(stderr, "ERROR: missing colons\n"); katch = 1;
      }
    if ((0<=latch[0]) && (0>latch[1]))
      {
      fprintf(stderr, "ERROR: unpaired colon\n"); katch = 1;
      }
    if (katch)
      {
      fprintf(stderr, "ERROR: at line %i, badly formatted field %i\n", \
                                                                 line, kase+1);
      *p2 = 0; fprintf(stderr, "       >%s<\n", p1); return -1;
      }
    latch[2] = p2 - p1;
    if      ((0 >= kase) || (1 == kase))
      {
      sz = p2 - p1; if (30 < sz) katch = 1;
      if (katch)
        {
        fprintf(stderr, "ERROR: at line %i, badly formatted field %i\n", \
                                                                 line, kase+1);
        *p2 = 0; fprintf(stderr, "       >%s<\n", p1); return -1;
        return -1;
        }
      memcpy(w.a[kase][0], p1, sz);
      }
    else if (2 <= kase && (kase <= 8))
      {
      sz = latch[0];
      if (30 > sz) memcpy(w.a[kase][0], p1, sz); else katch = 1;
      sz = latch[1]-latch[0]-1;
      if (30 > sz) memcpy(w.a[kase][1], (p1+latch[0]+1),sz); else katch = 1;
      sz = latch[2]-latch[1]-1;
      if (30 > sz) memcpy(w.a[kase][2], (p1+latch[1]+1),sz); else katch = 1;
      if (katch)
        {
        fprintf(stderr, "ERROR: at line %i, excessively long field %i\n", \
                                                                 line, kase+1);
        *p2 = 0; fprintf(stderr, "       >%s<\n", p1); return -1;
        }
      if ((2 == kase) || (3 == kase) || (7 == kase) || (8 == kase))
        {
        for (n = 0; n < 3; n++)
          {
          errno = 0; w.j[kase][n] = (int)strtol(w.a[kase][n], NULL, 10);
          if (errno)
            {
            fprintf(stderr, \
                      "ERROR: at line %i, unconvertible integer field %i\n", \
                                                                 line, kase+1);
            fprintf(stderr, "       >%s<\n", w.a[kase][n]); return -1;
            }
          if (0 > w.j[kase][n])
            {
            fprintf(stderr, "ERROR: at line %i, negative integer field %i\n", \
                                                                 line, kase+1);
            fprintf(stderr, "       >%s<\n", w.a[kase][n]); return -1;
            }
          }
        if (w.j[kase][0] > w.j[kase][2])
          {
          fprintf(stderr, "ERROR: at line %i, min > max in field %i\n", \
                                                                 line, kase+1);
          fprintf(stderr, "       >%s<\n", w.a[kase][0]); return -1;
          }
        if ( ! w.j[kase][1]) w.j[kase][1] = 100000000;
        }
      else
        {
        for (n = 0; n < 3; n++)
          {
          errno = 0; w.d[kase][n] = strtod(w.a[kase][n], NULL);
          if (errno)
            {
            fprintf(stderr, \
                       "ERROR: at line %i, unconvertible float field %i\n", \
                                                                 line, kase+1);
            fprintf(stderr, "       >%s<\n", w.a[kase][n]); return -1;
            }
          if (0. > w.d[kase][n])
            {
            fprintf(stderr, "ERROR: at line %i, negative integer field %i\n", \
                                                                 line, kase+1);
            fprintf(stderr, "       >%s<\n", w.a[kase][n]); return -1;
            }
          }
        if (w.d[kase][0] > w.d[kase][2])
          {
          fprintf(stderr, "ERROR: at line %i, min > max in field %i\n", \
                                                                 line, kase+1);
          fprintf(stderr, "       >%s<\n", w.a[kase][0]); return -1;
          }
        if (0.0000001 > fabs(w.d[kase][1])) w.d[kase][1] = 1.e8;
        }
      }
    else if (9 == kase)
      {
      if ('/' != *p1)
        {
        if (NULL == getcwd(w.outdir, 1024 - 2)) katch = 1;
        if ( ! katch) { if (1024 <= strlen(w.outdir) + strlen(p1)) katch = 1; }
        if (katch)
          {
          fprintf(stderr, "ERROR: at line %i, output path name too long\n", \
                                                             line); return -1;
          }
        p2 = p1; while (*p2) { if ('/' == *p2) p1 = p2; p2++; }
        if ('/' == *p1) p1++;
        strcat(w.outdir, "/"); strcat(w.outdir, p1);
        }
      else strcpy(w.outdir, p1);
      if ( ! katch)
        {
        errno = 0;
        if (stat(w.outdir, &stat0))
          {
          fprintf(stderr, "ERROR: at line %i, missing output directory: %s\n",\
                                                    line, w.outdir); katch = 1;
          }
        }
      if ( ! katch)
        {
        if (S_IFDIR != (stat0.st_mode & S_IFMT))
          {
          fprintf(stderr, "ERROR: at line %i, not a directory: %s\n", \
                                                    line, w.outdir); katch = 1;
          }
        }
      if (katch)
        {
        fprintf(stderr, "ERROR: at line %i, bad field %i\n", line, kase+1);
        return -1;
        }
      }
    else { fprintf(stderr, "BUG: %i=kase\n",kase); return -1; }
    kase++;
    p1 = p2;
    }
  if (jack) continue;
  if (10 < kase)
    {
    fprintf(stderr, "ERROR: line %i has too many fields: ignored\n",line);
    continue;
    }
  if (10 > kase)
    {
    fprintf(stderr, "ERROR: line %i has too few fields: ignored\n",line);
    continue;
    }
/*-----------------------------------------------------------------------------
RUN THE PROGRAM REPEATEDLY
-----------------------------------------------------------------------------*/
  for (manynode =  w.j[2][0]; \
       manynode <= w.j[2][2]; \
       manynode += w.j[2][1])
    {
    for (halfdegree =  w.j[3][0]; \
         halfdegree <= w.j[3][2]; \
         halfdegree += w.j[3][1])
      {
      for (beta =  w.d[4][0]; \
           beta <= w.d[4][2]; \
           beta += w.d[4][1])
        {
        for (chance =  w.d[5][0]; \
             chance <= w.d[5][2]; \
             chance += w.d[5][1])
          {
          for (inert =  w.d[6][0]; \
               inert <= w.d[6][2]; \
               inert += w.d[6][1])
            {
            for (incubation =  w.j[7][0]; \
                 incubation <= w.j[7][2]; \
                 incubation += w.j[7][1])
              {
              for (recovery =  w.j[8][0]; \
                   recovery <= w.j[8][2]; \
                   recovery += w.j[8][1])
                {
                p1 = w.a[0][0];
                if (( ! memcmp("0x",p1,2)) || ( ! memcmp("0X",p1,2))) p1 += 2;
                p2 = w.a[1][0];
                if (( ! memcmp("0x",p2,2)) || ( ! memcmp("0X",p2,2))) p2 += 2;
                fprintf(stdout,"   %s/%s%s-%i-%i-%5.3f-%4.2f-%4.2f-%i-%i\n", \
                                w.outdir, p1, p2, \
                                manynode, halfdegree, beta, \
                                chance, inert, incubation, recovery);
                snprintf(cmd, 4090, \
                         "./cov %s %s %i %i %5.3f %4.2f %4.2f %i %i %s", \
                                w.a[0][0], w.a[1][0], \
                                manynode, halfdegree, beta, \
                                chance, inert, incubation, recovery, \
                                w.outdir);
                if (0 != (rc = system(cmd)))
                  {
                  fprintf(stderr, "ERROR: system() returned %i\n",rc);
                  fprintf(stderr, "       Command was:\n%s\n", cmd);
                  }
                }
              }
            }
          }
        }
      }
    }
/*---------------------------------------------------------------------------*/
  }
if (ferror(pFILE)) { fprintf(stderr, "ERROR: failed to read entire file\n"); }
if (pFILE) fclose(pFILE); 
return 0;
}
