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

#ifdef COLINH_MINI_CORO_PLUS_IPP
#error "This file must be included in precisely one .cpp file of the project!"
#endif

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <functional>
#include <memory>
#include <new>
#include <stdexcept>
#include <string_view>
#include <utility>

#include <sys/mman.h>
#include <unistd.h>

#include "mini_coro_plus.hpp"

namespace mcp
{
   std::ostream& operator<<( std::ostream& os, const state st )
   {
      return os << to_string( st );
   }

   namespace internal
   {

#if defined( __aarch64__ )

      struct single_context
      {
         void *x[ 12 ] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
         void* sp = nullptr;
         void* lr = nullptr;
         void* d[ 8 ] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

         [[nodiscard]] const void* stack_pointer() const noexcept
         {
            return sp;
         }
      };

      extern "C"
      {
         void _mini_coro_plus_wrap_main();
         int _mini_coro_plus_switch( internal::single_context* from, internal::single_context* to );

__asm__(
  ".text\n"
#ifdef __APPLE__
  ".globl __mini_coro_plus_switch\n"
  "__mini_coro_plus_switch:\n"
#else
  ".globl _mini_coro_plus_switch\n"
  ".type _mini_coro_plus_switch #function\n"
  ".hidden _mini_coro_plus_switch\n"
  "_mini_coro_plus_switch:\n"
#endif

  "  mov x10, sp\n"
  "  mov x11, x30\n"
  "  stp x19, x20, [x0, #(0*16)]\n"
  "  stp x21, x22, [x0, #(1*16)]\n"
  "  stp d8, d9, [x0, #(7*16)]\n"
  "  stp x23, x24, [x0, #(2*16)]\n"
  "  stp d10, d11, [x0, #(8*16)]\n"
  "  stp x25, x26, [x0, #(3*16)]\n"
  "  stp d12, d13, [x0, #(9*16)]\n"
  "  stp x27, x28, [x0, #(4*16)]\n"
  "  stp d14, d15, [x0, #(10*16)]\n"
  "  stp x29, x30, [x0, #(5*16)]\n"
  "  stp x10, x11, [x0, #(6*16)]\n"
  "  ldp x19, x20, [x1, #(0*16)]\n"
  "  ldp x21, x22, [x1, #(1*16)]\n"
  "  ldp d8, d9, [x1, #(7*16)]\n"
  "  ldp x23, x24, [x1, #(2*16)]\n"
  "  ldp d10, d11, [x1, #(8*16)]\n"
  "  ldp x25, x26, [x1, #(3*16)]\n"
  "  ldp d12, d13, [x1, #(9*16)]\n"
  "  ldp x27, x28, [x1, #(4*16)]\n"
  "  ldp d14, d15, [x1, #(10*16)]\n"
  "  ldp x29, x30, [x1, #(5*16)]\n"
  "  ldp x10, x11, [x1, #(6*16)]\n"
  "  mov sp, x10\n"
  "  br x11\n"
#ifndef __APPLE__
  ".size _mini_coro_plus_switch, .-_mini_coro_plus_switch\n"
#endif
);

__asm__(
  ".text\n"
#ifdef __APPLE__
  ".globl __mini_coro_plus_wrap_main\n"
  "__mini_coro_plus_wrap_main:\n"
#else
  ".globl _mini_coro_plus_wrap_main\n"
  ".type _mini_coro_plus_wrap_main #function\n"
  ".hidden _mini_coro_plus_wrap_main\n"
  "_mini_coro_plus_wrap_main:\n"
#endif
  "  mov x0, x19\n"
  "  mov x30, x21\n"
  "  br x20\n"
#ifndef __APPLE__
  ".size _mini_coro_plus_wrap_main, .-_mini_coro_plus_wrap_main\n"
#endif
);

      }  // extern "C"

      void init_context( void* co, void* main, single_context& ctx, void* stack_base, std::size_t stack_size )
      {
         ctx.x[ 0 ] = co;  // coroutine
         ctx.x[ 1 ] = main;
         ctx.x[ 2 ] = (void*)( 0xdeaddeaddeaddead );  // Dummy return address.
         ctx.sp = (void*)( (std::size_t)stack_base + stack_size );
         ctx.lr = (void*)( _mini_coro_plus_wrap_main );
      }

#elif defined(__x86_64__) || defined(_M_X64)

