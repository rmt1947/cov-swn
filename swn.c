/* ************************************************************//** @file swn.c
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
@brief Functions implementing the Watts-Strogatz Small World Network.       \n
References:                                                                 \n
[1] D.J. Watts & S.H. Strogatz 1998 Nature, 393, 440-442.                   \n
[2] A. Barrat & M. Weigt 1999 arXiv:cond-mat/9903411v2.
*//* *************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <math.h>
#include <assert.h>
#include "swn.h"

int BarratWeigt(int,double,int,double *);

/* ************************************************************************//**
@brief   Insert an edge beween two nodes.
@param   parg1    The first  of the pair of nodes to be unlinked.
@param   parg2    The second of the pair of nodes to be unlinked.
@return           0  => nodes successfully linked;
                  1  => nodes were already linked, no action taken;
                  <0 => error.
*//***************************************************************************/
static inline int
 setupedge(struct node *const parg1, struct node *const parg2)
{
int n, m, latch[2];
size_t sz[2];
void *pvoid;
struct node *pnode[2], **pp;

pnode[0] = parg1;
pnode[1] = parg2;

for (n = 0; n < 2; n++)
  {
  m = (n + 1) % 2;
  latch[n] = 0;
  pp = pnode[n]->pp0;
  while (NULL != *pp)
    {
    if (*pp == pnode[m]) latch[n] = 1;
    pp++;
    }
  sz[n] = (size_t)(pp - (pnode[n]->pp0));
  }
if (latch[0] || latch[1])
  {
  if (latch[0] && latch[1]) return 1; else assert(0);
  }
assert( ! latch[0]);
assert( ! latch[1]);
for (n = 0; n < 2; n++)
  {
  m = (n + 1) % 2;
  if (NULL == (pvoid = realloc((void *)(pnode[n]->pp0), \
            ((sz[n] + 2) * sizeof(struct node *))))) \
             { fprintf(stderr, "ERROR: memory request refused\n"); return -8; }
  pnode[n]->pp0 = (struct node **)pvoid;
  assert(NULL == *(pnode[n]->pp0 + sz[n]));
  *(pnode[n]->pp0 + sz[n]) = pnode[m];
  *(pnode[n]->pp0 + sz[n] + 1) = NULL;
  }
return 0;
}
/* ************************************************************************//**
@brief   Destroy the edge beween two nodes.
@param   parg1    The first  of the pair of nodes to be unlinked.
@param   parg2    The second of the pair of nodes to be unlinked.
@return           0  => nodes successfully unlinked;
                  1  => nodes were already unlinked, no action taken;
                  -1 => error.
*//***************************************************************************/
static inline int
 breakedge(struct node *const parg1, struct node *const parg2)
{
int n, m;
size_t sz[2];
void *pvoid;
struct node *pnode[2], **pp[2];

pnode[0] = parg1;
pnode[1] = parg2;

for (n = 0; n < 2; n++)
  {
  m = (n + 1) % 2;
  pp[n] = pnode[n]->pp0;
  while (NULL != *(pp[n])) { if (*(pp[n]) == pnode[m]) break; (pp[n])++; }
  }
if ((NULL == *pp[0]) || (NULL == *pp[1]))
  {
  if ((NULL == *pp[0]) && (NULL == *pp[1])) return 1; else assert(0);
  }
assert(*pp[0] == pnode[1]);
assert(*pp[1] == pnode[0]);
for (n = 0; n < 2; n++)
  {
  while (NULL != *(pp[n] + 1)) { *(pp[n]) = *(pp[n] + 1); (pp[n])++; }
  *(pp[n])++ = NULL;
  sz[n] = (size_t)(pp[n] - (pnode[n]->pp0));
  assert(sz[n]);
  if (NULL == (pvoid = \
    realloc((void *)(pnode[n]->pp0), \
            (sz[n] * sizeof(struct node *))))) \
            { fprintf(stderr, "ERROR: memory request refused\n"); return -8; }
  pnode[n]->pp0 = (struct node **)pvoid;
  if (1 == sz[n]) assert(NULL == *(pnode[n]->pp0));
  }
return 0;
}
/* ************************************************************************//**
@param   swnseed     Seed for srand().        
@param   manynode    The number of nodes,
@param   halfdegree  Half the degree of nodes in the first-stage ring.
@param   dbeta       The rewiring fraction.
@param   ppnode0     Location to receive the base of the array of nodes.
@return              Zero unless error.
@note                To free memory, call with the extant manynode and ppnode0,
                     but with halfdegree=0.  On return *ppnode0 will be NULL.
*//* *************************************************************************/
int
 swn(unsigned int swnseed, int manynode, int halfdegree, double dbeta, \
                                                         struct node **ppnode0)
{
struct node *pnode0, **pp;
int j, m, n, lap, other;
int manytub, *ptub0;
unsigned int seed;
int beta;
FILE *pftubs;
int chktubs;
double bw, chkbw, tail;
int rc;

rc = 0;
/*-----------------------------------------------------------------------------
FREE MEMORY
-----------------------------------------------------------------------------*/
if ( ! halfdegree)
  {
  if (NULL != (pnode0 = *ppnode0))
    {
    for (j = 0; j < manynode; j++) { free((pnode0 + j)->pp0); }
    free(pnode0); *ppnode0 = NULL;
    }
  return 0;
  }
/*-----------------------------------------------------------------------------
VALIDATE ARGUMENTS
-----------------------------------------------------------------------------*/
if (manynode <= (2 * halfdegree))
  {
  fprintf(stderr, "ERROR: too few nodes: must exceed %i\n", \
                                                    2 * halfdegree); return -1;
  }
beta = (int)(nearbyint(1024. * dbeta));
if ((0 > beta) || (1024 < beta)) \
                  { fprintf(stderr, "ERROR: beta out of range\n"); return -1; }
/*-----------------------------------------------------------------------------
ALLOCATE MEMORY
-----------------------------------------------------------------------------*/
if (NULL == (pnode0 = (struct node *)calloc(manynode, sizeof(struct node))))
  {
  fprintf(stderr, "ERROR: memory request refused\n"); return -8;
  }
*ppnode0 = pnode0;
for (j = 0; j < manynode; j++)
  {
  if (NULL == ((pnode0 + j)->pp0 = \
                             (struct node **)malloc(sizeof(struct node *))))
    {
    fprintf(stderr, "ERROR: memory request refused\n"); return -8;
    }
  *((pnode0 + j)->pp0) = NULL;
  }
manytub = 6 * halfdegree;
if (NULL == (ptub0 = (int *)malloc(manytub * sizeof(int))))
  {
  fprintf(stderr, "ERROR: memory request refused\n"); return -8;
  }
/*-----------------------------------------------------------------------------
CONSTRUCT THE RING LATTICE.

LINKS TO NEIGHBOURS ARE STORED AT (.pp0 + i), WHERE 0 <= i < (2*halfdegree).
THE LINKS 0 <= i < halfdegree WILL NEVER BE ENTIRELY REMOVED, BUT MAY BE
REWIRED IN A SUBSEQUENT STEP.
THE LINKS halfdegree <= i < (2*halfdegree) MAY BE REMOVED AS THE RESULT OF A
REWIRING STEP IMPLEMENTED AT REMOTE NODES.
-----------------------------------------------------------------------------*/
for (j = 0; j < manynode; j++)
  {
  for (n = 0; n < halfdegree; n++)
    {
    m = j + (n + 1);
    if (manynode <= m) m -= manynode;
    else if    (0 > m) m += manynode;
    if (0 > setupedge((pnode0 + j), (pnode0 + m))) return -2;
    }
  for (n = 0; n < halfdegree; n++)
    {
    m = j - (n + 1);
    if (manynode <= m) m -= manynode;
    else if    (0 > m) m += manynode;
    if (0 > setupedge((pnode0 + j), (pnode0 + m))) return -2;
    }
  }
/*-----------------------------------------------------------------------------
REWIRE.

ON EACH LAP, EACH NODE  j  ON THE LATTICE IS VISITED PRECISELY ONCE.
ON LAP  l  ITS LINK TO NODE  j+l  IS REWIRED WITH PROBABILITY  beta  TO
A RANDOMLY CHOSEN OTHER NODE, SUBJECT TO THIS CHOICE CREATING NO DUPLICATE
LINKS OR SELF-LINKS (A.K.A. LOOPS).
-----------------------------------------------------------------------------*/
seed = swnseed;
srand(seed);
for (lap = 0; lap < halfdegree; lap++)
  {
  for (j = 0; j < manynode; j++)
    {
    if (beta <= (rand_r(&seed)) % 1024) continue;
    m = j + (lap + 1);
    if (manynode <= m) m -= manynode;
    else if    (0 > m) m += manynode;
/*-----------------------------------------------------------------------------
CHOOSE ANOTHER NODE TO LINK TO, REJECTING DUPLICATES AND LOOPS
-----------------------------------------------------------------------------*/
    while (1)
      {
      if ((other = (rand_r(&seed)) % manynode) == j) continue;
      pp = (pnode0 + other)->pp0;
      while (NULL != *pp) { if ((pnode0 + j) == *pp) break; pp++; }
      if (NULL == *pp) break;
      }
    if (0 > breakedge((pnode0 + j), (pnode0 + m))) return -3;
    if (0 > (rc = setupedge((pnode0 + j), (pnode0 + other)))) return -4;
    if (0 < rc) return -5;
    }
  }
/*-----------------------------------------------------------------------------
GET THE DEGREE DISTRIBUTION
-----------------------------------------------------------------------------*/
for (n = 0; n < manytub; n++) { *(ptub0 + n) = 0; }
for (j = 0, n = 0; j < manynode; j++)
  {
  pp = (pnode0 + j)->pp0;
  while (NULL != *pp) { pp++; }
  n = pp - ((pnode0 + j)->pp0);
  if (manytub <= n) n = manytub - 1;
  (*(ptub0 + n))++;
  }
for (n = 0, m = 0; n < manytub; n++) { m += *(ptub0 + n); }
if (manynode != m) return -6;

if (NULL == (pftubs = fopen("tubs.txt", "w")))
  {
  fprintf(stderr, "WORRY: cannot open output file: %s\n", "tubs.txt");
  }
else
  {
  fprintf(pftubs, "%7i=manynode, %i=halfdegree, %5.3f=beta\n\n", \
                                                  manynode, halfdegree, dbeta);
  fprintf(pftubs, "  Degree   Node count   Fraction     Ref.[2]\n");
  chktubs = 0; chkbw = 0.;
  for (n = 0; n < manytub - 1; n++)
    {
    if (0 > BarratWeigt(halfdegree, dbeta, n, &bw)) {;}

    fprintf(pftubs,"  %4i       %7i    %8.6f    %8.6f\n", \
             n, *(ptub0 + n), ((double)(*(ptub0 + n)))/((double)manynode), bw);
    chktubs += *(ptub0 + n);
    chkbw += bw;
    }
  tail = 0.; m = n;
  while (1.e-8 < fabs(bw))
    {
    if (0 > BarratWeigt(halfdegree, dbeta, m, &bw)) {;}
    tail += bw; m++;
    }
  assert(n == manytub - 1);
  chktubs += *(ptub0 + n);
  fprintf(pftubs,">=%4i       %7i    %8.6f    %8.6f\n", \
           n, *(ptub0 + n), ((double)(*(ptub0 + n)))/((double)manynode), tail);
  chkbw += tail;
  fclose(pftubs);
  assert(chktubs == manynode);
  assert(1.e-7 > fabs(1. - chkbw));
  }
if (ptub0) free(ptub0);
return 0;
}
/* ************************************************************************//**
@brief  Probability distribution of degree.

@note   See: A.Barrat & M.Weigt 1999 "On the properties of small-world network
        models", arXiv:cond-mat/9903411v2

@param  k         Halfdegree.
@param  c         Connectivity.
@param  beta      Rewiring parameter.
@param  presult   Location to receive the probability.
*//* *************************************************************************/
int
 BarratWeigt(int k, double beta, int c, double *presult)
{
double sum, betacomp, t1, t2, t3, t4, t5, t6;
int j, l, m, n, il, im;

if (c < k) { *presult = 0.; return 0; }
sum = 0.; betacomp = 1. - beta;
for (n = 0; (n <= k) && (n <= (c - k)); n++)
  {
  m = 1; l = 1;
  im = k; il = l;
  for (j = 0; j < n; j++)
    {
    m = m * im; im--;
    l = l * il; il++;
    }

  assert( ! (m % l));
  t1 = (double)(m / l);

  t2 = 1.; for (j = 0; j < n; j++)       { t2 = t2 * betacomp; }

  assert(0 <= (k - n));
  t3 = 1.; for (j = 0; j < (k - n); j++) { t3 = t3 * beta; }

  assert(0 <= (c - k - n));
  t4 = 1.; for (j = 0; j < (c - k - n); j++) { t4 = t4 * ( k * beta); }
  t5 = 1.; for (j = c - k - n; 0 < j; j--) { t5 = t5 * j; }

  t6 = exp( - (beta * k));

  sum += (t1 * t2 * t3 * t4 * t6) / t5;
  }
*presult = sum;
return 0;
}
/* ***************************************************************************/
