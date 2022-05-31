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
 * \file      hp2p.h
 * \author    Laurent Nguyen <laurent.nguyen@cea.fr>
 * \version   4.0
 * \date      21 June 2021
 * \brief     HP2P Benchmark
 *
 * \details   No
 */
#ifndef __HP2P_H__
#define __HP2P_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#include "mpi.h"
#ifdef _ENABLE_CUDA_
#include <cuda.h>
#include <cuda_runtime.h>
#endif

#define MAXCHARFILE 4096

/**
 * \struct config
 * \brief  Configuration object
 *
 * config is a struture which contains parameters to launch the benchmark
 */
typedef struct
{
  int nb_shuffle;		  // number of iterations
  int snap_freq;		  // number of iterations between snapshots
  int msg_size;			  // message size
  int nb_msg;			  // number of messages per communication
  char inname[MAXCHARFILE];       // configuration filename
  char outname[MAXCHARFILE];      // output filename
  int max_time;	        	  // max duration time in seconds for the run
  int build;	         	  // Algorithm to build couple list
  char *buildname;		  // Name of algorithm to build couple list
  int anonymize;                  // Write hostname or not
  char plotlyjs[MAXCHARFILE];     // Path to a plotly.min.js
  double __start_time;
  int align_size;
  char output_mode[4];
} hp2p_config;

/**
 * \struct mpi_config
 * \brief  Configuration object for MPI
 *
 * config is a struture which contains MPI configuration
 */
typedef struct
{
  int root;
  int rank;
  int local_rank;
  int nproc;
  MPI_Comm comm;
  MPI_Comm local_comm;
  char localhost[MPI_MAX_PROCESSOR_NAME];
  char *hostlist;
} hp2p_mpi_config;

typedef struct
{
  int *l_count;
  double *l_time;
  int *g_count;
  double *g_time;
  double *g_bw;
  double msg_size;
  int nb_msg;
  int count_time;

  double sum_time;
  double avg_time;
  double stdd_time;
  double max_time;
  int i_max_time;
  int j_max_time;
  double min_time;
  int i_min_time;
  int j_min_time;

  double sum_bw;
  double avg_bw;
  double stdd_bw;
  double max_bw;
  int i_max_bw;
  int j_max_bw;
  double min_bw;
  int i_min_bw;
  int j_min_bw;

  hp2p_mpi_config* mpi_conf;
} hp2p_result;

// hp2p_algo
int hp2p_algo_get_num();
char* hp2p_algo_get_name(int algo);
void hp2p_algo_build_couples(int *v, int size, int algo);
void hp2p_algo_mirroring_shift(int *v, int size);
void hp2p_algo_random(int *v, int size);

// hp2p_util
void hp2p_util_set_default_config(hp2p_config *conf);
void hp2p_util_free_config(hp2p_config *conf);
void hp2p_util_display_config(hp2p_config conf);
void hp2p_util_display_help(char command[]);
void hp2p_util_read_configfile(hp2p_config *conf);
void hp2p_util_read_commandline(int argc, char *argv[], hp2p_config *conf);
double hp2p_util_get_time();
void hp2p_util_init_tremain(hp2p_config *conf);
double hp2p_util_tremain(hp2p_config conf);

// hp2p_mpi
int hp2p_mpi_init(int *argc, char ***argv, hp2p_mpi_config *mpi_conf);
int hp2p_mpi_get_hostname(hp2p_mpi_config *mpi_conf, int anonymize);
int hp2p_mpi_finalize(hp2p_mpi_config *mpi_conf);

// hp2p_result
void hp2p_result_alloc(hp2p_result *result,
		       hp2p_mpi_config *mpi_conf,
		       int msg_size, int nb_msg
		       );
void hp2p_result_free(hp2p_result *result);
void hp2p_result_update(hp2p_result *result);
void hp2p_result_display(hp2p_result *result);
void hp2p_result_display_time(hp2p_result *result);
void hp2p_result_display_bw(hp2p_result *result);
void hp2p_result_write(hp2p_result result, hp2p_config conf, hp2p_mpi_config mpi_conf);
#endif
