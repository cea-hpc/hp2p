#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([HP2P], [3.1], [laurent.nguyen@cea.fr])
AM_INIT_AUTOMAKE([-Wall])
AC_CONFIG_SRCDIR([src/hp2p.cpp])
AC_CONFIG_HEADERS([config.h])

# use the C++ compiler for the following checks
AC_LANG([C++])

# use Python
AM_PATH_PYTHON([2.6])

# Checks for programs.
AC_PROG_CXX([mpic++ mpicxx])
AC_LANG_PUSH(C++)
AC_LANG_POP(C++)
AC_PROG_RANLIB

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([cstdlib cstring])
AC_CHECK_HEADERS([string])
AC_CHECK_HEADERS([vector algorithm list])
AC_CHECK_HEADERS([iostream fstream sstream])
AC_CHECK_HEADERS([sys/time.h unistd.h])
AC_CHECK_HEADERS([mpi.h])

# distribute additional compiler and linker flags among Makefiles
# --> set and change these variables instead of CXXFLAGS or LDFLAGS (for user only)
AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_LDFLAGS])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([gettimeofday])
AC_CHECK_FUNCS([getopt])

# Checks for MPI library functions.
AC_CHECK_FUNCS([MPI_Init MPI_Finalize])
AC_CHECK_FUNCS([MPI_File_open MPI_File_close])


AC_CONFIG_FILES([Makefile
	src/Makefile])
AC_OUTPUT
