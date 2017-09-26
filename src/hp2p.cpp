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
 * \file      hp2p.cpp
 * \author    Laurent Nguyen <laurent.nguyen@cea.fr>
 * \version   3.1
 * \date      26 August 2014
 * \brief     HP2P Benchmark
 *
 * \details   No
 */

#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <vector>

#include <sys/time.h>

#include <mpi.h>

using namespace std;

#define MAXCHARFILE 256
#define GRACETIME 120 // Time in seconds before the benchmark can stop
#ifndef ALIGNSIZE
#define ALIGNSIZE 4096 // Alignment for MPI buffers
#endif

// Build couple algorithm
int nmethod = 2;
enum build_method
{
  RANDOM,
  SHIFT
};
typedef enum build_method build_method;
string build_name[] = {"RANDOM", "SHIFT"};

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
  char hostfilename[MAXCHARFILE]; // hosts and corresponding ranks
  int bin_timer;		  // boolean: 0 to output time of each
				  //            iterations on stderr
				  //          1 for for writing a binary file
  double max_time;		  // max duration time in seconds for the run
  double __start_time;		  // time stamp at when execution start (hidden)
  build_method build;		  // Algorithm to build couple list
} config;
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
  int nproc;
  MPI_Comm comm;
} mpi_config;
/**
 * \fn     void default_config(config *conf)
 * \brief  load default config
 *
 * \param  conf
 * \return void
 **/
void default_config(config *conf)
{
  conf->nb_shuffle = 50000;
  conf->snap_freq = 1000;
  conf->msg_size = 1024 * 1024; // 1 Mb
  conf->nb_msg = 10;
  strcpy(conf->outname, "resu");
  strcpy(conf->inname, "");
  strcpy(conf->hostfilename, "hostfile.txt");
  conf->bin_timer = 0;
  conf->max_time = 86400; // 1 day
  conf->build = RANDOM;   // Random
}
/**
 * \fn     void display_config(config conf)
 * \brief  display config
 *
 * \param  conf
 * \return void
 **/
void display_config(config conf)
{
  cout << "=== Benchmark configuration ===" << endl;
  cout << endl;
  cout << "Configuration file         : " << conf.inname << endl;
  cout << "Number of iterations       : " << conf.nb_shuffle << endl;
  cout << "Iterations between snapshot: " << conf.snap_freq << endl;
  cout << "Message size               : " << conf.msg_size << endl;
  cout << "Number of msg per comm     : " << conf.nb_msg << endl;
  cout << "Bin timer                  : " << conf.bin_timer << endl;
  cout << "Max time                   : " << conf.max_time << endl;
  cout << "Hostfile                   : " << conf.hostfilename << endl;
  cout << "Build couple algorithm     : " << build_name[conf.build] << endl;
  cout << "Output file                : " << conf.outname << endl;
  cout << endl;
  cout << "===============================" << endl;
  cout << endl;
}
/**
 * \fn     void display_help()
 * \brief  Display help of benchmark
 *
 * \return void
 **/
void display_help(char command[])
{
  cerr << "Usage: " << command << " [-h] [-n nit] [-k freq] [-m nb_msg]"
       << endl;
  cerr << "       [-s msg_size] [-o output] [-r hostfile]" << endl;
  cerr << "       [-b bin_timer] [-i conf_file]" << endl;
  cerr << "Options:" << endl;
  cerr << "   -i conf_file    Configuration file" << endl;
  cerr << "   -n nit          Number of iterations" << endl;
  cerr << "   -k freq         Iterations between snapshot" << endl;
  cerr << "   -s msg_size     Message size" << endl;
  cerr << "   -m nb_msg       Number of msg per comm" << endl;
  cerr << "   -b bin_timer    Generate Bin timer (false = 0 (default), true = 1)" << endl;
  cerr << "   -t max_time     Max duration" << endl;
  cerr << "   -c build        Algorithm to build couple ";
  cerr << "(random = 0 (default), mirroring shift = 1)" << endl;
  cerr << "   -r hostfile     Hostfile" << endl;
  cerr << "   -o output       Output file" << endl;
  cerr << endl;
}
/**
 * \fn     void read_configfile(char *fname, config *conf)
 * \brief  Read benchmark configuration from a configuration file
 *
 * \param  conf
 * \return void
 **/
