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
 * \file      hp2p_util.c
 * \author    Laurent Nguyen <laurent.nguyen@cea.fr>
 * \version   4.0
 * \date      21 June 2021
 * \brief     HP2P Benchmark
 *
 * \details   No
 */

#include "hp2p.h"

/**
 * \fn     void hp2p_set_default_config(config *conf)
 * \brief  load default config
 *
 * \param  conf
 * \return void
 **/
void hp2p_util_set_default_config(hp2p_config *conf)
{
  conf->nb_shuffle = 5000;
  conf->snap_freq = conf->nb_shuffle + 1;
  conf->msg_size = 1024 * 1024; // 1 Mb
  conf->nb_msg = 10;
  strcpy(conf->inname, "");
  strcpy(conf->outname, "resu");
  conf->max_time = 86400; // 1 day
  conf->build = 0;   // Random
  conf->buildname = hp2p_algo_get_name(conf->build);
  conf->__start_time = 0.0;
  conf->align_size = 8;
  conf->anonymize = 0;
  strcpy(conf->plotlyjs, "");
}
/**
 * \fn     void hp2p_util_free_config(config *conf)
 * \brief  load default config
 *
 * \param  conf
 * \return void
 **/
void hp2p_util_free_config(hp2p_config *conf)
{
  conf->nb_shuffle = 0;
  conf->snap_freq = conf->nb_shuffle + 1;
  conf->msg_size = 0;
  conf->nb_msg = 0;
  strcpy(conf->inname, "");
  strcpy(conf->outname, "");
  strcpy(conf->plotlyjs, "");
  conf->max_time = 0;
  conf->build = 0;
  free(conf->buildname);
}
/**
 * \fn     void display_config(config conf)
 * \brief  display config
 *
 * \param  conf
 * \return void
 **/
void hp2p_util_display_config(hp2p_config conf)
{
  printf("=== Benchmark configuration ===\n");
  printf("\n");
  printf("Configuration file         : %s\n", conf.inname);
  printf("Number of iterations       : %d\n", conf.nb_shuffle);
  printf("Iterations between snapshot: %d\n", conf.snap_freq);
  printf("Message size               : %d\n", conf.msg_size);
  printf("Number of msg per comm     : %d\n", conf.nb_msg);
  printf("Alignment for MPI buffer   : %d\n", conf.align_size);
  printf("Max time                   : %d\n", conf.max_time);
  printf("Build couple algorithm     : %s\n", conf.buildname);
  printf("Output file                : %s\n", conf.outname);
  printf("Anonymize hostname         : %d\n", conf.anonymize);
  printf("Plotly.js file             : %s\n", conf.plotlyjs);
  printf("\n");
  printf("===============================\n");
  printf("\n");
}
/**
 * \fn     void hp2p_display_help
 * \brief  Display help of benchmark
 *
 * \return void
 **/
void hp2p_util_display_help(char command[])
{
  printf("Usage: %s [-h] [-n nit] [-k freq] [-m nb_msg]\n", command);
  printf("       [-s msg_size] [-o output] [-a align] [-y]\n");
  printf("       [-p file]");
  printf("       [-i conf_file]\n");
  printf("Options:\n");
  printf("   -i conf_file    Configuration file\n");
  printf("   -n nit          Number of iterations\n");
  printf("   -k freq         Iterations between snapshot\n");
  printf("   -s msg_size     Message size\n");
  printf("   -m nb_msg       Number of msg per comm\n");
  printf("   -a align        Alignment size for MPI buffer (default=8)\n");
  printf("   -t max_time     Max duration\n" );
  printf("   -c build        Algorithm to build couple\n");
  printf("                   (random = 0 (default), mirroring shift = 1)\n" );
  printf("   -y anon         1 = hide hostname, 0 = write hostname (default)\n");
  printf("   -p jsfile       Path to a plotly.min.js file to include into HTML\n");
  printf("                   Use get_plotlyjs.py script if plotly is installed\n");
  printf("                   in your Python\n");
  printf("   -o output       Output file\n" );
  printf("\n");
}
/**
 * \fn     void read_configfile(char *fname, config *conf)
 * \brief  Read benchmark configuration from a configuration file
 *
 * \param  conf
 * \return void
 **/
