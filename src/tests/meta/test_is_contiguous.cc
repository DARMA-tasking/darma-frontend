/*
//@HEADER
// ************************************************************************
//
//                      is_contiguous.cpp
//                         DARMA
//              Copyright (C) 2017 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#include <vector>
#include <string>
#include <map>

#include <darma/impl/meta/is_contiguous.h>

using namespace darma::meta;

static_assert(
  is_contiguous_iterator<typename std::vector<long>::iterator>::value,
  "std::vector iterator should be contiguous"
);

static_assert(
  is_contiguous_iterator<typename std::vector<long>::const_iterator>::value,
  "std::vector const_iterator should be contiguous"
);

// Doesn't work with ICPC
//static_assert(
//  is_contiguous_iterator<typename std::string::iterator>::value,
//  "std::string iterator should be contiguous"
//);

static_assert(
  is_contiguous_iterator<typename std::vector<long>::const_iterator>::value,
  "std::vector const_iterator should be contiguous"
);

static_assert(
  not is_contiguous_iterator<typename std::map<int, int>::iterator>::value,
  "std::map iterators should not be contiguous"
);

static_assert(
  is_contiguous_iterator<int*>::value,
  "int* should be contiguous"
);
