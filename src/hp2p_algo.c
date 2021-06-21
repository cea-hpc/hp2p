// Copyright (C) 2010-2016 CEA/DAM
// Copyright (C) 2010-2016 Laurent Nguyen <laurent.nguyen@cea.fr>
//
// This file is part of HP2P.
//
// This software is governed by the CeCILL-C license under French law and
// abiding by the rules of distribution of free software.  You can  use,
// modify and/ or redistribute the software under the terms of the CeCILL-C
// license as circulated by CEA, CNRS and INRIA at the following URL
// "http://www.cecill.info".

/**
 * \file      hp2p_algo.c
 * \author    Laurent Nguyen <laurent.nguyen@cea.fr>
 * \version   4.0
 * \date      21 June 2021
 * \brief     HP2P Benchmark
 *
 * \details   No
 */

#include "hp2p.h"

char *hp2p_algo_name[] = {
  "RANDOM",
  "SHIFT",
  NULL
};


/**
 * \fn     void hp2p_algo_get_num
 * \brief  load get number of algorithm
 *
 * \param  void
 * \return int
 **/
int hp2p_algo_get_num()
{
  int i = 0;
  char *p = NULL;
  p = hp2p_algo_name[0];
  while(p != NULL)
    {
      i++;
      p++;
    }
  return i;
}
/**
 * \fn     void hp2p_algo_get_name
 * \brief  get algo name
 *
 * \param  void
 * \return char *
 **/
char* hp2p_algo_get_name(int algo)
{
  char *tmp = NULL;
  int len = 0;

  len = strlen(hp2p_algo_name[algo]);

  tmp = (char *) malloc(len*sizeof(char)+1);
  strcpy(tmp, hp2p_algo_name[algo]);
  return tmp;
}
/**
 * \fn      void build_couples(int *v, int size, build_method algo)
 * \brief   Build pairs
 *          of workers.
 *
 * \param v Array of int
 * \param size Size of array
 * \param algo to build couples
 **/
void hp2p_algo_build_couples(int *v, int size, int algo)
{
  static int init = 0;
  // Generate random couples
  if (algo == 0)
  {
    hp2p_algo_random(v, size);
  }  
  else
  {
    hp2p_algo_mirroring_shift(v, size);
  }
}

void hp2p_algo_mirroring_shift(int *v, int size)
{
  static int init = 0;
  // Couples will be mirroring shift - ly permuted
  for (int i = 0; i < size; i++)
    {
      v[i] = (size + 1 - i + init) % size;
    }
  init++;
}
