// Copyright (C) 2010-2022 CEA/DAM
// Copyright (C) 2010-2022 Laurent Nguyen <laurent.nguyen@cea.fr>
//
// This file is part of HP2P.
//
// This software is governed by the CeCILL-C license under French law and
// abiding by the rules of distribution of free software.  You can  use,
// modify and/ or redistribute the software under the terms of the CeCILL-C
// license as circulated by CEA, CNRS and INRIA at the following URL
// "http://www.cecill.info".

/**
 * \file      hp2p.c
 * \author    Laurent Nguyen <laurent.nguyen@cea.fr>
 *            Marc Joos <marc.joos@cea.fr>
 * \version   4.0
 * \date      June 21 2023
 * \brief     HP2P Benchmark
 *
 * \details   No
 */

#include "hp2p.h"

/**
 * \fn       heavy_p2p_iteration(mpi_config mpi_conf, config conf, int other)
 * \brief    HP2P iteration: test a pair of workers several times
 *
 * \param    rank  current rank
 * \param    other other rank of couple
 * \param    nproc
 * \msg_size size of a message
 **/
double hp2p_iteration(hp2p_mpi_config mpi_conf, hp2p_config conf, int other)
{
  double time_hp2p = 0.0;
  int rank = 0;
  MPI_Comm comm;
  int nproc = 0;
  int msg_size = 0;
  int nb_msg = 0;
  int align_size = 0;
  int n = 0;
  int i = 0;
  int *buf1 = NULL;
  int *buf2 = NULL;
#if defined(_ENABLE_CUDA_) || defined(_ENABLE_ROCM_)
  int *d_buf1 = NULL;
  int *d_buf2 = NULL;
#endif
  double t0 = 0.0;
  double t1 = 0.0;

  rank = mpi_conf.rank;
  comm = mpi_conf.comm;
  nproc = mpi_conf.nproc;
  msg_size = conf.msg_size;
  nb_msg = conf.nb_msg;
  align_size = conf.align_size;
  if (rank == other)
  {
    time_hp2p = 0.0;
    MPI_Barrier(comm);
    MPI_Barrier(comm);
  }
  else
  {
    n = msg_size / sizeof(int);

    // Align MPI buffers

    if (posix_memalign((void **)&buf1, align_size, msg_size))
    {
      fprintf(stderr, "Cannot allocate memory...Exit\n");
    }
    if (posix_memalign((void **)&buf2, align_size, msg_size))
    {
      fprintf(stderr, "Cannot allocate memory...Exit\n");
    }

    for (i = 0; i < n; i++)
      buf1[i] = i;

#ifdef _ENABLE_CUDA_
    cudaMalloc(&d_buf1, n * sizeof(int));
    cudaMalloc(&d_buf2, n * sizeof(int));

    cudaMemcpy(d_buf1, buf1, n * sizeof(int), cudaMemcpyHostToDevice);
#endif
#ifdef _ENABLE_ROCM_
    hipMalloc(&d_buf1, n * sizeof(int));
    hipMalloc(&d_buf2, n * sizeof(int));

    hipMemcpy(d_buf1, buf1, n * sizeof(int), hipMemcpyHostToDevice);
#endif

    MPI_Request req[2];
    MPI_Status status[2];

    // First comm
#if defined(_ENABLE_CUDA_) || defined(_ENABLE_ROCM_)
    MPI_Irecv(d_buf2, n, MPI_INT, other, 0, comm, &req[0]);
    MPI_Isend(d_buf1, n, MPI_INT, other, 0, comm, &req[1]);
#else
    MPI_Irecv(buf2, n, MPI_INT, other, 0, comm, &req[0]);
    MPI_Isend(buf1, n, MPI_INT, other, 0, comm, &req[1]);
#endif
    MPI_Waitall(2, req, status);

    MPI_Barrier(comm);
    // send/recv nloops * msg_size MB of data
    t1 = 0;
    t0 = hp2p_util_get_time();
    for (i = 0; i < nb_msg; i++)
    {
#if defined(_ENABLE_CUDA_) || defined(_ENABLE_ROCM_)
      MPI_Irecv(d_buf2, n, MPI_INT, other, 0, comm, &req[0]);
      MPI_Isend(d_buf1, n, MPI_INT, other, 0, comm, &req[1]);
#else
      MPI_Irecv(buf2, n, MPI_INT, other, 0, comm, &req[0]);
      MPI_Isend(buf1, n, MPI_INT, other, 0, comm, &req[1]);
#endif
      MPI_Waitall(2, req, status);
    }

    t1 = hp2p_util_get_time();

    MPI_Barrier(comm);
    free((void *)buf1);
    free((void *)buf2);

#ifdef _ENABLE_CUDA_
    cudaFree(d_buf1);
    cudaFree(d_buf2);
#endif
#ifdef _ENABLE_ROCM__
    hipFree(d_buf1);
    hipFree(d_buf2);
#endif

    time_hp2p = (t1 - t0) / nb_msg;
  }
  return time_hp2p;
}

