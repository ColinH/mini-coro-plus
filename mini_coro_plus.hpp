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

#include <any>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <string_view>
#include <typeinfo>

namespace mcp
{
   class coroutine;

   namespace internal
   {
      class implementation;

   }  // namespace internal

   enum class state : std::uint8_t
   {
      STARTING,  // Created without entering the coroutine function.
      RUNNING,   // Entered the coroutine function and currently running it.
      SLEEPING,  // Entered the coroutine which then yielded back out again.
      CALLING,   // Entered the coroutine which then resumed a different one.
      COMPLETED  // Finished the coroutine function to completion.
   };

   [[nodiscard]] constexpr bool can_abort( const state st ) noexcept
   {
      return ( st == state::STARTING ) || ( st == state::SLEEPING ) || ( st == state::COMPLETED );
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
      using namespace std::literals::string_view_literals;

      switch( st ) {
         case state::STARTING:
            return "starting"sv;
         case state::RUNNING:
            return "running"sv;
         case state::SLEEPING:
            return "sleeping"sv;
         case state::CALLING:
            return "calling"sv;
         case state::COMPLETED:
            return "completed"sv;
      }
      return "invalid"sv;
   }

   std::ostream& operator<<( std::ostream&, const state );

   class control
   {
   public:
      control();

      explicit control( internal::implementation* );

      [[nodiscard]] mcp::state state() const noexcept;  // If the result is NOT state::RUNNING then the control object was passed somewhere it shouldn't be.
      [[nodiscard]] std::size_t stack_size() const noexcept;

      void yield();
      void yield( std::any&& any );
      void yield( const std::any& any );

      [[nodiscard]] std::any& yield_any();
      [[nodiscard]] std::any& yield_any( std::any&& any );
      [[nodiscard]] std::any& yield_any( const std::any& any );

      template< typename... Ts >
      void yield( Ts&&... ts )
      {
         yield( std::any( std::forward< Ts >( ts )... ) );
      }

      template< typename... Ts >
      [[nodiscard]] std::any& yield_any( Ts&&... ts )
      {
         return yield_any( std::any( std::forward< Ts >( ts )... ) );
      }

      template< typename T, typename... As >
      [[nodiscard]] T yield_as( As&&... as )
      {
         return std::any_cast< T >( yield_any( std::forward< As >( as )... ) );
      }

      template< typename T, typename... As >
      [[nodiscard]] std::optional< T > yield_opt( As&&... as )
      {
         std::any& any = yield_any( std::forward< As >( as )... );
         if( any.type() == typeid( T ) ) {
            return { std::any_cast< T >( any ) };
         }
         return std::nullopt;
      }

      template< typename T, typename... As >
      [[nodiscard]] T* yield_ptr( As&&... as )
      {
         return std::any_cast< T >( &yield_any( std::forward< As >( as )... ) );
      }

   private:
      internal::implementation* m_impl;
   };

   class coroutine
   {
   public:
      explicit coroutine( std::function< void() >&&, const std::size_t stack_size = 0 );
      explicit coroutine( const std::function< void() >&, const std::size_t stack_size = 0 );

      explicit coroutine( std::function< void( control& ) >&&, const std::size_t stack_size = 0 );
      explicit coroutine( const std::function< void( control& ) >&, const std::size_t stack_size = 0 );

      [[nodiscard]] mcp::state state() const noexcept;
      [[nodiscard]] std::size_t stack_size() const noexcept;
      [[nodiscard]] std::size_t stack_used() const noexcept;  // Not very precise?

      void abort();
      void clear();
      void resume();
      void resume( std::any&& any );
      void resume( const std::any& any );

      [[nodiscard]] std::any& resume_any();
      [[nodiscard]] std::any& resume_any( std::any&& any );
      [[nodiscard]] std::any& resume_any( const std::any& any );

      template< typename... Ts >
      void resume( Ts&&... ts )
      {
         resume( std::any( std::forward< Ts >( ts )... ) );
      }

      template< typename... Ts >
      [[nodiscard]] std::any& resume_any( Ts&&... ts )
      {
         return resume_any( std::any( std::forward< Ts >( ts )... ) );
      }

      template< typename T, typename... As >
      [[nodiscard]] T resume_as( As&&... as )
      {
         return std::any_cast< T >( resume_any( std::forward< As >( as )... ) );
      }

      template< typename T, typename... As >
      [[nodiscard]] std::optional< T > resume_opt( As&&... as )
      {
         std::any& any = resume_any( std::forward< As >( as )... );
         if( any.type() == typeid( T ) ) {
            return { std::any_cast< T >( any ) };
         }
         return std::nullopt;
      }

      template< typename T, typename... As >
      [[nodiscard]] T* resume_ptr( As&&... as )
      {
         return std::any_cast< T >( &resume_any( std::forward< As >( as )... ) );
      }

   protected:
      std::shared_ptr< internal::implementation > m_impl;
   };

}  // namespace mcp

#endif