      struct single_context
      {
         void* rip = nullptr;
         void* rsp = nullptr;
         void* rbp = nullptr;
         void* rbx = nullptr;
         void* r12 = nullptr;
         void* r13 = nullptr;
         void* r14 = nullptr;
         void* r15 = nullptr;

         [[nodiscard]] const void* stack_pointer() const noexcept
         {
            return rsp;
         }
      };

      extern "C"
      {
         void _mini_coro_plus_wrap_main();
         int _mini_coro_plus_switch( internal::single_context* from, internal::single_context* to );

__asm__(
  ".text\n"
#ifdef __MACH__ /* Darwin assembler */
  ".globl __mini_coro_plus_wrap_main\n"
  "__mini_coro_plus_wrap_main:\n"
#else /* Linux assembler */
  ".globl _mini_coro_plus_wrap_main\n"
  ".type _mini_coro_plus_wrap_main @function\n"
  ".hidden _mini_coro_plus_wrap_main\n"
  "_mini_coro_plus_wrap_main:\n"
#endif
  "  movq %r13, %rdi\n"
  "  jmpq *%r12\n"
#ifndef __MACH__
  ".size _mini_coro_plus_wrap_main, .-_mini_coro_plus_wrap_main\n"
#endif
);

__asm__(
  ".text\n"
#ifdef __MACH__ /* Darwin assembler */
  ".globl __mini_coro_plus_switch\n"
  "__mini_coro_plus_switch:\n"
#else /* Linux assembler */
  ".globl _mini_coro_plus_switch\n"
  ".type _mini_coro_plus_switch @function\n"
  ".hidden _mini_coro_plus_switch\n"
  "_mini_coro_plus_switch:\n"
#endif
  "  leaq 0x3d(%rip), %rax\n"
  "  movq %rax, (%rdi)\n"
  "  movq %rsp, 8(%rdi)\n"
  "  movq %rbp, 16(%rdi)\n"
  "  movq %rbx, 24(%rdi)\n"
  "  movq %r12, 32(%rdi)\n"
  "  movq %r13, 40(%rdi)\n"
  "  movq %r14, 48(%rdi)\n"
  "  movq %r15, 56(%rdi)\n"
  "  movq 56(%rsi), %r15\n"
  "  movq 48(%rsi), %r14\n"
  "  movq 40(%rsi), %r13\n"
  "  movq 32(%rsi), %r12\n"
  "  movq 24(%rsi), %rbx\n"
  "  movq 16(%rsi), %rbp\n"
  "  movq 8(%rsi), %rsp\n"
  "  jmpq *(%rsi)\n"
  "  ret\n"
#ifndef __MACH__
  ".size _mini_coro_plus_switch, .-_mini_coro_plus_switch\n"
#endif
);

      }  // extern "C"

      void init_context( void* co, void* main, internal::single_context& ctx, void* stack_base, std::size_t stack_size )
      {
         stack_size -= 128;  // Reserve 128 bytes for the Red Zone space (System V AMD64 ABI).
         void** stack_high_ptr = (void**)( (std::size_t)stack_base + stack_size - sizeof( std::size_t ) );
         stack_high_ptr[ 0 ] = (void*)( 0xdeaddeaddeaddead );  // Dummy return address.
         ctx.rip = (void*)( _mini_coro_plus_wrap_main );
         ctx.rsp = (void*)( stack_high_ptr );
         ctx.r12 = main;
         ctx.r13 = co;
      }

#else
#error "Unsupported architecture!"
#endif

      struct double_context
      {
         single_context this_ctx;
         single_context back_ctx;
      };

      struct terminator {};

      inline constexpr std::size_t align_quantum = 16;
      inline constexpr std::size_t min_stack_size = 1024 * 2;
      inline constexpr std::size_t default_stack_size = 1024 * 42;

