// Copyright (C) 2010-2021 CEA/DAM
// Copyright (C) 2010-2021 Laurent Nguyen <laurent.nguyen@cea.fr>
//
// This file is part of HP2P.
//
// This software is governed by the CeCILL-C license under French law and
// abiding by the rules of distribution of free software.  You can  use,
// modify and/ or redistribute the software under the terms of the CeCILL-C
// license as circulated by CEA, CNRS and INRIA at the following URL
// "http://www.cecill.info".

/**
 * \file      hp2p_result.c
 * \author    Laurent Nguyen <laurent.nguyen@cea.fr>
 * \version   4.0
 * \date      21 June 2021
 * \brief     HP2P Benchmark
 *
 * \details   No
 */

#include "hp2p.h"

void hp2p_result_alloc(hp2p_result *result,
		       hp2p_mpi_config *mpi_conf,
		       int msg_size, int nb_msg
		       )
{
  int i = 0;
  int j = 0;
  int nproc = 0;
  nproc = mpi_conf->nproc;
  result->mpi_conf = mpi_conf;
  result->msg_size = msg_size;
  result->nb_msg = nb_msg;
  result->l_time = (double *) malloc(nproc*sizeof(double));
  result->g_time = (double *) malloc(nproc*nproc*sizeof(double));
  result->g_bw = (double *) malloc(nproc*nproc*sizeof(double));
  for(i=0; i<nproc; i++)
    {
      result->l_time[i] = 0.0;
      for(j=0; j<nproc; j++)
	result->g_bw[i*nproc+j] = 0.0;
    }
}

void hp2p_result_free(hp2p_result *result)
{
  free(result->l_time);
  free(result->g_time);
}

void hp2p_result_update(hp2p_result *result)
{
  int nproc = 0;
  int count = 0;
  int msg_size = 0;
  int nb_msg = 0;
  int i = 0;
  int j = 0;

  nproc = result->mpi_conf->nproc;
  msg_size  = result->msg_size;
  nb_msg  = result->nb_msg;
  
  MPI_Gather(result->l_time, nproc, MPI_DOUBLE,
	     result->g_time, nproc, MPI_DOUBLE,
	     result->mpi_conf->root,
	     result->mpi_conf->comm);

  result->sum_time = 0.0;
  result->min_time = 1000000000.0;
  result->max_time = 0.0;
  for(i=0; i<nproc; i++)
    for(j=0; j<nproc; j++)
      {
	if(result->g_time[i*nproc + j] > 0.0)
	  {
	    // bw
	    result->g_bw[i*nproc + j] = nb_msg * msg_size / result->g_time[i*nproc + j];
	    // sum
	    result->sum_time += result->g_time[i*nproc + j];
	    result->sum_bw += result->g_bw[i*nproc + j];
	    count++;
	    // min
	    if(result->g_time[i*nproc + j] < result->min_time)
	      {
		result->min_time = result->g_time[i*nproc + j];
		result->i_min_time = i;
		result->j_min_time = j;
	      }
	    if(result->g_bw[i*nproc + j] < result->min_bw)
	      {
		result->min_bw = result->g_bw[i*nproc + j];
		result->i_min_bw = i;
		result->j_min_bw = j;
	      }
	    // max
	    if(result->g_time[i*nproc + j] > result->max_time)
	      {
		result->max_time = result->g_time[i*nproc + j];
		result->i_max_time = i;
		result->j_max_time = j;
	      }
	    if(result->g_bw[i*nproc + j] > result->max_bw)
	      {
		result->max_bw = result->g_bw[i*nproc + j];
		result->i_max_bw = i;
		result->j_max_bw = j;
	      }
	  }
      }
  result->avg_time = result->sum_time / (double) count;
  result->avg_bw = result->sum_bw / (double) count;
  result->count_time = count;
  if(nproc < 2)
    {
      result->stdd_bw = 0.0;
    }
  else
    {
      result->stdd_bw = sqrt(result->sum_bw/((double)(nproc-1)));
    }
}

void hp2p_result_display(hp2p_result *result)
{
  printf(" == SUMMARY ==\n\n");
  printf(" Min bandwidth : %0.2lf MB/s\n", result->min_bw / 1048576);
  printf(" Max bandwidth : %0.2lf MB/s\n", result->max_bw / 1048576);
  printf(" Avg bandwidth : %0.2lf MB/s\n", result->avg_bw / 1048576);
  printf(" Std bandwidth : %0.2lf MB/s\n", result->stdd_bw / 1048576);
  printf("\n");
  printf(" Min latency   : %0.2lf us\n", result->min_time * 1000000);
  printf(" Max latency   : %0.2lf us\n", result->max_time * 1000000);
  printf(" Avg latency   : %0.2lf us\n", result->avg_time * 1000000);
  printf(" Std latency   : %0.2lf us\n", result->stdd_time * 1000000);
  printf("\n");
}
