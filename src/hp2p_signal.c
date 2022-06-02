int towrite = 0;
int tokill = 0;
#include <hp2p.h>
#include <signal.h>
void signal_handle(int sig)
{
  if (sig != SIGUSR1 && sig != SIGALRM)
    tokill = sig;
  else
    tokill=0;
  towrite = 1;
  signal(sig,signal_handle);
}
void init_signal_writer(hp2p_config conf)
{
  signal(SIGALRM, signal_handle);
  signal(SIGUSR1, signal_handle);
  signal(SIGTERM, signal_handle);
  if (conf.alarm != 0.)
  {
    alarm(0);
    alarm(conf.alarm);
  }
  tokill = 0;
  towrite = 0;
}
void check_signal(hp2p_result result, hp2p_config conf,
		  hp2p_mpi_config mpi_conf, int rank, int root)
{
  MPI_Allreduce(MPI_IN_PLACE, &towrite, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
  if (towrite == 1)
  {
    MPI_Allreduce(MPI_IN_PLACE, &tokill, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    hp2p_result_update(&result);
    if (rank == root)
    {
      hp2p_result_display(&result);
      printf("Writing result...\n");
      hp2p_result_write(result, conf, mpi_conf);
    }
    if (tokill != 0)
    {
      if(rank == root)
        printf("received signal %d exiting\n",tokill);
      exit(tokill);
    }
    if (conf.alarm != 0.)
    {
      alarm(0);
      alarm(conf.alarm);
    }
    towrite = 0;
  }
}