      thread_local std::atomic< implementation* > running_coroutine = { nullptr };

      [[nodiscard]] std::size_t get_page_size() noexcept
      {
         return ::sysconf( _SC_PAGESIZE );
      }

      [[nodiscard]] constexpr bool nop_abort( const state st ) noexcept
      {
         return ( st == state::STARTING ) || ( st == state::COMPLETED );
      }

      [[nodiscard]] constexpr std::size_t align_forward( const std::size_t addr, const std::size_t align ) noexcept
      {
         return ( addr + ( align - 1 ) ) & ~( align - 1 );
      }

      [[nodiscard]] constexpr std::size_t calculate_stack_size( const std::size_t requested ) noexcept
      {
         if( requested == 0 ) {
            return default_stack_size;
         }
         if( requested < min_stack_size ) {
            return min_stack_size;
         }
         return align_forward( requested, align_quantum );
      }

      static_assert( align_forward( min_stack_size, align_quantum ) == min_stack_size );
      static_assert( align_forward( default_stack_size, align_quantum ) == default_stack_size );

      template< typename Coroutine >
      void try_catch_main( Coroutine* co )
      {
         try {
            co->execute();
            co->set_exception();
         }
         catch( const terminator& ) {
            co->set_exception();
         }
         catch( ... ) {
            co->set_exception( std::current_exception() );
         }
         co->yield( state::COMPLETED );
      }

      class implementation
      {
      public:
         implementation( implementation&& ) = delete;
         implementation( const implementation& ) = delete;

         ~implementation()
         {
            if( nop_abort( m_state ) ) {
               return;
            }
            if( m_state != state::SLEEPING ) {
               assert( !bool( "Destroying active coroutine!" ) );
               std::terminate();
            }
            try {
               assert( !m_exception );
               m_exception = std::make_exception_ptr( terminator() );
               resume_impl();
            }
            catch( ... ) {
               assert( !bool( "Exception while destroying coroutine!" ) );
               std::terminate();
            }
         }

         void operator=( implementation&& ) = delete;
         void operator=( const implementation&& ) = delete;

         [[nodiscard]] std::size_t stack_size() const noexcept
         {
            return m_stack_size;
         }

         [[nodiscard]] const void* stack_base() const noexcept
         {
            return m_stack_base;
         }

         [[nodiscard]] std::size_t stack_used() const noexcept
         {
            return static_cast< const char* >( m_stack_base ) + m_stack_size - static_cast< const char* >( m_contexts.this_ctx.stack_pointer() );
         }

         [[nodiscard]] mcp::state state() const noexcept
         {
            return m_state;
         }

         [[nodiscard]] std::any& xfer_r2y() noexcept
         {
            return m_xfer_r2y;
         }

         [[nodiscard]] std::any& xfer_y2r() noexcept
         {
            return m_xfer_y2r;
         }

         void set_state( const mcp::state st ) noexcept
         {
            m_state = st;
         }

         void set_xfer_r2y( std::any&& any = std::any() ) noexcept
         {
            m_xfer_r2y = std::move( any );
         }

         void set_xfer_y2r( std::any&& any = std::any() ) noexcept
         {
            m_xfer_y2r = std::move( any );
         }

         void set_exception( std::exception_ptr&& ptr = std::exception_ptr() ) noexcept
         {
            m_exception = std::move( ptr );
         }

         void abort()
         {
            if( nop_abort( m_state ) ){
               m_state = state::COMPLETED;
               return;
            }
            if( m_state != state::SLEEPING ) {
               throw std::logic_error( "Invalid state for coroutine abort!" );
            }
            assert( !m_exception );
            m_exception = std::make_exception_ptr( terminator() );
            resume_impl();

            if( m_exception ) {
               std::rethrow_exception( m_exception );
            }
         }

         void resume()
         {
            if( !can_resume( m_state ) ) {
               throw std::logic_error( "Invalid state for coroutine resume!" );
            }
            assert( !m_exception );
            resume_impl();

            if( m_exception ) {
               std::rethrow_exception( m_exception );
            }
         }

