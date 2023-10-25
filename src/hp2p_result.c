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
 * \file      hp2p_result.c
 * \author    Laurent Nguyen <laurent.nguyen@cea.fr>
 * \version   4.0
 * \date      21 June 2021
 * \brief     HP2P Benchmark
 *
 * \details   No
 */

#include "hp2p.h"

void hp2p_result_alloc(hp2p_result *result, hp2p_mpi_config *mpi_conf,
		       hp2p_config *conf)
{
  int i = 0;
  int j = 0;
  int nproc = 0;
  nproc = mpi_conf->nproc;
  result->mpi_conf = mpi_conf;
  result->conf = conf;
  result->msg_size = conf->msg_size;
  result->nb_msg = conf->nb_msg;
  result->l_time = (double *)malloc(nproc * sizeof(double));
  result->l_count = (int *)malloc(nproc * sizeof(int));
  result->g_count = (int *)malloc(nproc * nproc * sizeof(int));
  result->g_time = (double *)malloc(nproc * nproc * sizeof(double));
  result->g_bw = (double *)malloc(nproc * nproc * sizeof(double));
  for (i = 0; i < nproc; i++)
  {
    result->l_time[i] = 0.0;
    result->l_count[i] = 0;
    for (j = 0; j < nproc; j++)
      result->g_bw[i * nproc + j] = 0.0;
  }
  result->l_bsbw = (double *)calloc(conf->nb_shuffle, sizeof(double));
  result->g_bsbw = (double *)calloc(conf->nb_shuffle, sizeof(double));

  result->monitor_build_couples =
      (double *)calloc(conf->nb_shuffle, sizeof(double));
  result->monitor_heavyp2p = (double *)calloc(conf->nb_shuffle, sizeof(double));
  result->monitor_snapshot = (double *)calloc(conf->nb_shuffle, sizeof(double));
}

