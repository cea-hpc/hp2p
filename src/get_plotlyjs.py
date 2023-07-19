#!/usr/bin/env python3
# Copyright (C) 2010-2023 CEA/DAM
# Copyright (C) 2010-2023 Laurent Nguyen <laurent.nguyen@cea.fr>
#
# This file is part of HP2P.
#
# This software is governed by the CeCILL-C license under French law and
# abiding by the rules of distribution of free software.  You can  use,
# modify and/ or redistribute the software under the terms of the CeCILL-C
# license as circulated by CEA, CNRS and INRIA at the following URL
# "http://www.cecill.info".
"""Script to get path to plotly.min.js
"""

import plotly


print(str(plotly.__path__[0])+'/package_data/plotly.min.js')