         void yield( const mcp::state st )
         {
            if( running_coroutine.load() != this ) {
               throw std::logic_error( "Invalid coroutine for yield!" );
            }
            if( !can_yield( m_state ) ) {
               throw std::logic_error( "Invalid state for coroutine yield!" );
            }
            m_state = st;
            yield_impl();

            if( m_exception ) {
               std::rethrow_exception( m_exception );
            }
         }

      protected:
         std::any m_xfer_r2y;
         std::any m_xfer_y2r;
         std::exception_ptr m_exception;
         mcp::state m_state = state::STARTING;
         double_context m_contexts;
         implementation* m_previous = nullptr;  // Where to yield back to (intrusive linked list).
         void* const m_stack_base;
         const std::size_t m_stack_size;

         implementation( void* stack_base, const std::size_t stack_size ) noexcept
            : m_stack_base( stack_base ),
              m_stack_size( stack_size )
         {}

         void resume_impl() noexcept
         {
            assert( m_previous == nullptr );
            m_previous = running_coroutine.load();
            if( m_previous != nullptr ) {
               assert( m_previous->state() == state::RUNNING );
               m_previous->set_state( state::CALLING );
            }
            running_coroutine.store( this );
            m_state = state::RUNNING;
            // atomic_signal_fence( std::memory_order::memory_order_seq_cst );
            atomic_thread_fence( std::memory_order::memory_order_seq_cst );
            _mini_coro_plus_switch( &m_contexts.back_ctx, &m_contexts.this_ctx );
         }

         void yield_impl()
         {
            if( m_previous != nullptr ) {
               assert( m_previous->state() == state::CALLING );
               m_previous->set_state( state::RUNNING );
            }
            running_coroutine.store( m_previous );
            m_previous = nullptr;
            // atomic_signal_fence( std::memory_order::memory_order_seq_cst );
            atomic_thread_fence( std::memory_order::memory_order_seq_cst );
            _mini_coro_plus_switch( &m_contexts.this_ctx, &m_contexts.back_ctx );
         }
      };

      template< typename Coroutine >
      class initialization
         : public implementation
      {
      protected:
         initialization( void* stack_base, const std::size_t stack_size ) noexcept
            : implementation( stack_base, stack_size )
         {
            init_context( this, reinterpret_cast< void* >( &try_catch_main< Coroutine > ), m_contexts.this_ctx, m_stack_base, m_stack_size );
         }
      };

      template< typename F >
      class coroutine;

      template<>
      class coroutine< void() >
         : public initialization< coroutine< void() > >
      {
      public:
         using function_t = void();

         coroutine( std::function< function_t > function, void* stack_base, const std::size_t stack_size ) noexcept
            : initialization< coroutine< function_t > >( stack_base, stack_size ),
              m_function( std::move( function ) )
         {}

         void execute()
         {
            m_function();
         }

      private:
         std::function< function_t > m_function;
      };

      template<>
      class coroutine< void( control& ) >
         : public initialization< coroutine< void( control& ) > >
      {
      public:
         using function_t = void( control& );

         coroutine( std::function< function_t > function, void* stack_base, const std::size_t stack_size ) noexcept
            : initialization< coroutine< function_t > >( stack_base, stack_size ),
              m_function( std::move( function ) )
         {}

         void execute()
         {
            control c( this );
            m_function( c );
         }

      private:
         std::function< function_t > m_function;
      };

