/*
//@HEADER
// ************************************************************************
// 
//                        Kokkos v. 2.0
//              Copyright (2014) Sandia Corporation
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
// Questions? Contact  H. Carter Edwards (hcedwar@sandia.gov)
// 
// ************************************************************************
//@HEADER
*/

#ifndef KOKKOS_IMPL_OPENMP_TASK_HPP
#define KOKKOS_IMPL_OPENMP_TASK_HPP

#if defined( KOKKOS_ENABLE_TASKPOLICY )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace Kokkos {
namespace Impl {

template<>
class TaskQueueSpecialization< Kokkos::OpenMP >
{
public:

  using execution_space = Kokkos::OpenMP ;
  using queue_type      = Kokkos::Impl::TaskQueue< execution_space > ;
  using task_base_type  = Kokkos::Impl::TaskBase< execution_space , void , void > ;

  // Must specify memory space
  using memory_space = Kokkos::HostSpace ;

  static
  void iff_single_thread_recursive_execute( queue_type * const );

  // Must provide task queue execution function
  static void execute( queue_type * const );

  // Must provide mechanism to set function pointer in
  // execution space from the host process.
  template< typename FunctorType >
  static
  void proc_set_apply( task_base_type::function_type * ptr )
    {
      using TaskType = TaskBase< Kokkos::OpenMP
                               , typename FunctorType::value_type
                               , FunctorType
                               > ;
       *ptr = TaskType::apply ;
    }
};

extern template class TaskQueue< Kokkos::OpenMP > ;

//----------------------------------------------------------------------------

template<>
class TaskExec< Kokkos::OpenMP >
{
private:

  TaskExec( TaskExec && ) = delete ;
  TaskExec( TaskExec const & ) = delete ;
  TaskExec & operator = ( TaskExec && ) = delete ;
  TaskExec & operator = ( TaskExec const & ) = delete ;


  using PoolExec = Kokkos::Impl::OpenMPexec ;

  friend class Kokkos::Impl::TaskQueue< Kokkos::OpenMP > ;
  friend class Kokkos::Impl::TaskQueueSpecialization< Kokkos::OpenMP > ;

  PoolExec * const m_self_exec ;  ///< This thread's thread pool data structure 
  PoolExec * const m_team_exec ;  ///< Team thread's thread pool data structure
  int64_t    m_sync_mask ;
  int64_t    m_sync_value ;
  int        m_sync_step ;
  int        m_group_rank ; ///< Which "team" subset of thread pool
  int        m_team_rank ;  ///< Which thread within a team
  int        m_team_size ;

  TaskExec();
  TaskExec( PoolExec & arg_exec , int arg_team_size );

public:

#if defined( KOKKOS_ACTIVE_EXECUTION_MEMORY_SPACE_HOST )
  void * team_shared() const
    { return m_team_exec ? m_team_exec->scratch_thread() : (void*) 0 ; }

  int team_shared_size() const
    { return m_team_exec ? m_team_exec->scratch_thread_size() : 0 ; }

  /**\brief  Whole team enters this function call
   *         before any teeam member returns from
   *         this function call.
   */
  void team_barrier();
#else
  KOKKOS_INLINE_FUNCTION void team_barrier() {}
  KOKKOS_INLINE_FUNCTION void * team_shared() const { return 0 ; }
  KOKKOS_INLINE_FUNCTION int team_shared_size() const { return 0 ; }
#endif

  KOKKOS_INLINE_FUNCTION
  int team_rank() const { return m_team_rank ; }

  KOKKOS_INLINE_FUNCTION
  int team_size() const { return m_team_size ; }
};

template<typename iType>
struct TeamThreadRangeBoundariesStruct<iType, TaskExec< Kokkos::OpenMP > >
{
  typedef iType index_type;
  const iType begin ;
  const iType end ;
  //enum {increment = 1}; //TODO changed
  const iType increment ;
  TaskExec< Kokkos::OpenMP > & thread;

  KOKKOS_INLINE_FUNCTION
  TeamThreadRangeBoundariesStruct
    ( TaskExec< Kokkos::OpenMP > & arg_thread, const iType& arg_count)
    : begin(0)
    , end(arg_count)
    , increment( arg_thread.team_size() ) //TODO changed
    , thread(arg_thread)
    {}

