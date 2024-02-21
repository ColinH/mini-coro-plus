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

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <functional>
#include <memory>
#include <new>
#include <sys/mman.h>
#include <unistd.h>
#include <utility>

#include <iostream>  // TODO: Temporary.

namespace mcp
{
   class coroutine;

   namespace
   {
      struct terminator
      {};

   }  // namespace

   extern "C"
   {
      void mco_main( coroutine* );

   }  // extern "C"

#if defined(__aarch64__)

   namespace
   {
      struct single_context
      {
         void *x[ 12 ];  // x19-x30
         void *sp;
         void *lr;
         void *d[ 8 ];  // d8-d15
      };

   }  // namespace

   extern "C"
   {
      void _mco_wrap_main();
      int _mco_switch( single_context* from, single_context* to );

__asm__(
  ".text\n"
#ifdef __APPLE__
  ".globl __mco_switch\n"
  "__mco_switch:\n"
#else
  ".globl _mco_switch\n"
  ".type _mco_switch #function\n"
  ".hidden _mco_switch\n"
  "_mco_switch:\n"
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
  ".size _mco_switch, .-_mco_switch\n"
#endif
);

__asm__(
  ".text\n"
#ifdef __APPLE__
  ".globl __mco_wrap_main\n"
  "__mco_wrap_main:\n"
#else
  ".globl _mco_wrap_main\n"
  ".type _mco_wrap_main #function\n"
  ".hidden _mco_wrap_main\n"
  "_mco_wrap_main:\n"
#endif
  "  mov x0, x19\n"
  "  mov x30, x21\n"
  "  br x20\n"
#ifndef __APPLE__
  ".size _mco_wrap_main, .-_mco_wrap_main\n"
#endif
);

   }  // extern "C"

   namespace
   {
      void init_context( void* co, single_context& ctx, void* stack_base, std::size_t stack_size )
      {
         ctx.x[ 0 ] = co;  // coroutine
         ctx.x[ 1 ] = (void*)( mco_main );
         ctx.x[ 2 ] = (void*)( 0xdeaddeaddeaddead );  // Dummy return address.
         ctx.sp = (void*)( (std::size_t)stack_base + stack_size );
         ctx.lr = (void*)( _mco_wrap_main );
      }

   }  // namespace

#elif defined(__x86_64__) || defined(_M_X64)

   namespace
   {
      struct single_context
      {
         void *rip, *rsp, *rbp, *rbx, *r12, *r13, *r14, *r15;
      };

   }  // namespace

   extern "C"
   {
      void _mco_wrap_main();
      int _mco_switch( single_context* from, single_context* to );

__asm__(
  ".text\n"
#ifdef __MACH__ /* Darwin assembler */
  ".globl __mco_wrap_main\n"
  "__mco_wrap_main:\n"
#else /* Linux assembler */
  ".globl _mco_wrap_main\n"
  ".type _mco_wrap_main @function\n"
  ".hidden _mco_wrap_main\n"
  "_mco_wrap_main:\n"
#endif
  "  movq %r13, %rdi\n"
  "  jmpq *%r12\n"
#ifndef __MACH__
  ".size _mco_wrap_main, .-_mco_wrap_main\n"
#endif
);

__asm__(
  ".text\n"
#ifdef __MACH__ /* Darwin assembler */
  ".globl __mco_switch\n"
  "__mco_switch:\n"
#else /* Linux assembler */
  ".globl _mco_switch\n"
  ".type _mco_switch @function\n"
  ".hidden _mco_switch\n"
  "_mco_switch:\n"
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
  ".size _mco_switch, .-_mco_switch\n"
#endif
);

   }  // extern "C"

   namespace
   {
      void init_context( void* co, single_context& ctx, void* stack_base, const std::size_t stack_size )
      {
         stack_size -= 128;  // Reserve 128 bytes for the Red Zone space (System V AMD64 ABI).
         void** stack_high_ptr = (void**)( (std::size_t)stack_base + stack_size - sizeof( std::size_t ) );
         stack_high_ptr[ 0 ] = (void*)( 0xdeaddeaddeaddead );  // Dummy return address.
         ctx.rip = (void*)( _mco_wrap_main );
         ctx.rsp = (void*)( stack_high_ptr );
         ctx.r12 = (void*)( _mco_main );
         ctx.r13 = co;
      }

   }  // namespace

#else
#error "Unsupported architecture!"
#endif

