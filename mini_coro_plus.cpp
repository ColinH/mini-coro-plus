// Copyright (c) 2024 Dr. Colin Hirsch
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include "mini_coro_plus.hpp"
#include "mini_coro_plus.ipp"

void coro_entry()
{
   std::cerr << __LINE__ << " " << mcp::running_state() << std::endl;
   throw std::runtime_error( "hallo" );
   mcp::yield_running();
   std::cerr << __LINE__ << " " << mcp::running_state() << std::endl;
}

int main()
{
   try {
      mcp::coroutine coro( &coro_entry, 0 );
      std::cerr << __LINE__ << " " << coro.state() << std::endl;
      coro.resume();
      std::cerr << __LINE__ << " " << coro.state() << std::endl;
      coro.abort();
      std::cerr << __LINE__ << " " << coro.state() << std::endl;
   }
   catch( const std::exception& e ) {
      std::cerr << "error: " << e.what() << std::endl;
      return 1;
   }
   return 0;
}
