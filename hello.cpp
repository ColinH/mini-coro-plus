// Copyright (c) 2024 Dr. Colin Hirsch
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

#include <cassert>
#include <iostream>

#include "mini_coro_plus.hpp"
#include "mini_coro_plus.ipp"

void function()
{
   std::cout << "Hello, ";
   mcp::control().yield();
   std::cout << "World!\n";
}

int main()
{
   mcp::coroutine coro( function );
   assert( coro.state() == mcp::state::STARTING );
   coro.resume();
   assert( coro.state() == mcp::state::SLEEPING );
   coro.resume();
   assert( coro.state() == mcp::state::COMPLETED );
   return 0;
}