   namespace
   {
      inline constexpr std::size_t magic_number = 0x7E3CB1A9;
      inline constexpr std::size_t min_stack_size = 16384;
      inline constexpr std::size_t default_stack_size = 56 * 1024;  // 2040 * 1024 for VMEM

      struct double_context
      {
         double_context() noexcept
         {
            std::memset( this, 0, sizeof( double_context ) );  // TODO?
         }

         single_context this_ctx;
         single_context back_ctx;
      };

      // using func_t = void( * )( coroutine* );
      // using function_t = std::function< void( coroutine* ) >;

      thread_local std::atomic< coroutine* > current_coroutine = { nullptr };

      [[nodiscard]] std::size_t get_page_size() noexcept
      {
         return ::sysconf( _SC_PAGESIZE );
      }

      [[nodiscard]] constexpr std::size_t align_forward( const std::size_t addr, const std::size_t align ) noexcept
      {
         return ( addr + ( align - 1 ) ) & ~( align - 1 );
      }

      [[nodiscard]] constexpr std::size_t calculate_stack_size( const std::size_t size ) noexcept
      {
         if( size == 0 ) {
            return default_stack_size;
         }
         if( size < min_stack_size ) {
            return min_stack_size;
         }
         return align_forward( size, 16 );
      }

   }  // namespace internal

   [[nodiscard]] coroutine* running() noexcept
   {
      return current_coroutine.load();  // Can be nullptr!
   }

   void set_running( coroutine* co ) noexcept
   {
      current_coroutine.store( co );  // Can be nullptr.
   }

   enum class state : std::uint8_t
   {
      DEAD = 0,  /* The coroutine has finished normally or was uninitialized before finishing. */
      NORMAL =1,    /* The coroutine is active but not running (that is, it has resumed another coroutine). */
      RUNNING = 2,   /* The coroutine is active and running. */
      SUSPENDED = 3  /* The coroutine is suspended (in a call to yield, or it has not started running yet). */
   };

   class coroutine
   {
   public:
      coroutine( coroutine&& ) = delete;
      coroutine( const coroutine& ) = delete;

      ~coroutine()
      {
         if( state() != state::DEAD ) {
            std::terminate();  // TODO: What else?
         }
      }

      void operator=( coroutine&& ) = delete;
      void operator=( const coroutine&& ) = delete;

      void execute()
      {
         m_function( this );
      }

      [[nodiscard]] mcp::state state() const noexcept
      {
         return m_state;
      }

      [[nodiscard]] const std::exception_ptr& exception() const noexcept
      {
         return m_exception;
      }

      void set_state( const mcp::state st ) noexcept
      {
         m_state = st;
      }

      void set_exception( std::exception_ptr&& ptr ) noexcept
      {
         m_exception = std::move( ptr );
      }

      template< typename F >
      [[nodiscard]] static std::shared_ptr< coroutine > create( F&& function, const std::size_t size )
      {
         const std::size_t stack = stack_size( size );
         const std::size_t total = stack + this_size();

         void* memory = std::calloc( 1, total );

         if( memory == nullptr ) {
            throw std::bad_alloc();
         }
         coroutine* result = new( memory ) coroutine( std::forward< F >( function ), stack );
         // TODO: Handle exceptions during shared_ptr construction or switch to unique_ptr.
         return std::shared_ptr< coroutine >( result, []( coroutine* ptr ) {
            ptr->~coroutine();
            ::free( ptr );
         } );
      }

      template< typename F >
      [[nodiscard]] static std::shared_ptr< coroutine > create2( F&& function, const std::size_t size )
      {
         const std::size_t stack = stack_size( size );
         const std::size_t total = stack + this_size();

         void *memory = ::mmap( nullptr, total, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0 );

         if( memory == MAP_FAILED ) {
            throw std::bad_alloc();
         }
         coroutine* result = new( memory ) coroutine( std::forward< F >( function ), stack );
         // TODO: Handle exceptions during shared_ptr construction or switch to unique_ptr.
         return std::shared_ptr< coroutine >( result, [ total ]( coroutine* ptr ) {
            ptr->~coroutine();
            ::munmap( ptr, total );
         } );
      }

