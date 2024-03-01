// Copyright (c) 2024 Dr. Colin Hirsch
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include "mini_coro_plus.hpp"
#include "mini_coro_plus.ipp"

void function( mcp::control& ctrl )
{
   std::cout << "Hello";
   ctrl.yield();
   std::cout << "World!";
}

int main()
{
   mcp::coroutine coro( function );
   coro.resume();
   std::cout << ", ";
   coro.resume();
   std::cout << std::endl;
   return 0;
}
