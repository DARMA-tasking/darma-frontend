/*
//@HEADER
// ************************************************************************
//
//                          data_block.h
//                         dharma_new
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

#ifndef SRC_ABSTRACT_BACKEND_DATA_BLOCK_H_
#define SRC_ABSTRACT_BACKEND_DATA_BLOCK_H_

namespace dharma_runtime {

namespace abstract {

namespace backend {

// TODO Serializer needs to be changed to a Concept and DataBlock should be templated on Serializer (or something like that)

// Serializer is needed to deal with the problem that the serialization itself is a frontend problem,
// but the storage and caching of the (de)serialized blobs is the responsibility of the runtime (i.e.,
// a given version of the data for a given dependency should be deserialized at most once
// (excepting resourse limitations) per memory space and subsequent calls to get_data() should return
// a pointer to that deserialized buffer.  Conversely, a given version of the deserailzed data for a
// given dependency, if acquired and/or updated in deserialized form, should be serialized at most once
// per memory space (excepting resourse limitations) and only if it needs to be communicated to a
// different memory space).  Thus:
// TODO Rethink the relationship to and specification of Serializer to meet the above requirements

class DataBlock
{
  public:

    //template <typename Serializer>
    //DataBlock(
    //  void* data,
    //  Serializer ser
    //) =0;

    //template <typename Serializer, typename Allocator>
    //DataBlock(
    //  void* data,
    //  Serializer ser,
    //  Allocator alloc
    //) =0;

    void set_serialization_manager(
      abstract::frontend::SerializationManager ser
    ) =0;

    // Gets the *deserialized* version of the data
    // If a null Serializer is given, the data may be
    // assumed to be contiguous and not need deserialization
    virtual void*
    get_data() =0;

    // const version of the above (does it matter?)
    virtual const void*
    get_data() const =0;

    // acquire/manage data created elsewhere
    // (in *deserialized* form, if applicable)
    // Note that the serializer is required because
    // the runtime still needs a way of determining
    // the size of a piece of data, even if it is
    // contiguous
    virtual void
    acquire_data(void* data) =0;

    virtual void
    allocate_data() =0;

    // TODO eventually: allocate_data() and/or allocator pattern from STL

    virtual void
    wait_read_only() =0;

    virtual void
    wait_read_write() =0;

    virtual void
    wait_write_only() =0;

    virtual ~DataBlock() { }

};


} // end namespace backend

} // end namespace abstract

} // end namespace dharma_runtime



#endif /* SRC_ABSTRACT_BACKEND_DATA_BLOCK_H_ */
