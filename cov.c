/* ************************************************************//** @file cov.c
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
_______________________________________________________________________________
@brief Main program: cov.  Project COV-SWN.
*//* *************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <math.h>
#include <assert.h>
#include "swn.h"

/** Help message in response to command-line input mistakes */
#define USAGE do { \
   fprintf(stderr, \
   "USAGE: ./cov  seedcov  seedswn  manynode  halfdegree  beta\n" \
   "              chance  inert  incubating  recovery  [output_directory]\n" \
   "       where  0  <  halfdegree,\n" \
   "              (1 + 2*halfdegree) <= manynode,\n" \
   "              0. <= beta   <  1.,\n" \
   "              0. <= chance <= 1.,\n" \
   "              0. <= inert  <= 1.,\n" \
   "              0  <= incubating < recovery\n"); \
   } while(0)

/** Information residing on a single node */
struct status
  {
  struct node *pnode;   /**< Pointer to the connectivity information provided
                                                    by the small-world model */
  int   day;            /**< Count of days since infection                   */
  int   inert;          /**< Flag set if this node is inert                  */
  int   shuffle;        /**< Position of this node in the queue for updating */
  };

extern int swn(unsigned int,int,int,double,struct node **);

/* ************************************************************************//**
@brief  Run the epidemic.
        USAGE: ./cov  seedcov  seedswn  manynode  halfdegree  beta \
                      chance  inert  incubating  recovery [output_directory]
*//* *************************************************************************/
int
 main(int argc, char *argv[])
{
FILE *pfout;
char outfnm[2048];
char outdir[1024], *p1;
struct node **pp;
struct status *pother;
double dbeta, dchance, dinert;
unsigned int seed, seedcov, seedswn;
int manynode, halfdegree;
int manycase, manycasewas;
int manyedge;
int chance, inert;
int incubating, recovery;
struct status *pstatus0;
struct node *pnode0;
int i, j, m, tick, day;
int rc;

rc = 0;
/*-----------------------------------------------------------------------------
PARSE THE COMMAND LINE
-----------------------------------------------------------------------------*/
if ((10 > argc) || (11 < argc))
  {
  fprintf(stderr, "ERROR: expected 9 or 10 argments, got %i:\n", argc-1);
  for (m = 1; m < argc; m++) { fprintf(stderr, "      >%s<\n", argv[m]); }
  USAGE; return -1;
  }
errno = 0; seedcov = (int)strtol(argv[1], NULL, 16);
if (errno) { fprintf(stderr, "ERROR: bad seedcov\n"); USAGE; return -1; }
errno = 0; seedswn = (int)strtol(argv[2], NULL, 16);
if (errno) { fprintf(stderr, "ERROR: bad seedswn\n"); USAGE; return -1; }
errno = 0; manynode = (int)strtol(argv[3], NULL, 10);
if (errno) { fprintf(stderr, "ERROR: bad manynode\n"); USAGE; return -1; }
if ( ! manynode) { fprintf(stderr, "ERROR: no nodes\n"); USAGE; return -1; }
errno = 0; halfdegree = (int)strtol(argv[4], NULL, 10);
if (errno) { fprintf(stderr, "ERROR: bad halfdegree\n"); USAGE; return -1; }
if (manynode < (2 * halfdegree))
  {
  fprintf(stderr, "ERROR: too few nodes: must exceed %i\n", \
                                            2 * halfdegree); USAGE; return -1;
  }
errno = 0; dbeta = strtod(argv[5], NULL);
if (errno) { fprintf(stderr, "ERROR: bad beta\n"); USAGE; return -1; }
errno = 0; dchance = strtod(argv[6], NULL);
if (errno) { fprintf(stderr, "ERROR: bad chance\n"); USAGE; return -1; }
chance = (int)(nearbyint(1024. * dchance)); 
if ((0 > chance) || (1024 < chance)) \
         { fprintf(stderr, "ERROR: chance out of range\n"); USAGE; return -1; }
errno = 0; dinert = strtod(argv[7], NULL);
if (errno) { fprintf(stderr, "ERROR: bad inert\n"); USAGE; return -1; }
inert = (int)(nearbyint(1024. * dinert)); 
errno = 0; incubating = (int)strtol(argv[8], NULL, 10);
if (errno) { fprintf(stderr, "ERROR: bad incubating\n"); USAGE; return -1; }
errno = 0; recovery  = (int)strtol(argv[9], NULL, 10);
if (errno) { fprintf(stderr, "ERROR: bad recovery\n"); USAGE; return -1; }
if (incubating >= recovery)
  {
  fprintf(stderr, "ERROR: incubating must precede recovery\n");
  USAGE; return -1;
  }
if (11 == argc)
  {
  strncpy(outdir, argv[10], 1020); outdir[1020] = 0;
  p1 = outdir; while (*p1) { p1++; }
  if (outdir < p1) { p1--; if ('/' == *p1) *p1++ = 0; }
  }
else strcpy(outdir, "OUT");
/*-----------------------------------------------------------------------------
CONSTRUCT THE NETWORK
-----------------------------------------------------------------------------*/
if (0 > (rc = swn(seedswn, manynode, halfdegree, dbeta, &pnode0)))
  {
  fprintf(stderr, "ERROR: failed to construct the network, retcode %i\n", rc);
  return rc;
  }
/*-----------------------------------------------------------------------------
ALLOCATE MEMORY FOR THE STATUS ARRAY
-----------------------------------------------------------------------------*/
if (NULL == \
       (pstatus0 = (struct status *)malloc(manynode * sizeof(struct status))))
  {
  fprintf(stderr, "ERROR: memory allocation refused\n"); return -8;
  }
for (j = 0; j < manynode; j++)
  {
  (pstatus0 + j)->pnode = pnode0 + j;
  (pnode0 + j)->pvoid = (void *)(pstatus0 + j); 
  }
/*-----------------------------------------------------------------------------
NAME THE OUTPUT FILE AND OPEN IT
-----------------------------------------------------------------------------*/
snprintf(outfnm, 2040, "%s/%08X%08X-%i-%i-%5.3f-%4.2f-%4.2f-%i-%i", \
                            outdir, seedcov, seedswn, manynode, halfdegree, \
                                dbeta, dchance, dinert, incubating, recovery);
if (NULL == (pfout = fopen(outfnm, "w")))
  {
  fprintf(stderr, "ERROR: cannot open output file: %s\n", outfnm); return -16;
  }
/*-----------------------------------------------------------------------------
INITIALISE THE STATUS ARRAY.
PATIENT ZERO MUST HAVE AT LEAST  2*halfdegree  NEIGHBOURS.
-----------------------------------------------------------------------------*/
seed = seedcov;
srand(seed);
manyedge = 0;
while (1)
  {
  m = (rand_r(&seed)) % manynode; assert((0 <= m) && (manynode >m));
  pp = (pstatus0 + m)->pnode->pp0;
  while (NULL != *pp) { manyedge++; pp++; }
  if (manyedge >= (2*halfdegree)) break;
  }
for (j = 0; j < manynode; j++)
  {
  (pstatus0 + j)->day = 0;
  (pstatus0 + j)->inert = 0;
  (pstatus0 + j)->shuffle = j;
  if (m == j) (pstatus0 + j)->day = 1;
  else { if (inert > (rand_r(&seed)) % 1024) (pstatus0 + j)->inert = 1; }
  }
/*---------------------------------------------------------------------------*/
day = 0;
fprintf(pfout, "Day Infected Uninfected Contacts\n");
manycase = 0; manyedge = 0;
for (j = 0; j < manynode; j++)
  {
  if ((pstatus0 + j)->day) manycase++;
  else
    {
    pp = (pstatus0 + j)->pnode->pp0;
    while (NULL != *pp) { manyedge++; pp++; }
    }
  }
fprintf(pfout, "%3i  %7.4f  %7.4f  %7.4f\n", \
              day, ((double)manycase) / ((double)manynode), 
                   ((double)(manynode - manycase)) / ((double)manynode), 
                   ((double)manyedge) / ((double)(2 * manynode * halfdegree)));
/*-----------------------------------------------------------------------------
MAIN LOOP BEGINS
-----------------------------------------------------------------------------*/
manycasewas = 0; tick = 0;
while (365 > day)
  {
/*-----------------------------------------------------------------------------
RANDOMIZE THE ORDER IN WHICH THE NODES WILL BE UPDATED
-----------------------------------------------------------------------------*/
  for (i = manynode - 1; i > 0; i--)
    {
    int swap;

    swap = (pstatus0 + i)->shuffle;
    j = rand_r(&seed) % (i + 1);
    (pstatus0 + i)->shuffle = (pstatus0 + j)->shuffle;
    (pstatus0 + j)->shuffle = swap;
    }
/*-----------------------------------------------------------------------------
FOR EVERY NODE  j  WITH NON-ZERO .day, BUMP .day.
-----------------------------------------------------------------------------*/
  for (i = 0; i < manynode; i++)
    {
    j = (pstatus0 + i)->shuffle;
    if ((pstatus0 + j)->inert) continue;
    if ((pstatus0 + j)->day) ((pstatus0 + j)->day)++;
/*-----------------------------------------------------------------------------
... OTHERWISE SET .day OF NODE  j  TO  1  WITH PROBABILTY  chance  IF ANY
NEIGHBOUR IS IN THE INFECTIOUS WINDOW  incubating < .day < recovery.
THUS  chance  HAS UNITS: PER NEIGHBOUR PER DAY.
-----------------------------------------------------------------------------*/
    else
      {
      pp = (pstatus0 + j)->pnode->pp0;
      while (NULL != *pp)
        {
        pother = (struct status *)((*pp)->pvoid);
        if ((incubating < pother->day) && (recovery > pother->day))
          {
          if (chance > (rand_r(&seed)) % 1024) (pstatus0 + j)->day = 1;
          }
        pp++;
        }
      }
    }
/*-----------------------------------------------------------------------------
DETECT WHETHER ASYMPTOTE HAS BEEN REACHED
-----------------------------------------------------------------------------*/
  if (manycasewas < manycase) { manycasewas = manycase; tick = 0; }
  else tick++;
  day++;
/*-----------------------------------------------------------------------------
UPDATE STATISTICS FOR THIS TIMESTEP
-----------------------------------------------------------------------------*/
  manycase = 0; manyedge = 0;
  for (j = 0; j < manynode; j++)
    {
    if ((pstatus0 + j)->day) manycase++;
    else
      {
      pp = (pstatus0 + j)->pnode->pp0;
      while (NULL != *pp) { manyedge++; pp++; }
      }
    }
  fprintf(pfout, "%3i  %7.4f  %7.4f  %7.4f\n", \
              day, ((double)manycase) / ((double)manynode), 
                   ((double)(manynode - manycase)) / ((double)manynode), 
                   ((double)manyedge) / ((double)(2 * manynode * halfdegree)));
  }
/*-----------------------------------------------------------------------------
MAIN LOOP ENDS.  PRINT OUTPUT FILENAME ON  stdout.
-----------------------------------------------------------------------------*/
fprintf(stdout,"-> %s\n", outfnm);
/*----------------------------------------------------------------------------
CLEAN UP
-----------------------------------------------------------------------------*/
if (pfout) fclose(pfout);
if (0 > swn(0, manynode, 0, 0., &pnode0))
  {
  fprintf(stderr, "WORRY: failed to free some memory allocated by swn()\n");
  }
free(pstatus0);
return 0;
}
/* ***************************************************************************/