void hp2p_util_read_configfile(hp2p_config *conf)
{
  FILE * fp = NULL;
  char *buffer = NULL;
  int max_len = 1024;
  char *key = NULL;
  char *value = NULL;
  
  fp = fopen(conf->inname,"r");
  if(fp == NULL)
    {
      fprintf(stderr, "Cannot open %s ... exit\n", conf->inname);
    }

  buffer = (char *) malloc(max_len*sizeof(char));
  key = (char *) malloc(max_len*sizeof(char));
  value = (char *) malloc(max_len*sizeof(char));

  while (fgets(buffer, max_len - 1, fp))
    {
      // Remove trailing newline
      buffer[strcspn(buffer, "\n")] = 0;
      if(buffer[0] != '#')
	{
	  sscanf(buffer, "%s=%s", key, value);
	  if(strcmp(key, "nb_shuffle"))
	    conf->nb_shuffle = atoi(value);
	  if (strcmp(key, "snap_freq"))
  	    conf->snap_freq = atoi(value);
  	  if (strcmp(key, "msg_size"))
  	    conf->msg_size = atoi(value);
  	  if (strcmp(key, "nb_msg"))
  	    conf->nb_msg = atoi(value);
  	  if (strcmp(key, "align"))
  	    conf->align_size = atoi(value);
	  if (strcmp(key, "outname"))
  	    strcpy(conf->outname, value);
  	  if (strcmp(key, "build"))
	    {
	      conf->build = atoi(value);
	      free(conf->buildname);
	      conf->buildname = hp2p_algo_get_name(conf->build);
	    }
  	  if (strcmp(key, "max_time"))
  	    conf->max_time = atoi(value);
  	  if (strcmp(key, "anonymize"))
  	    conf->anonymize = atoi(value);
  	  if (strcmp(key, "plotlyjs"))
	    strcpy(conf->plotlyjs, value);
	}
    }
  free(buffer);

  fclose(fp);
}
/**
 * \fn     void hp2p_util_read_config(config *conf)
 * \brief  Read benchmark configuration from command line options
 *         or a configuration file
 *
 * \param  argc Number of arguments from commande line
 * \param  argv Array of arguments
 * \param  conf
 * \return void
 **/
void hp2p_util_read_commandline(int argc, char *argv[], hp2p_config *conf)
{
  int opt = 0;

  // Load default config
  hp2p_util_set_default_config(conf);

  // Parsing command line
  while ((opt = getopt(argc, argv, "hn:k:m:s:o:i:c:t:a:y:p:")) != -1)
  {
    switch (opt)
    {
    case 'h':
      hp2p_util_display_help(argv[0]);
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
    case 'a':
      conf->align_size = atoi(optarg);
      break;
    case 's':
      conf->msg_size = atoi(optarg);
      break;
    case 'o':
      strcpy(conf->outname, optarg);
      break;
    case 'c':
    {
      conf->build = atoi(optarg);
      free(conf->buildname);
      conf->buildname = hp2p_algo_get_name(conf->build);;
    }
    break;
    case 't':
      conf->max_time = atoi(optarg);
      break;
    case 'y':
      conf->anonymize = atoi(optarg);
      break;
    case 'i':
      strcpy(conf->inname, optarg);
      break;
    case 'p':
      strcpy(conf->plotlyjs, optarg);
      break;
    default:
      break;
    }
  }

  // If a configuration file is given
  if (strlen(conf->inname) > 0)
  {
    hp2p_util_read_configfile(conf);
  }
}
/**
 * \fn     double hp2p_util_get_time()
 * \brief  convenient function for timing
 *
 * \return Time in seconds
 **/
double hp2p_util_get_time()
{
  struct timeval t;
  gettimeofday(&t, 0);
  return t.tv_sec + 0.000001 * t.tv_usec;
}
/**
 * \fn     void hp2p_init_tremain(config conf)
 * \brief  init time stamp at the beginning
 *
 * \param  conf
 **/
void hp2p_util_init_tremain(hp2p_config *conf)
{
  conf->__start_time = hp2p_util_get_time();
}
/**
 * \fn     double hp2p_tremain(config conf)
 * \brief  init time stamp at the beginning
 *
 * \param  conf
 * \return remaining time before job ends
 **/
double hp2p_util_tremain(hp2p_config conf)
{
  double tremain = 0.0;
  tremain = (double) conf.max_time + conf.__start_time - hp2p_util_get_time();
  return tremain;
}

