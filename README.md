# Mini Coro Plus

Minimalistic asymmetric stackful [coroutines](https://en.wikipedia.org/wiki/Coroutine) for C++17.

Based on the design and implementation of [minicoro](https://github.com/edubart/minicoro) by Eduardo Bart.

Uses assembly context switching code from [LuaCoco](https://coco.luajit.org) by Mike Pall.

## Status

Not quite finished yet, but the first unit tests all run successfully.

Currently only x86-64 and AArch64 are supported and only on Posix-compatible platforms.

## Hello

A traditional "hello world" example.

The `.ipp` file contains the library implementation and must be included in exactly one translation unit.
The `.hpp` file contains the interface and can be included as often as required.

```c++
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
```

To compile this example best use the included `Makefile` that builds all included `.cpp` files into corresponding executables -- and runs the `tests`.
The "hello world" example can then be invoked manually.

```
mini-coro-plus> make
c++ -std=c++17 -stdlib=libc++ -pedantic -MM -MQ build/dep/hello.d hello.cpp -o build/dep/hello.d
c++ -std=c++17 -stdlib=libc++ -pedantic -Wall -Wextra -Werror -O3 hello.cpp  -o build/bin/hello
build/bin/tests
mcp: all testcases succeeded
mini-coro-plus> build/bin/hello
Hello, World!
```

## Creating

A coroutine is created with a `std::function< void() >`, and, optionally, a stack size.

As usual this means that function pointers, function objects (aka. functors) and closures (evaluated lambda expressions) can be passed as first argument.

A newly created coroutine sits in state `STARTING` and does **not** implicitly jump into its coroutine function!

## Running

Running the coroutine function is achieve by calling `resume()` from outside the coroutine which puts the coroutine into state `RUNNING`.

This call to `resume()` will return when one of the following conditions is met:

 1. The coroutine function returns. The coroutine enters state `COMPLETED` and may **not** be resumed again.
 2. The coroutine function calls `ctrl.yield()` on a suitable instance of `mcp::control`. The coroutine enters state `SLEEPING` and can be resumed again later. Resuming continues execution of the coroutine function after the aforementioned  `yield()`.
 3. The coroutine function throws an exception. The coroutine enters state `COMPLETED` and may **not** be resumed again. In this case the call to `resume()` will throw the coroutine's exception rather than returning normally.

If the `resume()` is performed by another coroutine (itself in `RUNNING` state) then this calling coroutine transitions to state `CALLING`.

It is an error to call any function other than `state()` on a coroutine in `COMPLETED` state.

It is an error to call `abort()` on a coroutine that is in `RUNNING` or `CALLING` state.

## Destroying

Destroying a coroutine object in states `RUNNING` or `CALLING` is an error.

Destroying a coroutine object in states `STARTING` or `COMPLETED` does nothing special.

Destroying a coroutine in state `SLEEPING` performs an implicit `resume()` into the coroutine in order to clean up the coroutine by destroying all local objects currently on the coroutine stack.
This cleanup is achieved by throwing a dedicated exception from the `yield()` call inside the coroutine, the `yield()` call that last put the coroutine into `SLEEPING` state.

In order to not interfere with the cleanup, coroutine functions must take care to **not** accidentally catch and ignore exceptions of type `mcp::internal::terminator`!
These exceptions must escape from the coroutine function.

Calling `abort()` on a coroutine performs the cleanup for coroutines in state `SLEEPING` but not much else.

## Interface

The following is an excerpt of `mini_coro_plus.hpp` with all parts that are not considered part of the public interface removed.

```c++
namespace mcp
{
   enum class state : std::uint8_t
   {
      STARTING,  // Created without entering the coroutine function.
      RUNNING,   // Entered the coroutine function and currently running it.
      SLEEPING,  // Entered the coroutine which then yielded back out again.
      CALLING,   // Entered the coroutine which then resumed a different one.
      COMPLETED  // Finished the coroutine function to completion.
   };

   [[nodiscard]] constexpr bool can_abort( const state ) noexcept;
   [[nodiscard]] constexpr bool can_resume( const state ) noexcept;
   [[nodiscard]] constexpr bool can_yield( const state ) noexcept;

   [[nodiscard]] constexpr std::string_view to_string( const state ) noexcept;

   std::ostream& operator<<( std::ostream&, const state );

   // Control is for coroutine functions to control the coroutine they are currently running in.

   class control
   {
   public:
      control();

      [[nodiscard]] mcp::state state() const noexcept;  // Always state::RUNNING when used correctly.
      [[nodiscard]] std::size_t stack_size() const noexcept;

      void yield();
      void yield( std::any&& any );
      void yield( const std::any& any );

      [[nodiscard]] std::any& yield_any();
      [[nodiscard]] std::any& yield_any( std::any&& any );
      [[nodiscard]] std::any& yield_any( const std::any& any );

      template< typename... Ts >
      void yield( Ts&&... ts );

      template< typename... Ts >
      [[nodiscard]] std::any& yield_any( Ts&&... ts );

      template< typename T, typename... As >
      [[nodiscard]] T yield_as( As&&... as );

      template< typename T, typename... As >
      [[nodiscard]] std::optional< T > yield_opt( As&&... as );

      template< typename T, typename... As >
      [[nodiscard]] T* yield_ptr( As&&... as );
   };

   // Coroutine is for creating and controlling coroutines from the outside.

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
      void resume( Ts&&... ts );

      template< typename... Ts >
      [[nodiscard]] std::any& resume_any( Ts&&... ts );

      template< typename T, typename... As >
      [[nodiscard]] T resume_as( As&&... as );

      template< typename T, typename... As >
      [[nodiscard]] std::optional< T > resume_opt( As&&... as );

      template< typename T, typename... As >
      [[nodiscard]] T* resume_ptr( As&&... as );
   };

}  // namespace mcp

#endif

```

## Control Flow

Assume that `coro` is an `mcp::coroutine` created with a closure, function or functor `F` as first argument.
And remember that creating a coroutine does *not* yet call `F`.

| Action | Where | What | Where | What |
| --- | --- | --- | --- | --- |
| Start | Outside | Call `coro.resume()` | Inside | `F()` is called |
| Yield | Inside | Call `ctrl.yield()` | Outside | `coro.resume()` returns |
| Resume | Outside | Call `coro.resume()` | Inside | `ctrl.yield()` returns |
| Finish | Inside | Return from `F()` | Outside | `coro.resume()` returns |
| Throw | Inside | `F()` throws `E` | Outside | `coro.resume()` throws `E` |

A coroutine can be resumed multiple times, until it finishes or throws.

## Multithreading

This library is essentially thread agnostic and compatible with multi-threaded applications.

In a multi-threaded application it is safe for multiple threads to create and run coroutines.
Calling `mcp::control().yield()` applies to the coroutine running on the current thread.
It is an error to call from outside of a running coroutine.

Coroutines can be created on one thread and resumed on a different thread.
The usual care needs to be taken as a coroutine object **must not** be used in multiple threads simultaneously as bad things **will** happen.

## Development

This project was started to answer to investigate whether it is possible to make [minicoro](https://github.com/edubart/minicoro) more C++ compatible.
Two of the main points were (a) what is needed to make C++ exception propagate from inside a coroutine to outside, and (b) can the coroutine object itself use RAII to clean up after itself?
Turns out, the answer is "yes", however the necessary changes are quite fundamental and probably not well suited for additional or optional inclusion in minicoro.

---

Copyright (c) 2024 Dr. Colin Hirsch
