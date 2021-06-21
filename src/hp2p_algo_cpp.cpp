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
 * \file      hp2p_algo_cpp.cpp
 * \author    Laurent Nguyen <laurent.nguyen@cea.fr>
 * \version   4.0
 * \date      21 June 2021
 * \brief     HP2P Benchmark
 *
 * \details   No
 */

#include <vector>
#include <algorithm>

extern "C"
{
  void hp2p_algo_random(int *v, int size)
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
  };
}
