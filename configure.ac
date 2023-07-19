#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([HP2P],[4.0],[laurent.nguyen@cea.fr])
AM_INIT_AUTOMAKE([-Wall])
AC_CONFIG_SRCDIR([src/])

# use the C++ compiler for the following checks
AC_LANG([C++])

# Checks for programs.
AC_PROG_CC([mpicc])
AC_PROG_CXX([mpic++ mpicxx])
AC_LANG_PUSH(C++)
AC_LANG_POP(C++)
AC_LANG_PUSH(C)
AC_LANG_POP(C)
AC_PROG_RANLIB

# Checks for libraries.

# Checks for header files.
m4_warn([obsolete],
[The preprocessor macro `STDC_HEADERS' is obsolete.
  Except in unusual embedded environments, you can safely include all
  ISO C90 headers unconditionally.])dnl
# Autoupdate added the next two lines to ensure that your configure
# script's behavior did not change.  They are probably safe to remove.

AC_CHECK_HEADERS([string])
AC_CHECK_HEADERS([vector algorithm list])
AC_CHECK_HEADERS([stdio.h stdlib.h string.h])
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
#AC_CHECK_FUNCS([MPI_File_open MPI_File_close])
#check if signal has been asked
AC_ARG_ENABLE([signal],
              [AS_HELP_STRING([--enable-signal],
                              [enable signal handling in output management (default is no). SIGUSR1 makes the process generate an output. SIGTERM makes the process generate an output and exit.])
              ],
              [],
              [enable_signal=no])

# Checks for CUDA
AC_ARG_ENABLE([cuda],
              [AS_HELP_STRING([--enable-cuda],
                              [enable cuda benchmarks (default is no).])
              ],
              [],
              [enable_cuda=no])

AC_ARG_WITH([cuda],
            [AS_HELP_STRING([--with-cuda=@<:@CUDA installation path@:>@],
                            [Provide path to CUDA installation])
            ],
            [AS_CASE([$with_cuda],
                     [yes|no], [],
                     [CPPFLAGS="-I$with_cuda/include $CPPFLAGS"
                      LDFLAGS="-L$with_cuda/lib64 -Wl,-rpath=$with_cuda/lib64 -L$with_cuda/lib -Wl,-rpath=$with_cuda/lib $LDFLAGS"])
            ])
AS_CASE([$enable_cuda],
	[yes], [build_cuda=yes])

-# Checks for ROCm
AC_ARG_ENABLE([rocm],
              [AS_HELP_STRING([--enable-rocm],
                                         [Enable ROCm benchmarks])
              ],
              [],
              [enable_rocm=no])

AC_ARG_WITH([rocm],
            [AS_HELP_STRING([--with-rocm=@<:@ROCm installation path@:>@],
                            [Provide path to ROCm installation])
            ],
            [AS_CASE([$with_rocm],
                     [yes|no], [],
                     [CPPFLAGS="-I$with_rocm/include $CPPFLAGS"
                      LDFLAGS="-L$with_rocm/lib64 -Wl,-rpath=$with_rocm/lib64 -L$with_rocm/lib -Wl,-rpath=$with_rocm/lib -lamdhip64 $LDFLAGS"])
            ])

AS_CASE([$enable_signal],
	[yes], [CPPFLAGS="-D_HP2P_SIGNAL $CPPFLAGS"])

AS_IF([test "x$build_cuda" = xyes], [
       AC_SEARCH_LIBS([cuPointerGetAttribute], [cuda], [],
                      [AC_MSG_ERROR([cannot link with -lcuda])])
       AC_SEARCH_LIBS([cudaFree], [cudart], [],
                      [AC_MSG_ERROR([cannot link with -lcudart])])
       AC_CHECK_HEADERS([cuda.h], [],
                        [AC_MSG_ERROR([cannot include cuda.h])])
       AC_DEFINE([_ENABLE_CUDA_], [1], [Enable CUDA])
       ])
AS_IF([test "x$enable_rocm" = xyes], [
       AC_DEFINE([__HIP_PLATFORM_HCC__])
       AC_CHECK_HEADERS([hip/hip_runtime_api.h], [],
                        [AC_MSG_ERROR([cannot include hip/hip_runtime_api.h])])
       AC_SEARCH_LIBS([hipFree], [amdhip64], [],
                      [AC_MSG_ERROR([cannot link with -lamdhip64])])
       AC_DEFINE([_ENABLE_ROCM_], [1], [Enable ROCm])
       ])

AC_CONFIG_FILES([Makefile
	src/Makefile])
AC_OUTPUT