void read_configfile(config *conf)
{
  std::ifstream fd(conf->inname);
  if (!fd.is_open())
  {
    cerr << "Error: File doesn't exist" << endl;
    exit(EXIT_FAILURE);
  }
  std::string line;
  while (std::getline(fd, line))
  {
    if (line[0] != '#') // skip comment
    {
      std::istringstream is_line(line);
      std::string key;
      char delim = '=';
      if (std::getline(is_line, key, delim))
      {
	std::string value;
	if (std::getline(is_line, value))
	{
	  if (key == "nb_shuffle")
	    conf->nb_shuffle = atoi(value.c_str());
	  if (key == "snap_freq")
	    conf->snap_freq = atoi(value.c_str());
	  if (key == "msg_size")
	    conf->msg_size = atoi(value.c_str());
	  if (key == "nb_msg")
	    conf->nb_msg = atoi(value.c_str());
	  if (key == "outname")
	    strcpy(conf->outname, value.c_str());
	  if (key == "hostfile")
	    strcpy(conf->hostfilename, value.c_str());
	  if (key == "bin_timer")
	    conf->bin_timer = atoi(value.c_str());
	  if (key == "build")
	  {
	    int tmp = atoi(value.c_str());
	    if (tmp == (int)RANDOM)
	      conf->build = RANDOM;
	    if (tmp == (int)SHIFT)
	      conf->build = SHIFT;
	  }
	  if (key == "max_time")
	    conf->max_time = atof(value.c_str());
	}
      }
    }
  }
}
/**
 * \fn     void read_config(config *conf)
 * \brief  Read benchmark configuration from command line options
 *         or a configuration file
 *
 * \param  argc Number of arguments from commande line
 * \param  argv Array of arguments
 * \param  conf
 * \return void
 **/
void read_config(int argc, char *argv[], config *conf)
{
  int opt = 0;

  // Load default config
  default_config(conf);

  // Parsing command line
  while ((opt = getopt(argc, argv, "hn:k:m:s:o:r:b:i:c:t:")) != -1)
  {
    switch (opt)
    {
    case 'h':
      display_help(argv[0]);
      exit(EXIT_SUCCESS);
      break;
    case 'n':
      conf->nb_shuffle = atoi(optarg);
      break;
    case 'k':
      conf->snap_freq = atoi(optarg);
      break;
    case 'm':
      conf->nb_msg = atoi(optarg);
      break;
    case 's':
      conf->msg_size = atoi(optarg);
      break;
    case 'o':
      strcpy(conf->outname, optarg);
      break;
    case 'r':
      strcpy(conf->hostfilename, optarg);
      break;
    case 'b':
      conf->bin_timer = atoi(optarg);
      break;
    case 'c':
    {
      int tmp = atoi(optarg);
      if (tmp == (int)RANDOM)
	conf->build = RANDOM;
      if (tmp == (int)SHIFT)
	conf->build = SHIFT;
    }
    break;
    case 't':
      conf->max_time = atof(optarg);
      break;
    case 'i':
      strcpy(conf->inname, optarg);
      break;
    default:
      break;
    }
  }

  // If a configuration file is given
  if (strlen(conf->inname) > 0)
  {
    read_configfile(conf);
  }
}
/**
 * \fn     double get_time()
 * \brief  convenient function for timing
 *
 * \return Time in seconds
 **/
double get_time()
{
  struct timeval t;
  gettimeofday(&t, 0);
  return t.tv_sec + 0.000001 * t.tv_usec;
}
/**
 * \fn     voif init_hp2p_tremain(config conf)
 * \brief  init time stamp at the beginning
 *
 * \param  conf
 **/
