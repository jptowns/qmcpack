//////////////////////////////////////////////////////////////////////////////////////
//// This file is distributed under the University of Illinois/NCSA Open Source License.
//// See LICENSE file in top directory for details.
////
//// Copyright (c) 2019 QMCPACK developers.
////
//// File developed by: Ye Luo, yeluo@anl.gov, Argonne National Laboratory
////
//// File created by: Ye Luo, yeluo@anl.gov, Argonne National Laboratory
////////////////////////////////////////////////////////////////////////////////////////


#ifndef QMCPLUSPLUS_CUDA_ERROR_H
#define QMCPLUSPLUS_CUDA_ERROR_H

#include <iostream>
#include <string>
#include <stdexcept>
#include <cuda_runtime_api.h>

#define cudaErrorCheck(ans, cause) { cudaAssert((ans), cause, __FILE__, __LINE__); }

inline void cudaAssert(cudaError_t code, const std::string& cause, const char *file, int line, bool abort=true)
{
  if (code != cudaSuccess) 
  {
    std::ostringstream err;
    err << "cudaAssert: " << cudaGetErrorString(code)
        << ", file " << file << ", line " << line << std::endl
        << cause << std::endl;
    std::cerr << err.str();
    //if (abort) exit(code);
    throw std::runtime_error(cause);
  }
}

#endif
