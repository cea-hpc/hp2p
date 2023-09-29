#!/usr/bin/env bash
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

##
## file      hp2p_clang-format.sh
## author    Laurent Nguyen <laurent.nguyen@cea.fr>
## version   4.1
## date      September 29 2023
## brief     HP2P Benchmark
##
## details   This script parse all C/C++ files to format them
##

FORMAT_COMMAND=clang-format

which ${FORMAT_COMMAND} >/dev/null 2>&1
ret=$?

if [[ ${ret} -ne 0 ]]
then
    echo "clang-format command not found..."
    exit 0
fi

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
echo "Script directory: $SCRIPT_DIR"

for f in $(ls ${SCRIPT_DIR}/../src/*.h ${SCRIPT_DIR}/../src/*.c ${SCRIPT_DIR}/../src/*.cpp)
do
    echo "Processing ${f}"
    ${FORMAT_COMMAND} ${f} > ${f}.new
    diff ${f} ${f}.new >/dev/null 2>&1
    ret=$?
    if [[ ${ret} -ne 0 ]]
    then
	mv ${f}.new ${f}
    else
	rm -f ${f}.new
    fi
done