/**
 * \fn              void heavy_p2p_main(config conf, mpi_config mpi_conf)
 * \brief           The HP2P benchmark. outer loop: number of iterations
 *
 * \param conf      Benchmark configuration
 * \param mpi_conf  MPI configuration
 *
 * => Initialization
 * => loop on:
 *    - build random couples + scatter accross workers
 *    - perform HP2P iteration
 *    - periodic snapshots
 *    - output timings of the current iteration (+ periodic flush)
 * => final output/snapshot + finalization
 **/
void hp2p_main(hp2p_config conf, hp2p_mpi_config mpi_conf)
{
  // MPI Configuration
  int nproc = 0;
  int rank = 0;
  int root = 0;
  MPI_Comm comm;
  // Benchmark parameters
  int nloops = 0;
  int msg_size = 0;
  // Couples array
  int *couples = NULL;

  // Duration control
  double tremain = 1.e6;
  // Timers
  double start = 0.0;
  double time_build_couples = 0.0;
  double time_heavyp2p = 0.0;
  double time_control = 0.0;
  double local_time = 0.;
  double max_time = 0.;

  hp2p_result result;

  int i = 0;
  int other = -1;

  // MPI Configuration
  nproc = mpi_conf.nproc;
  rank = mpi_conf.rank;
  root = mpi_conf.root;
  comm = mpi_conf.comm;
  // Benchmark parameters
  nloops = conf.nb_shuffle;
  msg_size = conf.msg_size;
  hp2p_result_alloc(&result, &mpi_conf, &conf);
  hp2p_util_init_tremain(&conf);
  // Initialize random generator
  if (conf.seed < 0)
  {
    srand(time(NULL));
  }
  else
  {
    srand(conf.seed);
  }

#ifdef _HP2P_SIGNAL
  init_signal_writer(conf);
#endif
  if (rank == root)
  {
    couples = (int *)malloc(nproc * sizeof(int));
  }

  // Main loop
  for (i = 1; i <= nloops && tremain >= 0; i++)
  {
    result.current_iteration = i;
    other = -1;
    // Check time left before job ends
    tremain = hp2p_util_tremain(conf);
    // Build random couples
    if (rank == root)
    {
      start = MPI_Wtime();
      hp2p_algo_build_couples(couples, nproc, conf.build);
    }
    MPI_Scatter(couples, 1, MPI_INT, &other, 1, MPI_INT, root, comm);
    if (rank == root)
      time_build_couples = MPI_Wtime() - start;
    // HP2P iteration
    if (rank == root)
    {
      start = MPI_Wtime();
    }
    local_time = hp2p_iteration(mpi_conf, conf, other);
    result.l_bsbw[i - 1] = msg_size / local_time;
    result.l_time[other] += local_time;
    if (((conf.time_mult < 1.) && (conf.local_max_time > 0.0) &&
	 (conf.local_max_time < local_time)) ||
	((conf.time_mult >= 1.) && (result.avg_time > 0.0) &&
	 (conf.time_mult * result.avg_time < local_time)))
    {
      if (rank < other)
	fprintf(stderr,
		"warning: the communication between %d and %d was slow.\nTime "
		"of communication : %lf\nMean Time of communication : %lf\n",
		rank, other, local_time, result.avg_time);
    }
    if (other != rank)
      result.l_count[other]++;
    if (rank == root)
    {
      time_heavyp2p = MPI_Wtime() - start;
      start = MPI_Wtime();
    }
    // Periodic snapshot
    if (i && ((i % conf.snap_freq) == 0))
    {
      hp2p_result_update(&result);
      if (rank == root)
      {
	hp2p_result_display(&result);
	hp2p_result_write(result);
      }
      MPI_Barrier(comm);
    }
    // Follow the run
    if (nloops >= 100 && rank == root && ((i % (nloops / 100)) == 0))
    {
      printf(" %d %% done\n", (int)(100 * ((double)i) / ((double)nloops)));
    }
#ifdef _HP2P_SIGNAL
    check_signal(result);
#endif
    // output time of each iteration
  }
  // Final snapshot
  // io_snapshot(mpi_conf, conf, ltime, ttime, times, counts, conf.outname);
  hp2p_result_update(&result);
  if (rank == root)
  {
    hp2p_result_display(&result);
    printf(" Writing final result...\n");
    fflush(stdout);
    hp2p_result_write(result);
    printf(" Writing final result... Done\n");
    fflush(stdout);
  }

  MPI_Barrier(comm);
  hp2p_result_free(&result);
  // Release memory and files
  if (rank == root)
  {
    free(couples);
  }
}

int main(int argc, char *argv[])
{
  hp2p_config conf;
  hp2p_mpi_config mpi_conf;

  hp2p_mpi_init(&argc, &argv, &mpi_conf);
  hp2p_util_set_default_config(&conf);
  hp2p_util_read_commandline(argc, argv, &conf);
  if (mpi_conf.rank == mpi_conf.root)
    hp2p_util_display_config(conf);
  hp2p_mpi_get_hostname(&mpi_conf, conf.anonymize);
  hp2p_main(conf, mpi_conf);
  hp2p_util_free_config(&conf);
  hp2p_mpi_finalize(&mpi_conf);
  return EXIT_SUCCESS;
};