void hp2p_result_free(hp2p_result *result)
{
  free(result->l_count);
  free(result->l_time);
  free(result->g_count);
  free(result->g_time);
  free(result->g_bw);
  free(result->l_bsbw);
  free(result->g_bsbw);
  free(result->monitor_build_couples);
  free(result->monitor_heavyp2p);
  free(result->monitor_snapshot);
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
  msg_size = result->msg_size;
  nb_msg = result->nb_msg;

  MPI_Allgather(result->l_time, nproc, MPI_DOUBLE, result->g_time, nproc,
		MPI_DOUBLE, result->mpi_conf->comm);
  MPI_Allgather(result->l_count, nproc, MPI_INT, result->g_count, nproc,
		MPI_INT, result->mpi_conf->comm);

  MPI_Allreduce(result->l_bsbw, result->g_bsbw, result->current_iteration,
		MPI_DOUBLE, MPI_SUM, result->mpi_conf->comm);

  result->sum_time = 0.0;
  result->min_time = 1.0e15;
  result->max_time = 0.0;
  result->sum_bw = 0.0;
  result->min_bw = 1.0e15;
  result->max_bw = 0.0;
  result->i_min_time = 0;
  result->j_min_time = 0;
  result->i_min_bw = 0;
  result->j_min_bw = 0;
  result->i_max_time = 0;
  result->j_max_time = 0;
  result->i_max_bw = 0;
  result->j_max_bw = 0;

  for (i = 0; i < nproc; i++)
    for (j = 0; j < nproc; j++)
    {
      if (result->g_time[i * nproc + j] > 0.0)
      {
	result->g_time[i * nproc + j] =
	    result->g_time[i * nproc + j] / result->g_count[i * nproc + j];
	// bw
	result->g_bw[i * nproc + j] = msg_size / result->g_time[i * nproc + j];
	// sum
	result->sum_time += result->g_time[i * nproc + j];
	result->sum_bw += result->g_bw[i * nproc + j];
	count++;
	// min
	if (result->g_time[i * nproc + j] < result->min_time)
	{
	  result->min_time = result->g_time[i * nproc + j];
	  result->i_min_time = i;
	  result->j_min_time = j;
	}
	if (result->g_bw[i * nproc + j] < result->min_bw)
	{
	  result->min_bw = result->g_bw[i * nproc + j];
	  result->i_min_bw = i;
	  result->j_min_bw = j;
	}
	// max
	if (result->g_time[i * nproc + j] > result->max_time)
	{
	  result->max_time = result->g_time[i * nproc + j];
	  result->i_max_time = i;
	  result->j_max_time = j;
	}
	if (result->g_bw[i * nproc + j] > result->max_bw)
	{
	  result->max_bw = result->g_bw[i * nproc + j];
	  result->i_max_bw = i;
	  result->j_max_bw = j;
	}
      }
    }
  result->avg_time = result->sum_time / (double)count;
  result->avg_bw = result->sum_bw / (double)count;
  result->count_time = count;

  // standard deviation
  result->stdd_bw = 0.0;
  result->stdd_time = 0.0;
  for (i = 0; i < nproc; i++)
    for (j = 0; j < nproc; j++)
    {
      if (result->g_time[i * nproc + j] > 0.0)
      {
	result->stdd_time +=
	    (result->g_time[i * nproc + j] - result->avg_time) *
	    (result->g_time[i * nproc + j] - result->avg_time);
	result->stdd_bw += (result->g_bw[i * nproc + j] - result->avg_bw) *
			   (result->g_bw[i * nproc + j] - result->avg_bw);
      }
    }

  if (nproc < 2)
  {
    result->stdd_bw = 0.0;
  }
  else
  {
    result->stdd_bw = sqrt(result->stdd_bw / ((double)count));
    result->stdd_time = sqrt(result->stdd_time / ((double)count));
  }

  // Bisection bandwidth
  result->sum_bsbw = 0.0;
  result->min_bsbw = 1.0e15;
  result->max_bsbw = 0.0;
  result->stdd_bsbw = 0.0;

  for (i = 0; i < result->current_iteration; i++)
  {
    result->g_bsbw[i] = result->g_bsbw[i] / 2.0;
    result->sum_bsbw += result->g_bsbw[i];
    if (result->g_bsbw[i] > result->max_bsbw)
      result->max_bsbw = result->g_bsbw[i];
    if (result->g_bsbw[i] < result->min_bsbw)
      result->min_bsbw = result->g_bsbw[i];
  }
  result->avg_bsbw = result->sum_bsbw / (result->current_iteration + 1);
  for (i = 0; i < result->current_iteration; i++)
    result->stdd_bsbw += (result->g_bsbw[i] - result->avg_bsbw) *
			 (result->g_bsbw[i] - result->avg_bsbw);

  if (result->current_iteration < 1)
    result->stdd_bsbw = 0.0;
  else
    result->stdd_bsbw =
	sqrt(result->stdd_bsbw / ((double)result->current_iteration + 1));
}

void hp2p_result_display(hp2p_result *result)
{
  double m = 1048576.0;
  int ncouples = result->mpi_conf->nproc / 2;
  printf("\n\n");
  printf(" === SUMMARY ===\n\n");
  printf(" Number of iteration      : %d \n\n", result->current_iteration);
  printf(" Min bandwidth            : %0.2lf MB/s\n", result->min_bw / m);
  printf(" Max bandwidth            : %0.2lf MB/s\n", result->max_bw / m);
  printf(" Avg bandwidth            : %0.2lf MB/s\n", result->avg_bw / m);
  printf(" Std bandwidth            : %0.2lf MB/s\n", result->stdd_bw / m);
  printf("\n");
  printf(" Min latency              : %0.2lf us\n", result->min_time * 1000000);
  printf(" Max latency              : %0.2lf us\n", result->max_time * 1000000);
  printf(" Avg latency              : %0.2lf us\n", result->avg_time * 1000000);
  printf(" Std latency              : %0.2lf us\n",
	 result->stdd_time * 1000000);
  printf("\n");
  printf(" Min bisection bandwidth  : %0.2lf MB/s\n", result->min_bsbw / m);
  printf(" Max bisection bandwidth  : %0.2lf MB/s\n", result->max_bsbw / m);
  printf(" Avg bisection bandwidth  : %0.2lf MB/s\n", result->avg_bsbw / m);
  printf(" Std bisection bandwidth  : %0.2lf MB/s\n", result->stdd_bsbw / m);
  printf("\n");
  printf(" Min bisection efficiency : %0.2lf %%\n",
	 100.0 * result->min_bsbw / (ncouples * result->min_bw));
  printf(" Max bisection efficiency : %0.2lf %%\n",
	 100.0 * result->max_bsbw / (ncouples * result->max_bw));
  printf(" Avg bisection efficiency : %0.2lf %%\n",
	 100.0 * result->avg_bsbw / (ncouples * result->avg_bw));
  printf("\n");
  printf(" ===============\n\n");
}