  KOKKOS_INLINE_FUNCTION
  TeamThreadRangeBoundariesStruct
    ( TaskExec< Kokkos::OpenMP > & arg_thread
    , const iType & arg_begin
    , const iType & arg_end
    )
    //: begin( arg_begin ) //TODO changed
    : begin( arg_begin + arg_thread.team_rank() )
    , end(   arg_end)
    , increment( arg_thread.team_size() ) //TODO changed
    , thread( arg_thread )
    {}
};

}} /* namespace Kokkos::Impl */

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace Kokkos {

template<typename iType>
KOKKOS_INLINE_FUNCTION
Impl::TeamThreadRangeBoundariesStruct<iType,Impl::TaskExec< Kokkos::OpenMP > >
TeamThreadRange( Impl::TaskExec< Kokkos::OpenMP > & thread
               , const iType & count )
{
  return Impl::TeamThreadRangeBoundariesStruct<iType,Impl::TaskExec< Kokkos::OpenMP > >(thread,count);
}

template<typename iType>
KOKKOS_INLINE_FUNCTION
Impl::TeamThreadRangeBoundariesStruct<iType,Impl:: TaskExec< Kokkos::OpenMP > >
TeamThreadRange( Impl:: TaskExec< Kokkos::OpenMP > & thread, const iType & begin , const iType & end )
{
  return Impl::TeamThreadRangeBoundariesStruct<iType,Impl:: TaskExec< Kokkos::OpenMP > >(thread,begin,end);
}

  /** \brief  Inter-thread parallel_for. Executes lambda(iType i) for each i=0..N-1.
   *
   * The range i=0..N-1 is mapped to all threads of the the calling thread team.
   * This functionality requires C++11 support.*/
template<typename iType, class Lambda>
KOKKOS_INLINE_FUNCTION
void parallel_for
  ( const Impl::TeamThreadRangeBoundariesStruct<iType,Impl:: TaskExec< Kokkos::OpenMP > >& loop_boundaries
  , const Lambda& lambda
  )
{
  for( iType i = loop_boundaries.begin; i < loop_boundaries.end; i+=loop_boundaries.increment) {
    lambda(i);
  }
}

//TODO const issue
template<typename iType, class Lambda, typename ValueType>
KOKKOS_INLINE_FUNCTION
void parallel_reduce
  ( const Impl::TeamThreadRangeBoundariesStruct<iType,Impl:: TaskExec< Kokkos::OpenMP > >& loop_boundaries
  , const Lambda& lambda
  , ValueType& initialized_result)
{
  int team_rank = loop_boundaries.thread.team_rank(); // member num within the team
  ValueType result = initialized_result;

  for( iType i = loop_boundaries.begin; i < loop_boundaries.end; i+=loop_boundaries.increment) {
    lambda(i, result);
    //printf("0-- tr: %d of %d, ir: %ld, r: %ld, i:[%d]\n", team_rank, loop_boundaries.thread.team_size(),
    //        initialized_result, result, i);
  }
  //printf("1-- tr: %d of %d, ir: %ld, r: %ld\n", team_rank, loop_boundaries.thread.team_size(),
  //        initialized_result, result);

  if ( 1 < loop_boundaries.thread.team_size() ) {

    ValueType *shared = (ValueType*) loop_boundaries.thread.team_shared();

    loop_boundaries.thread.team_barrier();
    shared[team_rank] = result;

    loop_boundaries.thread.team_barrier();

    // reduce across threads to thread 0
    if (team_rank == 0) {
      for (int i = 1; i < loop_boundaries.thread.team_size(); i++) {
        shared[0] += shared[i];
      }
    }

    loop_boundaries.thread.team_barrier();

    // broadcast result
    initialized_result = shared[0];
    //printf("2-- tr: %d of %d, ir: %ld, r: %ld\n", team_rank, loop_boundaries.thread.team_size(),
    //        initialized_result, result);
  }
  else {
    initialized_result = result ;
  }
}

// placeholder for future function
template< typename iType, class Lambda, typename ValueType, class JoinType >
KOKKOS_INLINE_FUNCTION
void parallel_reduce
  (const Impl::TeamThreadRangeBoundariesStruct<iType,Impl::TaskExec< Kokkos::OpenMP > >& loop_boundaries,
   const Lambda & lambda,
   const JoinType & join,
   ValueType& initialized_result)
{
  int team_rank = loop_boundaries.thread.team_rank(); // member num within the team
  ValueType result = initialized_result;

  for( iType i = loop_boundaries.begin; i < loop_boundaries.end; i+=loop_boundaries.increment) {
    lambda(i, result);
    //printf("0-- tr: %d of %d, ir: %ld, r: %ld, i:[%d]\n", team_rank, loop_boundaries.thread.team_size(),
    //        initialized_result, result, i);
  }
  //printf("1-- tr: %d of %d, ir: %ld, r: %ld\n", team_rank, loop_boundaries.thread.team_size(),
  //        initialized_result, result);

  if ( 1 < loop_boundaries.thread.team_size() ) {
    ValueType *shared = (ValueType*) loop_boundaries.thread.team_shared();

    loop_boundaries.thread.team_barrier();
    shared[team_rank] = result;

    loop_boundaries.thread.team_barrier();

    // reduce across threads to thread 0
    if (team_rank == 0) {
      for (int i = 1; i < loop_boundaries.thread.team_size(); i++) {
        join(shared[0], shared[i]);
      }
    }

    loop_boundaries.thread.team_barrier();

    // broadcast result
    initialized_result = shared[0];
    //printf("2-- tr: %d of %d, ir: %ld, r: %ld\n", team_rank, loop_boundaries.thread.team_size(),
    //        initialized_result, result);
  }
  else {
    initialized_result = result ;
  }
}

// placeholder for future function
template< typename iType, class Lambda, typename ValueType >
KOKKOS_INLINE_FUNCTION
void parallel_reduce
  (const Impl::ThreadVectorRangeBoundariesStruct<iType,Impl::TaskExec< Kokkos::OpenMP > >& loop_boundaries,
   const Lambda & lambda,
   ValueType& initialized_result)
{
}
// placeholder for future function
template< typename iType, class Lambda, typename ValueType, class JoinType >
KOKKOS_INLINE_FUNCTION
void parallel_reduce
  (const Impl::ThreadVectorRangeBoundariesStruct<iType,Impl::TaskExec< Kokkos::OpenMP > >& loop_boundaries,
   const Lambda & lambda,
   const JoinType & join,
   ValueType& initialized_result)
{
}

// placeholder for future function
template< typename iType, class Lambda, typename ValueType >
KOKKOS_INLINE_FUNCTION
void parallel_scan_excl
  (const Impl::TeamThreadRangeBoundariesStruct<iType,Impl::TaskExec< Kokkos::OpenMP > >& loop_boundaries,
   const Lambda & lambda,
   ValueType& initialized_result)
{
}
// placeholder for future function
template< typename iType, class Lambda, typename ValueType >
KOKKOS_INLINE_FUNCTION
void parallel_scan_incl
  (const Impl::TeamThreadRangeBoundariesStruct<iType,Impl::TaskExec< Kokkos::OpenMP > >& loop_boundaries,
   const Lambda & lambda,
   ValueType& initialized_result)
{
}
// placeholder for future function
template< typename iType, class Lambda, typename ValueType >
KOKKOS_INLINE_FUNCTION
void parallel_scan_excl
  (const Impl::ThreadVectorRangeBoundariesStruct<iType,Impl::TaskExec< Kokkos::OpenMP > >& loop_boundaries,
   const Lambda & lambda,
   ValueType& initialized_result)
{
}
// placeholder for future function
template< typename iType, class Lambda, typename ValueType >
KOKKOS_INLINE_FUNCTION
void parallel_scan_incl
  (const Impl::ThreadVectorRangeBoundariesStruct<iType,Impl::TaskExec< Kokkos::OpenMP > >& loop_boundaries,
   const Lambda & lambda,
   ValueType& initialized_result)
{
}


} /* namespace Kokkos */

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#endif /* #if defined( KOKKOS_ENABLE_TASKPOLICY ) */
#endif /* #ifndef KOKKOS_IMPL_OPENMP_TASK_HPP */
