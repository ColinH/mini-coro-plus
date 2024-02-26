// Copyright (c) 2024 Dr. Colin Hirsch
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

#include <cassert>
#include <iostream>

#include "mini_coro_plus.hpp"
#include "mini_coro_plus.ipp"

int sum;

void function()
{
   mcp::control().yield();
   char a[ 128 ];
   mcp::control().yield();
   for( std::size_t i = 0; i < sizeof( a ); ++i ) {
      sum += a[ i ];
   }
   mcp::control().yield();
   char b[ 1024 ];
   mcp::control().yield();
   for( std::size_t i = 0; i < sizeof( b ); ++i ) {
      sum += b[ i ];
   }
   mcp::control().yield();
}

int main()
{
   mcp::coroutine coro( function );
   assert( coro.state() == mcp::state::STARTING );
   std::cout << "stack size: " << coro.stack_size() << " used: " << coro.stack_used() << std::endl;
   coro.resume();
   assert( coro.state() == mcp::state::SLEEPING );
   std::cout << "stack size: " << coro.stack_size() << " used: " << coro.stack_used() << std::endl;
   coro.resume();
   assert( coro.state() == mcp::state::SLEEPING );
   std::cout << "stack size: " << coro.stack_size() << " used: " << coro.stack_used() << std::endl;
   coro.resume();
   assert( coro.state() == mcp::state::SLEEPING );
   std::cout << "stack size: " << coro.stack_size() << " used: " << coro.stack_used() << std::endl;
   coro.resume();
   assert( coro.state() == mcp::state::SLEEPING );
   std::cout << "stack size: " << coro.stack_size() << " used: " << coro.stack_used() << std::endl;
   coro.resume();
   assert( coro.state() == mcp::state::SLEEPING );
   std::cout << "stack size: " << coro.stack_size() << " used: " << coro.stack_used() << std::endl;
   coro.resume();
   assert( coro.state() == mcp::state::COMPLETED );
   std::cout << "stack size: " << coro.stack_size() << " used: " << coro.stack_used() << std::endl;
   std::cout << "sum " << sum << std::endl;
   return 0;
}
