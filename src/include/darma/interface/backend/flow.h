/*
//@HEADER
// ************************************************************************
//
//                      flow.h
//                         DARMA
//              Copyright (C) 2016 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
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
// Questions? Contact David S. Hollman (dshollm@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#ifndef DARMA_INTERFACE_BACKEND_FLOW_H
#define DARMA_INTERFACE_BACKEND_FLOW_H

namespace darma_runtime {
namespace abstract {
namespace backend {

/** @brief A backend-allocated object representing the input/output state of a Handle at the beginning/end of a task.
 *
 *  When executing tasks, data "flows" from one task to the next.
 *  A precursor task produces data that will be consumed by a successor task.
 *  Each task carries a unique Use variable for each Handle it uses. Each Use has an input flow and output flow.
 *  This is true even of a read-only Use, with the output indicating the release of anti-dependence.
 *  An equivalence relationship between two Flows `a` and `b` is indicated by allocating the Flow `a` with
 *  a call to `Runtime::make_same_flow(b)` or vice versa.  Equivalence must be defined within the backend.
 *  The translation layer will never make an equivalence test itself.
 *
 *  The life-cycle of a Flow consists of 4 strictly ordered phases.  For some Flow instance flw,
 *
 *  + Creation -- `&flw` is a pointer returned by any of `make_initial_flow()`,
 *      `make_fetching_flow()`, `make_null_flow()`, `make_same_flow()`, `make_forwarding_flow()`, or `make_next_flow()`
 *  + Registration -- Each flow is owned by a Use as either input or output. Each Use will be registered
 *      through `Runtime::register_use()` before being used in a task or publication.  All flows have exactly
 *      one `Use` association in their lifetime; that is, `&flw` is either a return value of `Use::get_in_flow()`
 *      or `Use::get_out_flow()` for some `Use` object that is an argument to `register_use()` at
 *      some time after `flw` was created but before it is released.  To ensure this
 *      strict ordering of the `Flow` life-cycle, the runtime must enforce
 *      atomicity among `register_use(&u)`, `make_next_flow(&flw, ...)`/`make_forwarding_flow(&flw, ...)`/`make_same_flow(&flw, ...)`,
 *      `release_use(&u)` for any `Flow` `flw` that could be returned by `u.get_in_flow()` or
 *      `u.get_out_flow()` for some `Use` `u`.
 *  + Release -- Each `Flow` is owned by a `Use` as either input or output. `Flow`s are released through
 *       to `Runtime::release_use()` on the owning `Use`.  The `Flow` will never be used directly (or indirectly)
 *       by the translation after calling `release_use()`.
 *  + At most one call to `runtime.make_next_flow(&flw, ...)` can happen anytime after creation, but before release.
 *       Any number of calls to `runtime.make_same_flow(&flw, ...)` can happen anytime after creation, but before release.
 *       At most one call to `runtime.make_forwarding_flow(&flw, ...)` can be made in the lifetime of a `Flow` (and
 *       this call does not preclude a `make_next_flow(&flw, ...)` call also being made)
 *
 *  Two Flow objects, `a` and `b`, are considered to consume or produce the same version of the same data
 *  if `a` was constructed using `make_same(b)` or if `b` was constructed using `make_same(a)`.
 *  The flow returned by `make_same(a)`, however, is a different object and is therefore
 *  has an independent life cycle and is independently modifiable by the backend.
 */
class Flow {
  public:

};


} // end namespace backend
} // end namespace abstract
} // end namespace darma_runtime

#endif //DARMA_INTERFACE_BACKEND_FLOW_H
