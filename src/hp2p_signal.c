// Copyright (C) 2010-2023 CEA/DAM
// Copyright (C) 2010-2023 Laurent Nguyen <laurent.nguyen@cea.fr>
//
// This file is part of HP2P.
//
// This software is governed by the CeCILL-C license under French law and
// abiding by the rules of distribution of free software.  You can  use,
// modify and/ or redistribute the software under the terms of the CeCILL-C
// license as circulated by CEA, CNRS and INRIA at the following URL
// "http://www.cecill.info".

/**
 * \file      hp2p_signal.c
 * \author    Vincent Ducrot <vincent.ducrot.tgcc@cea.fr>
 * \version   4.0
 * \date      September 22 2022
 * \brief     HP2P Benchmark
 *
 * \details   No
 */

int towrite = 0;
int tokill = 0;
#include <hp2p.h>
#include <signal.h>
void signal_handle(int sig)
{
  if (sig != SIGUSR1 && sig != SIGALRM)
    tokill = sig;
  else
    tokill = 0;
  towrite = 1;
}
void init_signal_writer(hp2p_config conf)
{
  signal(SIGALRM, signal_handle);
  signal(SIGUSR1, signal_handle);
  signal(SIGTERM, signal_handle);
  if (conf.alarm != 0)
  {
    alarm(0);
    alarm(conf.alarm);
  }
  tokill = 0;
  towrite = 0;
}
void check_signal(hp2p_result result)
{
  int rank = result.mpi_conf->rank;
  int root = result.mpi_conf->root;
  MPI_Allreduce(MPI_IN_PLACE, &towrite, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
  if (towrite == 1)
  {
    MPI_Allreduce(MPI_IN_PLACE, &tokill, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    hp2p_result_update(&result);
    if (rank == root)
    {
      hp2p_result_display(&result);
      printf("Writing result...\n");
      hp2p_result_write(result);
      fflush(stdout);
    }
    if (tokill != 0)
    {
      if (rank == root)
	printf("received signal %d exiting\n", tokill);
      exit(tokill);
    }
    if (result.conf->alarm != 0)
    {
      alarm(0);
      alarm(result.conf->alarm);
    }
    towrite = 0;
  }
}
