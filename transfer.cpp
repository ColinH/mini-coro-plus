// Copyright (c) 2024 Dr. Colin Hirsch
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

#include <cassert>
#include <iostream>

#include "mini_coro_plus.hpp"
#include "mini_coro_plus.ipp"

void function( mcp::control& ctrl )
{
   const int i = ctrl.yield_as< int >();
   std::cout << i << std::endl;
}

int main()
{
   mcp::coroutine coro( function );
   coro.resume();
   coro.resume( 42 );
   return 0;
}