      void abort()
      {
         assert( m_previous == nullptr );
         assert( m_state == state::SUSPENDED );

         try {
            throw terminator();
         }
         catch( const terminator& ) {
            m_exception = std::current_exception();
         }
         resume_impl();

         if( m_exception ) {
            std::rethrow_exception( m_exception );
         }
      }

      void resume()
      {
         assert( m_previous == nullptr );
         assert( m_state == state::SUSPENDED );

         m_exception = std::exception_ptr();

         resume_impl();  // Will return when the coroutine yields or terminates.

         if( m_exception ) {
            std::rethrow_exception( m_exception );
         }
      }

      void yield( const mcp::state st = mcp::state::SUSPENDED )
      {
         assert( running() == this );
         assert( m_state == state::RUNNING );

         m_exception = std::exception_ptr();

         volatile std::size_t dummy;

         const std::size_t stack_addr = (std::size_t)&dummy;
         const std::size_t stack_min = (std::size_t)m_stack_base;
         const std::size_t stack_max = stack_min + m_stack_size;

         if( ( m_magic != magic_number ) || ( stack_addr < stack_min ) || ( stack_addr > stack_max ) ) {
            throw std::runtime_error( "Coroutine stack overflow detected after the fact!" );
         }
         m_state = st;
         yield_impl();

         if( m_exception ) {
            std::rethrow_exception( m_exception );
         }
      }

   protected:
      template< typename F >
      coroutine( F&& function, const std::size_t stack ) noexcept
         : m_function( std::forward< F >( function ) ),
           m_stack_base( reinterpret_cast< char* >( this ) + this_size() ),
           m_stack_size( stack )
      {
         init_context( this, m_contexts.this_ctx, m_stack_base, m_stack_size );
      }

      [[nodiscard]] static std::size_t this_size() noexcept
      {
         return align_forward( sizeof( coroutine ), 16 );
      }

      [[nodiscard]] static std::size_t stack_size( const std::size_t size ) noexcept
      {
         return align_forward( calculate_stack_size( size ), 16 );
      }

      void resume_impl() noexcept
      {
         coroutine* prev_co = running();
         m_previous = prev_co;
         if( prev_co != nullptr ) {
            assert( prev_co->state() == state::RUNNING );
            prev_co->set_state( state::NORMAL );
         }
         set_running( this );
         m_state = state::RUNNING;
         _mco_switch( &m_contexts.back_ctx, &m_contexts.this_ctx );
      }

      void yield_impl()
      {
         coroutine* prev_co = m_previous;
         m_previous = nullptr;
         if( prev_co != nullptr ) {
            assert( prev_co->state() == state::NORMAL );
            prev_co->set_state( state::RUNNING );
         }
         set_running( prev_co );
         _mco_switch( &m_contexts.this_ctx, &m_contexts.back_ctx );
      }

      std::exception_ptr m_exception;
      std::function< void( coroutine* ) > m_function;
      double_context m_contexts;
      mcp::state m_state = state::SUSPENDED;
      coroutine* m_previous = nullptr;  // Intrusive single-linked list for where to yield to.

      void* m_stack_base = nullptr;
      std::size_t m_stack_size = 0;

      volatile std::size_t m_magic = magic_number;
   };

   extern "C"
   {
      void mco_main( coroutine* co )
      {
         try {
            co->execute();
            std::cerr << "finished" << std::endl;
         }
         catch( const terminator& ) {
            std::cerr << "terminated" << std::endl;
            co->set_exception( std::exception_ptr() );
         }
         catch( ... ) {
            std::cerr << "errored" << std::endl;
            co->set_exception( std::current_exception() );
         }
         co->yield( state::DEAD );
      }

   }  // extern "C"

   void yield_running()
   {
      running()->yield();
   }

}  // namespace mcp

#include <cstdio>
#include <iostream>

void coro_entry( mcp::coroutine* co )
{
   printf("coroutine 1\n");
   // throw std::runtime_error( "hallo" );
   co->yield();
   printf("coroutine 2\n");
}

int main()
{
   try {
      const auto coro = mcp::coroutine::create2( &coro_entry, 0 );
      printf("main 1 %d\n", int( coro->state() ) );
      coro->resume();
      printf("main 2 %d\n", int( coro->state() ) );
      coro->abort();
      printf("main 3 %d\n", int( coro->state() ) );
   }
   catch( const std::exception& e ) {
      std::cerr << "error: " << e.what() << std::endl;
      return 1;
   }
   return 0;
}