void init_hp2p_tremain(config *conf) { conf->__start_time = get_time(); }
/**
 * \fn     double hp2p_tremain(config conf)
 * \brief  init time stamp at the beginning
 *
 * \param  conf
 * \return remaining time before job ends
 **/
double hp2p_tremain(config conf)
{
  double tremain = 0.0;
  tremain = conf.max_time + conf.__start_time - get_time();
  return tremain;
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
void build_couples(int *v, int size, build_method algo)
{
  static int init = 0;
  // Generate random couples
  if (algo == RANDOM)
  {
    int i, j;
    std::vector<int> lst;
    for (i = 0; i < size; i++)
      lst.push_back(i);

    std::random_shuffle(lst.begin(), lst.end());
    // LN : odd size
    for (int ii = 0; ii < size - 1; ii += 2)
    {
      i = lst[ii];
      j = lst[ii + 1];
      v[i] = j;
      v[j] = i;
    }
    if (size % 2 == 1)
      v[lst[size - 1]] = lst[size - 1];
  }
  // Couples will be mirroring shift - ly permuted
  else
  {
    for (int i = 0; i < size; i++)
    {
      v[i] = (size + 1 - i + init) % size;
    }
    init++;
  }
}

/**
 * \fn       heavy_p2p_iteration(mpi_config mpi_conf, config conf, int other)
 * \brief    HP2P iteration: test a pair of workers several times
 *
 * \param    rank  current rank
 * \param    other other rank of couple
 * \param    nproc
 * \msg_size size of a message
 **/
double heavy_p2p_iteration(mpi_config mpi_conf, config conf, int other)
{
  double time_hp2p = 0.0;
  int rank = mpi_conf.rank;
  MPI_Comm comm = mpi_conf.comm;
  int nproc = mpi_conf.nproc;
  int msg_size = conf.msg_size;
  int nb_msg = conf.nb_msg;
  if (rank == other)
  {
    time_hp2p = 0.0;
    MPI_Barrier(comm);
    MPI_Barrier(comm);
  }
  else
  {
    const int N = msg_size / sizeof(int);
    int *buf1 = NULL;
    int *buf2 = NULL;

    // Align MPI buffers
    posix_memalign((void **)&buf1, ALIGNSIZE, msg_size);
    posix_memalign((void **)&buf2, ALIGNSIZE, msg_size);

    for (int i = 0; i < N; i++)
      buf1[i] = i;

    MPI_Request req[2];
    MPI_Status status[2];

    // First comm
    MPI_Irecv(buf2, N, MPI_INT, other, 0, comm, &req[0]);
    MPI_Isend(buf1, N, MPI_INT, other, 0, comm, &req[1]);
    MPI_Waitall(2, req, status);

    MPI_Barrier(comm);
    // send/recv nloops * msg_size MB of data
    const int nloops = nb_msg;
    double t0 = get_time();
    double t1 = 0;
    for (int k = 0; k < nloops; k++)
    {
      MPI_Irecv(buf2, N, MPI_INT, other, 0, comm, &req[0]);
      MPI_Isend(buf1, N, MPI_INT, other, 0, comm, &req[1]);
      MPI_Waitall(2, req, status);
    }

    t1 = get_time();

    MPI_Barrier(comm);
    free((void *)buf1);
    free((void *)buf2);
    time_hp2p = (t1 - t0) / nloops;
  }
  return time_hp2p;
}
/**
 * \fn              void io_snapshot(mpi_config mpi_conf, config conf,
 *                                   double *ltime, double *times,
 *                                   int *counts, char *gname)
 * \brief           Snapshot with MPI_IO
 *
 * \param conf      Benchmark configuration
 * \param mpi_conf  MPI configuration
 * \param ltime     temporary array
 * \param times     array of times for messages
 * \param counts    number of draws
 * \param gname     output snapshot filename
 *
 **/
void io_snapshot(mpi_config mpi_conf, config conf, double *ltime, double *ttime,
		 double *times, int *counts, string gname)
{
  // MPI Configuration
  int nproc = mpi_conf.nproc;
  int rank = mpi_conf.rank;
  int root = mpi_conf.root;
  MPI_Comm comm = mpi_conf.comm;
  // Benchmark configuration
  int msg_size = conf.msg_size;
  // Output
  string bw_filename = gname + ".bin";
  string time_filename = gname + ".time.bin";
  string count_filename = gname + ".count.bin";

  MPI_File bwfile, timefile, countfile;
  MPI_Info infoin, infoout, infodel;
  char value[1024] = "";
  int flag = 0, count = 0;
  MPI_Status status;

  for (int ii = 0; ii < nproc; ii++)
  {
    ltime[ii] = ((counts[ii] != 0 && times[ii] > 1.e-6)
		     ? msg_size / (times[ii] / counts[ii])
		     : 0.);
    ttime[ii] = ((counts[ii] != 0) ? (times[ii] / counts[ii]) : 0.);
  }
  // Delete file before write
  if (rank == root)
  {
    // Delete by MPI - doesn't work
    // MPI_File_delete(gname, infodel);
    // Original code
    string del_cmd =
	"rm -rf " + bw_filename + " " + time_filename + " " + count_filename;
    std::system(del_cmd.c_str());
    cout << "Saving snapshot" << endl;
  }
  MPI_Barrier(comm);
  MPI_File_open(comm, (char *)bw_filename.c_str(),
		MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &bwfile);
  MPI_File_open(comm, (char *)time_filename.c_str(),
		MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &timefile);
  MPI_File_open(comm, (char *)count_filename.c_str(),
		MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &countfile);
  if (rank == root)
  {
    MPI_File_get_info(bwfile, &infoout);
    // MPI_Info_get( infoout, "cb_nodes", 1024, value, &flag );
    cout << "MPI-IO Hint cb_nodes = " << value << endl;
  }
  // set file view
  MPI_File_set_view(bwfile, rank * nproc * sizeof(double), MPI_DOUBLE,
		    MPI_DOUBLE, "native", MPI_INFO_NULL);
  MPI_File_set_view(timefile, rank * nproc * sizeof(double), MPI_DOUBLE,
		    MPI_DOUBLE, "native", MPI_INFO_NULL);
  MPI_File_set_view(countfile, rank * nproc * sizeof(int), MPI_INT, MPI_INT,
		    "native", MPI_INFO_NULL);
  // write buffer to file
  MPI_File_write_all(bwfile, ltime, nproc, MPI_DOUBLE, &status);
  MPI_File_write_all(timefile, ttime, nproc, MPI_DOUBLE, &status);
  MPI_File_write_all(countfile, counts, nproc, MPI_INT, &status);
  // close file
  MPI_File_close(&bwfile);
  MPI_File_close(&timefile);
  MPI_File_close(&countfile);
  MPI_Barrier(comm);
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
void heavy_p2p_main(config conf, mpi_config mpi_conf)
{
  // MPI Configuration
  int nproc = mpi_conf.nproc;
  int rank = mpi_conf.rank;
  int root = mpi_conf.root;
  MPI_Comm comm = mpi_conf.comm;
  // Benchmark parameters
  int nloops = conf.nb_shuffle;
  int msg_size = conf.msg_size;
  // Output file
  string bin_timer_filename = conf.outname;
  bin_timer_filename += ".bin_timer";
  ofstream f_bin_timer;
  // Couples array
  int *couples = NULL;
  if (rank == root)
  {
    couples = new int[nproc];
  }
  double *times = new double[nproc];
  int *counts = new int[nproc];
  for (int i = 0; i < nproc; i++)
  {
    times[i] = 0.0;
    counts[i] = 0;
  }
  double *ltime = new double[nproc];
  double *ttime = new double[nproc];
  // Duration control
  double tremain = 1.e6;
  // Timers
  double start = 0.0;
  double time_build_couples = 0.0;
  double time_heavyp2p = 0.0;
  double time_control = 0.0;
  // Open bin timer file
  if (rank == root && conf.bin_timer)
  {
    f_bin_timer.open(bin_timer_filename.c_str(), ios::out | ios::binary);
  }
  // Main loop
  for (int i = 1; i <= nloops && tremain >= GRACETIME; i++)
  {
    int other = -1;
    // Check time left before job ends
    tremain = hp2p_tremain(conf);
    // Build random couples
    if (rank == root)
    {
      start = MPI_Wtime();
      build_couples(couples, nproc, conf.build);
    }
    MPI_Scatter(couples, 1, MPI_INT, &other, 1, MPI_INT, root, comm);
    if (rank == root)
      time_build_couples = MPI_Wtime() - start;
    // HP2P iteration
    if (rank == root)
    {
      start = MPI_Wtime();
    }
    times[other] += heavy_p2p_iteration(mpi_conf, conf, other);
    if (other != rank)
      counts[other]++;
    if (rank == root)
    {
      time_heavyp2p = MPI_Wtime() - start;
      start = MPI_Wtime();
    }
    // Periodic snapshot
    if (i && ((i % conf.snap_freq) == 0))
    {
      io_snapshot(mpi_conf, conf, ltime, ttime, times, counts, conf.outname);
    }
    // Follow the run
    if (nloops >= 100 && rank == root && ((i % (nloops / 100)) == 0))
    {
      cout << 100 * ((double)i) / ((double)nloops) << "% done" << endl;
    }
    // output time of each iteration
    if (rank == root)
    {
      time_control = MPI_Wtime() - start;
      start = MPI_Wtime();
      if (!conf.bin_timer)
      {
	std::cerr << "Couple " << i
		  << "\t Time Building Couple : " << time_build_couples
		  << " s\t HeavyP2P : " << time_heavyp2p
		  << "s\ttime for log and output " << time_control
		  << "s\tremaining run time : " << tremain << "s\n"
		  << std::endl;
      }
      else
      {
	f_bin_timer.write((char *)&time_build_couples, sizeof(double));
	f_bin_timer.write((char *)&time_heavyp2p, sizeof(double));
	f_bin_timer.write((char *)&time_control, sizeof(double));
	if (i % conf.snap_freq == 0)
	  f_bin_timer.flush();
      }
    }
  }
  // Final snapshot
  io_snapshot(mpi_conf, conf, ltime, ttime, times, counts, conf.outname);
  // Some stats
  double local_bw = 0.0;
  double local_bw_min = 1e16;
  double local_bw_max = 0.0;
  double bw = 0.0;
  double bw_max = 0.0;
  double bw_min = 0.0;
  int local_bw_count = 0;
  int bw_count = 0;
  double local_time = 0.0;
  double local_time_min = 9999.0;
  double local_time_max = 0.0;
  int local_time_count = 0.0;
  int lat_count = 0;
  double lat = 0.0;
  double lat_max = 0.0;
  double lat_min = 0.0;

  for (int ii = 0; ii < nproc; ii++)
  {
    if (ltime[ii] > 0.0)
    {
      if (ltime[ii] > local_bw_max)
	local_bw_max = ltime[ii];
      if (ltime[ii] < local_bw_min)
	local_bw_min = ltime[ii];
      local_bw += ltime[ii];
      local_bw_count++;
    }
    if (ttime[ii] > 0.0)
    {
      if (ttime[ii] > local_time_max)
	local_time_max = ttime[ii];
      if (ttime[ii] < local_time_min)
	local_time_min = ttime[ii];
      local_time += ttime[ii];
      local_time_count++;
    }
  }

  MPI_Allreduce(&local_bw, &bw, 1, MPI_DOUBLE, MPI_SUM, comm);
  MPI_Allreduce(&local_bw_count, &bw_count, 1, MPI_INT, MPI_SUM, comm);
  bw = bw / (double)bw_count;
  MPI_Allreduce(&local_bw_max, &bw_max, 1, MPI_DOUBLE, MPI_MAX, comm);
  MPI_Allreduce(&local_bw_min, &bw_min, 1, MPI_DOUBLE, MPI_MIN, comm);

  MPI_Allreduce(&local_time, &lat, 1, MPI_DOUBLE, MPI_SUM, comm);
  MPI_Allreduce(&local_time_count, &lat_count, 1, MPI_INT, MPI_SUM, comm);
  lat = lat / (double)lat_count;
  MPI_Allreduce(&local_time_max, &lat_max, 1, MPI_DOUBLE, MPI_MAX, comm);
  MPI_Allreduce(&local_time_min, &lat_min, 1, MPI_DOUBLE, MPI_MIN, comm);

  if (rank == root)
  {
    cout << " == SUMMARY ==" << endl << endl;
    cout << " Min bandwidth : " << bw_min / 1048576 << " MB/s" << endl;
    cout << " Avg bandwidth : " << bw / 1048576 << " MB/s" << endl;
    cout << " Max bandwidth : " << bw_max / 1048576 << " MB/s" << endl;
    cout << endl;
    cout << " Min latency   : " << lat_min * 1000000 << " us" << endl;
    cout << " Avg latency   : " << lat * 1000000 << " us" << endl;
    cout << " Max latency   : " << lat_max * 1000000 << " us" << endl;
    cout << endl;
  }

  // Release memory and files
  if (rank == root)
  {
    f_bin_timer.close();
    delete[] couples;
  }
  delete[] times;
  delete[] counts;
  delete[] ltime;
  delete[] ttime;
}
/**
 * \fn     int main (int *argc, char *argv[])
 * \brief  Main program
 *
 * \param
 * \return EXIT_SUCCESS
 */
int main(int argc, char *argv[])
{
  config conf;
  mpi_config mpi_conf;
  int namelen = 0;
  char name[MPI_MAX_PROCESSOR_NAME] = "";
  char *gname = NULL; // List of all

  // MPI Initialisation
  mpi_conf.comm = MPI_COMM_WORLD;
  mpi_conf.root = 0; // Rank 0 is the root rank

  MPI_Init(&argc, &argv);

  // Be careful, every process doesn't have the same time for their MPI_Init
  init_hp2p_tremain(&conf);

  MPI_Comm_size(mpi_conf.comm, &(mpi_conf.nproc));
  MPI_Comm_rank(mpi_conf.comm, &(mpi_conf.rank));
  MPI_Get_processor_name(name, &namelen);

  // Read benchmark configuration from command line options
  // or a configuration file
  // only processor 0 read the configuration file and broadcast
  // on others
  if (mpi_conf.rank == mpi_conf.root)
  {
    read_config(argc, argv, &conf);
    display_config(conf);
  }
  MPI_Bcast(&conf, sizeof(conf), MPI_CHAR, mpi_conf.root, mpi_conf.comm);
  // Rank 0 retrieve all (mpi_rank, name) couples
  if (mpi_conf.rank == mpi_conf.root)
  {
    gname = new char[mpi_conf.nproc * MPI_MAX_PROCESSOR_NAME];
  }
  MPI_Gather(name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, gname,
	     MPI_MAX_PROCESSOR_NAME, MPI_CHAR, mpi_conf.root, mpi_conf.comm);
  if (mpi_conf.rank == mpi_conf.root)
  {
    ofstream fd(conf.hostfilename);
    if (!fd)
    {
      cerr << "Error: cannot open hostfile %s" << endl;
      exit(EXIT_FAILURE);
    }
    for (int i = 0; i < mpi_conf.nproc; i++)
    {
      // Print on stdout
      cout << "Rank " << i << " is running on ";
      cout << &gname[i * MPI_MAX_PROCESSOR_NAME] << endl;
      // Print in file (OpenMPI hostfile style)
      fd << "rank " << i << "=";
      fd << &gname[i * MPI_MAX_PROCESSOR_NAME] << endl;
    }
    fd.close();
    delete[] gname;
  }

  // HP2P benchmark
  heavy_p2p_main(conf, mpi_conf);

  MPI_Finalize();

  return EXIT_SUCCESS;
}