      template< typename F >
      [[nodiscard]] static std::shared_ptr< implementation > make_shared( std::function< F > function, const std::size_t requested )
      {
         const std::size_t page_size = get_page_size();
         const std::size_t temp_size = calculate_stack_size( requested );
         const std::size_t this_size = align_forward( sizeof( coroutine< F > ), align_quantum );
         const std::size_t total_size = align_forward( temp_size + this_size, page_size );
         const std::size_t stack_size = total_size - this_size - align_quantum;
         const std::size_t alloc_size = total_size + page_size;

         char* const memory = static_cast< char* >( ::mmap( nullptr, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0 ) );

         if( memory == MAP_FAILED ) {
            throw std::bad_alloc();
         }
         if( ::mprotect( memory, page_size, PROT_NONE ) != 0 ) {
            const int r = ::munmap( memory, alloc_size );
            assert( r == 0 );
            (void)r;
            throw std::runtime_error( "Coroutine mprotect setup failed!" );
         }
         char* const object = memory + alloc_size - this_size;
         coroutine< F >* const created = new( object ) coroutine< F >( std::move( function ), memory + page_size, stack_size );  // noexcept

         return std::shared_ptr< implementation >( std::shared_ptr< void >( static_cast< void* >( memory ), [ created, alloc_size ]( void* ptr ) {
            created->~coroutine< F >();
            const int r = ::munmap( ptr, alloc_size );
            assert( r == 0 );
            (void)r;
         } ), created );
      }

   }  // namespace internal

   control::control()
      : control( internal::running_coroutine.load() )
   {}

   control::control( internal::implementation* impl )
      : m_impl( impl )
   {
      if( !m_impl ) {
         throw std::logic_error( "Creating control outside of running coroutine!" );
      }
   }

   mcp::state control::state() const noexcept
   {
      return bool( m_impl ) ? m_impl->state() : state::COMPLETED;
   }

   std::size_t control::stack_size() const noexcept
   {
      return m_impl->stack_size();
   }

   void control::yield()
   {
      m_impl->set_xfer_y2r();
      m_impl->yield( mcp::state::SLEEPING );
   }

   void control::yield( std::any&& any )
   {
      m_impl->set_xfer_y2r( std::move( any ) );
      m_impl->yield( mcp::state::SLEEPING );
   }

   void control::yield( const std::any& any )
   {
      yield( std::any( any ) );
   }

   std::any& control::yield_any()
   {
      yield();
      return m_impl->xfer_r2y();
   }

   std::any& control::yield_any( std::any&& any )
   {
      yield( std::move( any ) );
      return m_impl->xfer_r2y();
   }

   std::any& control::yield_any( const std::any& any )
   {
      yield( any );
      return m_impl->xfer_r2y();
   }

   coroutine::coroutine( std::function< void() >&& f, const std::size_t requested )
      : m_impl( internal::make_shared( std::move( f ), requested ) )
   {}

   coroutine::coroutine( const std::function< void() >& f, const std::size_t requested )
      : m_impl( internal::make_shared( f, requested ) )
   {}

   coroutine::coroutine( std::function< void( control& ) >&& f, const std::size_t requested )
      : m_impl( internal::make_shared( std::move( f ), requested ) )
   {}

   coroutine::coroutine( const std::function< void( control& ) >& f, const std::size_t requested )
      : m_impl( internal::make_shared( f, requested ) )
   {}

   mcp::state coroutine::state() const noexcept
   {
      return bool( m_impl ) ? m_impl->state() : state::COMPLETED;
   }

   std::size_t coroutine::stack_size() const noexcept
   {
      return m_impl->stack_size();
   }

   std::size_t coroutine::stack_used() const noexcept
   {
      return m_impl->stack_used();
   }

   void coroutine::abort()
   {
      m_impl->abort();
   }

   void coroutine::clear()
   {
      std::shared_ptr< internal::implementation >( std::move( m_impl ) )->abort();
   }

   void coroutine::resume()
   {
      m_impl->set_xfer_r2y();
      m_impl->resume();
   }

   void coroutine::resume( std::any&& any )
   {
      m_impl->set_xfer_r2y( std::move( any ) );
      m_impl->resume();
   }

   void coroutine::resume( const std::any& any )
   {
      resume( std::any( any ) );
   }

   std::any& coroutine::resume_any()
   {
      resume();
      return m_impl->xfer_y2r();
   }

   std::any& coroutine::resume_any( std::any&& any )
   {
      resume( std::move( any ) );
      return m_impl->xfer_y2r();
   }

   std::any& coroutine::resume_any( const std::any& any )
   {
      resume( any );
      return m_impl->xfer_y2r();
   }

   // TODO: A function to obtain how much stack is currently used in a running coroutine?

}  // namespace mcp

#define COLINH_MINI_CORO_PLUS_IPP
