// Copyright (c) 2024 Dr. Colin Hirsch
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

#include <cstddef>
#include <iostream>

namespace mcp::test
{
   std::size_t failed = 0;

}  // namespace mcp::test

#define MCP_TEST_ASSERT( ... )                   \
   do {                                          \
      if( !( __VA_ARGS__ ) ) {                   \
         std::cerr << "mcp: unit test assert [ " \
                   << ( #__VA_ARGS__ )           \
                   << " ] failed in line [ "     \
                   << __LINE__                   \
                   << " ] file [ "               \
                   << __FILE__ << " ]"           \
                   << std::endl;                 \
         ++mcp::test::failed;                    \
      }                                          \
   } while( false )

#define MCP_TEST_THROWS( ... )                      \
   do {                                             \
      try {                                         \
         __VA_ARGS__;                               \
         std::cerr << "mcp: unit test [ "           \
                   << ( #__VA_ARGS__ )              \
                   << " ] did not throw in line [ " \
                   << __LINE__                      \
                   << " ] file [ "                  \
                   << __FILE__ << " ]"              \
                   << std::endl;                    \
         ++mcp::test::failed;                       \
      }                                             \
      catch( ... ) {                                \
      }                                             \
   } while( false )

#include "mini_coro_plus.hpp"
#include "mini_coro_plus.ipp"

namespace mcp::test
{
   struct cycle
   {
      explicit cycle( std::size_t& c ) noexcept
         : m_cycle( c )
      {
         MCP_TEST_ASSERT( m_cycle++ == 0 );
      }

      cycle( cycle&& ) = delete;
      cycle( const cycle& ) = delete;

      ~cycle()
      {
         MCP_TEST_ASSERT( m_cycle++ == 1 );
      }

      void operator=( cycle&& ) = delete;
      void operator=( const cycle& ) = delete;

   private:
      std::size_t& m_cycle;
   };

   void tests()
   {
      coroutine( [](){} );
      {
         coroutine coro( [](){} );
         MCP_TEST_ASSERT( coro.state() == state::STARTING );
         coro.abort();
         MCP_TEST_ASSERT( coro.state() == state::COMPLETED );
         MCP_TEST_THROWS( coro.yield() );
         MCP_TEST_THROWS( coro.resume() );
         coro.abort();
         MCP_TEST_ASSERT( coro.state() == state::COMPLETED );
         MCP_TEST_THROWS( coro.yield() );
         MCP_TEST_THROWS( coro.resume() );
      } {
         coroutine coro( [](){} );
         MCP_TEST_ASSERT( coro.state() == state::STARTING );
         coro.resume();
         MCP_TEST_ASSERT( coro.state() == state::COMPLETED );
      } {
         bool b = false;
         coroutine coro( [ & ](){
            MCP_TEST_ASSERT( !b );
            b = true;
         } );
         MCP_TEST_ASSERT( coro.state() == state::STARTING );
         MCP_TEST_ASSERT( !b );
         coro.resume();
         MCP_TEST_ASSERT( coro.state() == state::COMPLETED );
         MCP_TEST_ASSERT( b );
      } {
         std::size_t c = 0;
         std::size_t r = 0;
         coroutine coro( [ & ](){
            for( std::size_t i = 0; i < 1000; ++i ) {
               MCP_TEST_ASSERT( c == i );
               ++c;
               yield_running();
            }
         } );
         for( coro.resume(); coro.state() != state::COMPLETED; coro.resume() ) {
            ++r;
         }
         MCP_TEST_ASSERT( r == 1000 );
         MCP_TEST_ASSERT( c == 1000 );
      } {
         coroutine coro( [](){
            coroutine( [](){} );
         } );
         coro.resume();
         MCP_TEST_ASSERT( coro.state() == state::COMPLETED );
      } {
         std::size_t t = 0;
         MCP_TEST_ASSERT( t == 0 );
         coroutine outer( [ & ](){
            MCP_TEST_ASSERT( t++ == 1 );
            coroutine inner( [ & ](){
               MCP_TEST_ASSERT( t++ == 3 );
               inner.yield();
               MCP_TEST_THROWS( inner.abort() );
               MCP_TEST_THROWS( inner.resume() );
               MCP_TEST_ASSERT( t++ == 5 );
               inner.yield();
               MCP_TEST_ASSERT( t++ == 7 );
            } );
            MCP_TEST_ASSERT( inner.state() == state::STARTING );
            MCP_TEST_ASSERT( t++ == 2 );
            inner.resume();
            MCP_TEST_THROWS( outer.abort() );
            MCP_TEST_THROWS( outer.resume() );
            MCP_TEST_ASSERT( t++ == 4 );
            inner.resume();
            MCP_TEST_ASSERT( t++ == 6 );
            inner.resume();
            MCP_TEST_ASSERT( t++ == 8 );
            MCP_TEST_ASSERT( inner.state() == state::COMPLETED );
         } );
         MCP_TEST_ASSERT( outer.state() == state::STARTING );
         MCP_TEST_ASSERT( t++ == 0 );
         outer.resume();
         MCP_TEST_ASSERT( t++ == 9 );
         MCP_TEST_ASSERT( outer.state() == state::COMPLETED );
      } {
         std::size_t t = 0;
         MCP_TEST_ASSERT( t == 0 );
         coroutine outer( [ & ](){
            MCP_TEST_ASSERT( t++ == 1 );
            coroutine inner( [ & ](){
               MCP_TEST_ASSERT( t++ == 3 );
               inner.yield();
               MCP_TEST_ASSERT( t++ == 5 );
               inner.yield();
               MCP_TEST_ASSERT( t++ == 8 );
            } );
            MCP_TEST_ASSERT( inner.state() == state::STARTING );
            MCP_TEST_ASSERT( t++ == 2 );
            inner.resume();
            MCP_TEST_ASSERT( t++ == 4 );
            inner.resume();
            outer.yield();
            MCP_TEST_ASSERT( t++ == 7 );
            inner.resume();
            outer.yield();
            MCP_TEST_ASSERT( t++ == 10 );
            MCP_TEST_ASSERT( inner.state() == state::COMPLETED );
         } );
         MCP_TEST_ASSERT( outer.state() == state::STARTING );
         MCP_TEST_ASSERT( t++ == 0 );
         outer.resume();
         MCP_TEST_ASSERT( t++ == 6 );
         outer.resume();
         MCP_TEST_ASSERT( t++ == 9 );
         outer.resume();
         MCP_TEST_ASSERT( t++ == 11 );
         MCP_TEST_ASSERT( outer.state() == state::COMPLETED );
      } {
         struct foo {};
         coroutine coro( [](){
            throw foo();
         } );
         try {
            coro.resume();
            MCP_TEST_ASSERT( false );
         }
         catch( const foo& ) {
         }
         MCP_TEST_ASSERT( coro.state() == state::COMPLETED );
      } {
         struct foo {};
         coroutine outer( [](){
            coroutine inner( [](){
               throw foo();
            } );
            inner.resume();
         } );
         try {
            outer.resume();
            MCP_TEST_ASSERT( false );
         }
         catch( const foo& ) {
         }
         MCP_TEST_ASSERT( outer.state() == state::COMPLETED );
      } {
         std::size_t c = 0;
         coroutine coro( [ & ](){
            cycle y( c );
            coro.yield();
            c == 42;
         } );
         MCP_TEST_ASSERT( c == 0 );
         coro.resume();
         MCP_TEST_ASSERT( c == 1 );
         coro.abort();
         MCP_TEST_ASSERT( c == 2 );
         MCP_TEST_ASSERT( coro.state() == state::COMPLETED );
      }
   }

}  // namespace mcp::test

int main()
{
   mcp::test::tests();

   if( mcp::test::failed > 0 ) {
      std::cerr << "mcp: failed testcases: " << mcp::test::failed << std::endl;
      return 1;
   }
   std::cout << "mcp: all testcases succeeded" << std::endl;
   return 0;
}
