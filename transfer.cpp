// Copyright (c) 2024 Dr. Colin Hirsch
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

#include <cassert>
#include <iostream>

#include "mini_coro_plus.hpp"
#include "mini_coro_plus.ipp"

void function( mcp::control& ctrl )
{
   std::string s = "test";
   const int i = ctrl.yield_as< int >( s );
   std::cout << i << std::endl;
}

int main()
{
   mcp::coroutine coro( function );
   const auto s = coro.resume_as< std::string >();
   std::cout << s << std::endl;
   coro.resume( 42 );
   return 0;
}
