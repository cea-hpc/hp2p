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
 * \file      hp2p.c
 * \author    Laurent Nguyen <laurent.nguyen@cea.fr>
 * \version   4.0
 * \date      21 June 2021
 * \brief     HP2P Benchmark
 *
 * \details   No
 */

#include "hp2p.h"

int main(int argc, char *argv[])
{
  hp2p_config conf;
  hp2p_util_set_default_config(&conf);
  hp2p_util_display_config(conf);
  hp2p_util_display_help(argv[0]);

  hp2p_util_free_config(&conf);
  return EXIT_SUCCESS;
};



