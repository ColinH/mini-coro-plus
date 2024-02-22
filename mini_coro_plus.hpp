// Copyright (c) 2024 Dr. Colin Hirsch
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

// The code in this library is heavily based on minicoro by Eduardo Bart.
// See https://github.com/edubart/minicoro

// Copyright (c) 2021-2023 Eduardo Bart

// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Some of the following assembly code is taken from LuaCoco by Mike Pall.
// See https://coco.luajit.org/index.html

// MIT license

// Copyright (C) 2004-2016 Mike Pall. All rights reserved.

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef COLINH_MINI_CORO_PLUS_HPP
#define COLINH_MINI_CORO_PLUS_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <ostream>
#include <string_view>

namespace mcp
{
   class coroutine;

   namespace internal
   {
      class implementation;

   }  // namespace internal

   enum class state : std::uint8_t
   {
      STARTING,  // Created but has not started execution.
      RUNNING,   // Has started execution and is executing this very moment.
      SLEEPING,  // Has started execution but is not curently executing.
      CALLING,   // Has started execution but has called another coroutine.
      COMPLETED  // Has finished execution and MUST NOT be resumed again.
   };

   std::ostream& operator<<( std::ostream&, const state );

   [[nodiscard]] constexpr bool can_abort( const state st ) noexcept
   {
      return ( st == state::STARTING ) || ( st == state::SLEEPING ) || ( st == state::COMPLETED );
   }

   [[nodiscard]] constexpr bool nop_abort( const state st ) noexcept
   {
      return ( st == state::STARTING ) || ( st == state::COMPLETED );
   }

   [[nodiscard]] constexpr bool can_resume( const state st ) noexcept
   {
      return ( st == state::STARTING ) || ( st == state::SLEEPING );
   }

   [[nodiscard]] constexpr bool can_yield( const state st ) noexcept
   {
      return st == state::RUNNING;
   }

   [[nodiscard]] constexpr std::string_view to_string( const state st ) noexcept
   {
      switch( st ) {
         case state::STARTING:
            return "starting";
         case state::RUNNING:
            return "running";
         case state::SLEEPING:
            return "sleeping";
         case state::CALLING:
            return "calling";
         case state::COMPLETED:
            return "completed";
      }
      return "invalid";
   }

   void yield_running();

   class coroutine
   {
   public:
      explicit coroutine( std::function< void() >&&, const std::size_t stack_size = 0 );
      explicit coroutine( const std::function< void() >&, const std::size_t stack_size = 0 );

      [[nodiscard]] mcp::state state() const noexcept;

      void abort();
      void clear();
      void resume();
      void yield();

   protected:
      std::shared_ptr< internal::implementation > m_impl;
   };

}  // namespace mcp

#endif