void hp2p_result_display_time(hp2p_result *result)
{
  int i = 0;
  int j = 0;
  int nproc = 0;
  nproc = result->mpi_conf->nproc;
  for (i = 0; i < nproc; i++)
  {
    for (j = 0; j < nproc; j++)
      printf("%lf\t", result->g_time[i * nproc + j]);
    printf("\n");
  }
}

void hp2p_result_display_bw(hp2p_result *result)
{
  int i = 0;
  int j = 0;
  int nproc = 0;
  nproc = result->mpi_conf->nproc;
  for (i = 0; i < nproc; i++)
  {
    for (j = 0; j < nproc; j++)
      printf("%lf\t", result->g_bw[i * nproc + j] / (1024.0 * 1024.0));
    printf("\n");
  }
}

void hp2p_result_write_html_get_time(char *date, char *hour)
{
  time_t now;
  struct tm *ltm;

  now = time(0);
  ltm = localtime(&now);
  sprintf(date, "%d/%d/%d", ltm->tm_mday, 1 + ltm->tm_mon, 1900 + ltm->tm_year);
  sprintf(hour, "%d:%d:%d", ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
}

void hp2p_result_write_binary(hp2p_result result)
{
  FILE *fp = NULL;
  char *filename = NULL;
  char date[1024];
  char hour[1024];
  int i = 0;
  int j = 0;
  int nproc = 0;
  char ch = '\0';
  int pos = 0;

  nproc = result.mpi_conf->nproc;
  hp2p_result_write_html_get_time(date, hour);
  filename = (char *)malloc((strlen(result.conf->outname) + 16) * sizeof(char));
  strcpy(filename, result.conf->outname);
  strcat(filename, ".bin");
  fp = fopen(filename, "wb");
  /* file format is :
   *  nb rank
   *  hostlist[nbrank]
   *  result[nbrank*nbrank]
   */
  if (fp != NULL)
  {
    nproc = result.mpi_conf->nproc;
    fwrite(&nproc, sizeof(int), 1, fp);
    fwrite(result.mpi_conf->hostlist, sizeof(char),
	   nproc * MPI_MAX_PROCESSOR_NAME, fp);
    fwrite(result.g_bw, sizeof(double), nproc * nproc, fp);
    fwrite(result.g_time, sizeof(double), nproc * nproc, fp);
    fwrite(result.g_count, sizeof(int), nproc * nproc, fp);
    fclose(fp);
  }
  free(filename);
  hp2p_result_display_bw(&result);
}

void hp2p_result_write_html_css(FILE *fp)
{
  if (fp != NULL)
  {
    fprintf(fp, "  <style>\n");
    fprintf(fp, "\n");
    fprintf(fp, "    body {\n");
    fprintf(fp, "        margin:0;\n");
    fprintf(fp, "    }\n");
    fprintf(fp, "\n");
    fprintf(fp, "    .banner {\n");
    fprintf(fp, "      overflow: hidden;\n");
    fprintf(fp, "      background-color: #114073;\n");
    fprintf(fp, "      margin: 0;\n");
    fprintf(fp, "      height: 75px;\n");
    fprintf(fp, "      margin-left: 0px;\n");
    fprintf(fp, "      margin-right: 0px;\n");
    fprintf(fp, "      width: 100%%;\n");
    fprintf(fp, "    }\n");
    fprintf(fp, "    .banner > h2 {\n");
    fprintf(fp, "      float: left;\n");
    fprintf(fp, "      color: white;\n");
    fprintf(fp, "      padding: 2px;\n");
    fprintf(fp, "      font-size: 25px;\n");
    fprintf(fp, "      margin-left: 8%%;\n");
    fprintf(fp, "      font-family: 'Open Sans', sans-serif;\n");
    fprintf(fp, "    }\n");
    fprintf(fp, "\n");
    fprintf(fp, "    .stats-container {\n");
    fprintf(fp, "      padding: 0;\n");
    fprintf(fp, "      margin: 0;\n");
    fprintf(fp, "      display: flex;\n");
    fprintf(fp, "      flex-wrap: wrap;\n");
    fprintf(fp, "      align-items: baseline;\n");
    fprintf(fp, "      justify-content: center;\n");
    fprintf(fp, "    }\n");
    fprintf(fp, "     .stats-container > div {\n");
    fprintf(fp, "      margin: 4px;\n");
    fprintf(fp, "      padding: 2px;\n");
    fprintf(fp, "      background-color: white;\n");
    fprintf(fp, "      width: 800px;\n");
    fprintf(fp, "      text-align: center;\n");
    fprintf(fp, "      line-height: 20px;\n");
    fprintf(fp, "      font-size: 15px;\n");
    fprintf(fp, "    }\n");
    fprintf(fp, "    .flex-container {\n");
    fprintf(fp, "      padding: 0;\n");
    fprintf(fp, "      margin: 0;\n");
    fprintf(fp, "      display: flex;\n");
    fprintf(fp, "      flex-wrap: wrap;\n");
    fprintf(fp, "      align-items: baseline;\n");
    fprintf(fp, "      justify-content: center;\n");
    fprintf(fp, "    }\n");
    fprintf(fp, "    .flex-container > div {\n");
    fprintf(fp, "      margin: 2px;\n");
    fprintf(fp, "      padding: 2px;\n");
    fprintf(fp, "    }\n");
    fprintf(fp, "</style>\n");
  }
}

void hp2p_result_write_html_plotlyjs(FILE *fp, hp2p_result result)
{
  FILE *fplotly = NULL;
  char ch = '\0';
  int pos = 0;

  if (fp != NULL)
  {
    fprintf(fp, "<script type=\"text/javascript\">window.PlotlyConfig = "
		"{MathJaxConfig: 'local'};</script>\n");
    if (strlen(result.conf->plotlyjs) > 0)
    {
      fplotly = fopen(result.conf->plotlyjs, "r");
      if (fplotly != NULL)
      {
	fprintf(fp, "<script type=\"text/javascript\">\n");
	fseek(fplotly, 0L, SEEK_END); // file pointer at end of file
	pos = ftell(fplotly);
	fseek(fplotly, 0L, SEEK_SET); // file pointer set at start
	while (pos--)
	{
	  ch = fgetc(fplotly); // copying file character by character
	  fputc(ch, fp);
	}
	fprintf(fp, "\n</script>\n");
	fclose(fplotly);
      }
      else
	fprintf(fp,
		"<script "
		"src=\"https://cdn.plot.ly/plotly-%s.min.js\"></script>\n",
		PLOTLY_VERSION);
    }
    else
      fprintf(fp,
	      "<script "
	      "src=\"https://cdn.plot.ly/plotly-%s.min.js\"></script>\n",
	      PLOTLY_VERSION);
  }
}

void hp2p_result_write_html_header(FILE *fp, hp2p_result result)
{
  char date[1024];
  char hour[1024];

  if (fp != NULL)
  {
    hp2p_result_write_html_get_time(date, hour);
    fprintf(fp, "<!DOCTYPE html>\n");
    fprintf(fp, "<html>\n");
    fprintf(fp, "  <head>\n");
    fprintf(fp, "    <meta charset=\"utf-8\" />\n");
    fprintf(fp, "     <title>CEA-HPC - HP2P on %s - %s at %s</title>\n",
	    &(result.mpi_conf->hostlist[0]), date, hour);
    fprintf(fp, "  </head>\n");
    hp2p_result_write_html_css(fp);
    hp2p_result_write_html_plotlyjs(fp, result);
    fprintf(fp, "<body style=\"background-color:rgb(220, 220, 220);\">\n");
    fflush(fp);
  }
}

void hp2p_result_write_html_footer(FILE *fp)
{
  char date[1024];
  char hour[1024];

  if (fp != NULL)
  {
    hp2p_result_write_html_get_time(date, hour);
    fprintf(fp, "<div class=stats-container>\n");
    fprintf(fp, "<div>\n");
    fprintf(fp,
	    "This page was generated by CEA-HPC <a "
	    "href=\"https://github.com/cea-hpc/hp2p\">HP2P</a> benchmark on %s "
	    "at %s.\n",
	    date, hour);
    fprintf(fp, "</div>\n");
    fprintf(fp, "</div>\n");

    fprintf(fp, "</body>\n");
    fprintf(fp, "</html>\n");
    fprintf(fp, "\n");
    fflush(fp);
  }
}

void hp2p_result_write_html_default_stats(FILE *fp, hp2p_result result)
{
  // Convert bytes to Mb
  double m = 1024.0 * 1024.0;

  if (fp != NULL)
  {
    // Main title
    fprintf(fp, "<div class=\"banner\">\n");
    fprintf(fp, "<h2>HP2P results vizualisation</h2>\n");
    fprintf(fp, "</div>\n");

    // Run configuration
    fprintf(fp, "<div class=stats-container>\n");
    fprintf(fp, "<div>\n");
    fprintf(fp, "<h2>Details</h2>\n");
    fprintf(fp, "Number of iterations: %d / %d<br>\n", result.current_iteration,
	    result.conf->nb_shuffle);
    fprintf(fp, "Message size: %d bytes<br>\n", result.conf->msg_size);
    fprintf(fp, "Number of messages per communication: %d<br>\n",
	    result.conf->nb_msg);
    fprintf(fp, "MPI buffer alignment: %d<br>\n", result.conf->align_size);
    fprintf(fp, "Algorithm: %s<br>\n", hp2p_algo[result.conf->build]);
    fprintf(fp, "</div>\n");
    fprintf(fp, "</div>\n");

    // Bandwidth statistics
    fprintf(fp, "<div class=stats-container >\n");
    fprintf(fp, "<div>\n");
    fprintf(fp, "<h2>Bandwidth Statistics</h2>\n");
    fprintf(
	fp, "Minimum Bandwidth: %0.2lf MB/s between %s and %s<br>\n",
	result.min_bw / m,
	&result.mpi_conf->hostlist[MPI_MAX_PROCESSOR_NAME * result.i_min_bw],
	&result.mpi_conf->hostlist[MPI_MAX_PROCESSOR_NAME * result.j_min_bw]);
    fprintf(
	fp, "Maximum Bandwidth: %0.2lf MB/s between %s and %s<br>\n",
	result.max_bw / m,
	&result.mpi_conf->hostlist[MPI_MAX_PROCESSOR_NAME * result.i_max_bw],
	&result.mpi_conf->hostlist[MPI_MAX_PROCESSOR_NAME * result.j_max_bw]);
    fprintf(fp, "Average: %0.2lf MB/s<br>\n", result.avg_bw / m);
    fprintf(fp, "Standard deviation: %0.2lf MB/s<br>\n", result.stdd_bw / m);
    fprintf(fp, "</div>\n");

    // Latency statistics
    fprintf(fp, "<div>\n");
    fprintf(fp, "<h2>Latency Statistics</h2>\n");
    fprintf(
	fp,
	"Minimum Latency: %0.2lf <span>&#181;</span>s between %s and %s<br>\n",
	result.min_time * 1.e6,
	&result.mpi_conf->hostlist[MPI_MAX_PROCESSOR_NAME * result.i_min_time],
	&result.mpi_conf->hostlist[MPI_MAX_PROCESSOR_NAME * result.j_min_time]);
    fprintf(
	fp,
	"Maximum Latency: %0.2lf <span>&#181;</span>s between %s and %s<br>\n",
	result.max_time * 1.e6,
	&result.mpi_conf->hostlist[MPI_MAX_PROCESSOR_NAME * result.i_max_time],
	&result.mpi_conf->hostlist[MPI_MAX_PROCESSOR_NAME * result.j_max_time]);
    fprintf(fp, "Average: %0.2lf <span>&#181;</span>s<br>\n",
	    result.avg_time * 1.e6);
    fprintf(fp, "Standard deviation: %0.2lf <span>&#181;</span>s<br>\n",
	    result.stdd_time * 1.e6);
    fprintf(fp, "</div>\n");

    // Bisection bandwidth
    fprintf(fp, "<div>\n");
    fprintf(fp, "<h2>Bisection bandwidth Statistics</h2>\n");
    fprintf(fp, "Minimum Bisection Bandwidth: %0.2lf MB/s<br>\n",
	    result.min_bsbw / m);
    fprintf(fp, "Maximum Bisection Bandwidth: %0.2lf MB/s<br>\n",
	    result.max_bsbw / m);
    fprintf(fp, "Average: %0.2lf MB/s<br>\n", result.avg_bsbw / m);
    fprintf(fp, "Standard deviation: %0.2lf MB/s<br>\n", result.stdd_bsbw / m);
    fprintf(fp, "</div>\n");

    fprintf(fp, "</div>\n");
  }
}

void hp2p_result_write_html(hp2p_result result)
{
  FILE *fp = NULL;
  char *filename = NULL;
  int i = 0;
  int j = 0;
  double m = 1024.0 * 1024.0;
  filename = (char *)malloc((strlen(result.conf->outname) + 16) * sizeof(char));
  strcpy(filename, result.conf->outname);
  strcat(filename, ".html");
  fp = fopen(filename, "w");
  if (fp != NULL)
  {
    hp2p_result_write_html_header(fp, result);
    hp2p_result_write_html_default_stats(fp, result);

    // Data
    fprintf(fp, "<script type=\"text/javascript\">\n");
    fprintf(fp, "// hostlist start\n");
    fprintf(fp, "var hostlist = \n[");
    for (i = 0; i < result.mpi_conf->nproc; i++)
      fprintf(fp, "    \"%s\", ",
	      &result.mpi_conf->hostlist[MPI_MAX_PROCESSOR_NAME * i]);
    fprintf(fp, "    ]\n;\n");
    fprintf(fp, "// hostlist end\n");
    fprintf(fp, "// msg_size start\n");
    fprintf(fp, "var msg_size = \n");
    fprintf(fp, "    %d\n", result.conf->msg_size);
    fprintf(fp, "    ;\n");
    fprintf(fp, "// msg_size end\n");
    fprintf(fp, "// bandwidth start\n");
    fprintf(fp, "var bandwidth = \n[");
    for (i = 0; i < result.mpi_conf->nproc; i++)
    {
      fprintf(fp, "    [");
      for (j = 0; j < result.mpi_conf->nproc; j++)
	fprintf(fp, " %0.2lf,",
		result.g_bw[i * result.mpi_conf->nproc + j] / m);
      fprintf(fp, " ], ");
    }
    fprintf(fp, "    ]\n;\n");
    fprintf(fp, "// bandwidth end\n");
    fprintf(fp, "      var bw_avg = [];\n");
    fprintf(fp, "      for (const bw_line of bandwidth) {\n");
    fprintf(fp, "	var sum = bw_line.reduce((a, b) => a + b, 0);\n");
    fprintf(fp, "	bw_avg.push(sum/(bw_line.length-1));}\n");
    fprintf(fp, "</script>\n");

    // Bandwidth heatmap
    fprintf(fp, "<div class=flex-container >\n");
    fprintf(
	fp,
	"<div><div id=\"0565c6e2-a43b-4956-84fb-40d2e9ecd5ff\" style=\"height: "
	"800px; width: 80%%;\" class=\"plotly-graph-div\"></div>\n");
    fprintf(fp, "  <script type=\"text/javascript\">\n");
    fprintf(fp, "    window.PLOTLYENV=window.PLOTLYENV || {};\n");
    fprintf(fp, "    window.PLOTLYENV.BASE_URL=\"https://plot.ly\";\n");
    fprintf(fp,
	    "    Plotly.newPlot(\"0565c6e2-a43b-4956-84fb-40d2e9ecd5ff\",\n");
    fprintf(fp, "    [{\"uid\": \"6c733ff6-e3de-4cc9-ae93-e189fb397286\",\n");
    fprintf(fp, "    \"colorscale\": \"Jet\",\n");
    fprintf(fp, "    \"y\": hostlist,\n");
    fprintf(fp, "    \"x\": hostlist,\n");
    fprintf(fp, "    \"z\": bandwidth,\n");
    fprintf(fp, "    \"type\": \"heatmap\"}],\n");
    fprintf(
	fp,
	"    {\"height\": 800, \"width\": 800, \"autosize\": true, \"title\": "
	"{\"text\": \"Bandwidth (MB/s)\"}, \"yaxis\": {\"autorange\": "
	"\"reversed\"}}, {\"plotlyServerURL\": \"https://plot.ly\", "
	"\"linkText\": \"Export to plot.ly\", \"showLink\": false}\n");
    fprintf(fp, "    )\n");
    fprintf(fp, "\n  </script>\n\n");
    fprintf(fp, "</div>\n");

    // Distribution bandwidth
    fprintf(fp, "<div class=flex-container >\n");
    fprintf(fp, "<div><div id=\"1234\" style=\"height: 800px; width: 80%%;\" "
		"class=\"plotly-graph-div\"></div>\n");
    fprintf(fp, "  <script type=\"text/javascript\">\n");
    fprintf(fp, "    window.PLOTLYENV=window.PLOTLYENV || {};\n");
    fprintf(fp, "    window.PLOTLYENV.BASE_URL=\"https://plot.ly\";\n");
    fprintf(fp, "    Plotly.newPlot(\"1234\",\n");
    fprintf(fp, "    [{\"uid\": \"5678\",\n");
    fprintf(fp, "    \"colorscale\": \"Jet\",\n");
    fprintf(fp,
	    "    \"x\": [].concat(...bandwidth).filter(function(number){return "
	    "number > 0.0;}),\n");
    fprintf(fp, "    \"type\": \"histogram\"}],\n");
    fprintf(
	fp,
	"    {\"height\": 800, \"width\": 800, \"autosize\": true, \"title\": "
	"{\"text\": \"Distribution Bandwidth (MB/s)\"}, \"yaxis\": {\"title\": "
	"\"Number of pairs\"}, \"xaxis\": {\"title\": \"Bandwidth (MB/s)\"}}, "
	"{\"plotlyServerURL\": \"https://plot.ly\", \"linkText\": \"Export to "
	"plot.ly\", \"showLink\": false}\n");
    fprintf(fp, "    )\n");
    fprintf(fp, "  </script>\n");
    fprintf(fp, "</div>\n");
    fprintf(fp, "\n");

    // Distribution of average bandwidth per node
    fprintf(fp, "<div class=flex-container >\n");
    fprintf(fp, "<div><div id=\"12345\" style=\"height: 800px; width: 80%%;\" "
		"class=\"plotly-graph-div\"></div>\n");
    fprintf(fp, "  <script type=\"text/javascript\">\n");
    fprintf(fp, "    window.PLOTLYENV=window.PLOTLYENV || {};\n");
    fprintf(fp, "    window.PLOTLYENV.BASE_URL=\"https://plot.ly\";\n");
    fprintf(fp, "    Plotly.newPlot(\"12345\",\n");
    fprintf(fp, "    [{\"uid\": \"56789\",\n");
    fprintf(fp, "    \"colorscale\": \"Jet\",\n");
    fprintf(fp, "    \"x\": bw_avg,\n");
    fprintf(fp, "    \"type\": \"histogram\"}],\n");
    fprintf(
	fp,
	"    {\"height\": 800, \"width\": 800, \"autosize\": true, \"title\": "
	"{\"text\": \"Distribution of average bandwidth per node (MB/s)\"}, "
	"\"yaxis\": {\"title\": \"Number of nodes\"}, \"xaxis\": {\"title\": "
	"\"Bandwidth (MB/s)\"} }, {\"plotlyServerURL\": \"https://plot.ly\", "
	"\"linkText\": \"Export to plot.ly\", \"showLink\": false}\n");
    fprintf(fp, "    )\n");
    fprintf(fp, "  </script>\n");
    fprintf(fp, "</div>\n");
    fprintf(fp, "\n");

    fprintf(fp, "</div>\n");
    fprintf(fp, "</div>\n");
    hp2p_result_write_html_footer(fp);
    fclose(fp);
  }
  free(filename);
}

void hp2p_result_write_monitoring_html(hp2p_result result)
{
  FILE *fp = NULL;
  char *filename = NULL;
  int i = 0;
  int j = 0;
  double m = 1024.0 * 1024.0;
  filename = (char *)malloc((strlen(result.conf->outname) + 32) * sizeof(char));
  strcpy(filename, result.conf->outname);
  strcat(filename, "-monitoring.html");
  fp = fopen(filename, "w");
  if (fp != NULL)
  {
    hp2p_result_write_html_header(fp, result);
    hp2p_result_write_html_default_stats(fp, result);

    // Data
    fprintf(fp, "<script type=\"text/javascript\">\n");
    fprintf(fp, "// msg_size start\n");
    fprintf(fp, "var msg_size = \n");
    fprintf(fp, "    %d\n", result.conf->msg_size);
    fprintf(fp, "    ;\n");
    fprintf(fp, "// msg_size end\n");
    fprintf(fp, "// bisection bandwidth start\n");
    fprintf(fp, "var bisection_bandwidth = \n[");
    for (i = 0; i < result.conf->nb_shuffle; i++)
    {
      fprintf(fp, " %0.2lf,", result.g_bsbw[i] / m);
    }
    fprintf(fp, "    ]\n;\n");
    fprintf(fp, "// bisection bandwidth end\n");
    fprintf(fp, "</script>\n");
    fprintf(fp, "\n");

    // Bisection bandwidth boxplot
    fprintf(fp, "<div class=flex-container >\n");
    fprintf(fp, "<div><div id=\"123456\" style=\"height: 800px; width: 80%%;\" "
		"class=\"plotly-graph-div\"></div>\n");
    fprintf(fp, "  <script type=\"text/javascript\">\n");
    fprintf(fp, "    window.PLOTLYENV=window.PLOTLYENV || {};\n");
    fprintf(fp, "    window.PLOTLYENV.BASE_URL=\"https://plot.ly\";\n");
    fprintf(fp, "    Plotly.newPlot(\"123456\",\n");
    fprintf(fp, "    [{\"uid\": \"5678910\",\n");
    fprintf(fp, "    \"name\": \"\",\n");
    fprintf(fp, "    \"boxmean\": \"sd\",\n");
    fprintf(fp, "    \"y\": bisection_bandwidth,\n");
    fprintf(fp, "    \"type\": \"box\"}],\n");
    fprintf(
	fp,
	"    {\"height\": 800, \"width\": 800, \"autosize\": true, \"title\": "
	"{\"text\": \"Bisection bandwidth (MB/s)\"}, "
	"\"yaxis\": {\"title\": \"Bandwidth (MB/s)\"} }, {\"plotlyServerURL\": "
	"\"https://plot.ly\", "
	"\"linkText\": \"Export to plot.ly\", \"showLink\": false}\n");
    fprintf(fp, "    )\n");
    fprintf(fp, "  </script>\n");
    fprintf(fp, "</div>\n");
    fprintf(fp, "\n");
    fprintf(fp, "</div>\n");
    hp2p_result_write_html_footer(fp);
    fclose(fp);
  }
  free(filename);
}

void hp2p_result_write(hp2p_result result)
{
  if (!strcmp(result.conf->output_mode, "bin"))
    hp2p_result_write_binary(result);
  else
  {
    hp2p_result_write_html(result);
    hp2p_result_write_monitoring_html(result);
  }
}
