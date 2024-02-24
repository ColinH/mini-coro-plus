# Mini Coro Plus

Minimalistic asymmetric stackful [coroutines](https://en.wikipedia.org/wiki/Coroutine) for C++17.

Based on the design and implementation of [minicoro](https://github.com/edubart/minicoro) by Eduardo Bart.
Uses assembly context switching code from (LuaCoco)[https://coco.luajit.org] by Mike Pall.

## Status

Not quite finished yet, but the first unit tests all run successfully.

Currently only x86-64 and AArch64 are supported and only on Posix-compatible platforms.

## Hello

A traditional "hello world" example.

The `.ipp` file contains the library implementation and must be included in exactly one translation unit of the project.
The `.hpp` file contains the interface and can be included as often as required.

```c++
#include <iostream>

#include "mini_coro_plus.hpp"
#include "mini_coro_plus.ipp"

void function()
{
   std::cout << "Hello, ";
   mcp::yield_running();
   std::cout << "World!\n";
}

int main()
{
   mcp::coroutine coro( function );
   coro.resume();
   coro.resume();
   return 0;
}
```

A [lambda expression](https://en.cppreference.com/w/cpp/language/lambda) can be used as first argument to the coroutine constructor.

How to compile the example when not using the included `Makefile`.

```
$ g++ -std=c++17 -O3 -Wall -Wextra hello.cpp -o hello
$ ./hello
Hello, World!
```

## Creating

A coroutine is created with a `std::function< void() >`, and optionally a stack size.

A newly created coroutine sits in state `STARTING` and does **not** implicitly jump into its function.

## Running

Running the coroutine function is achieve by calling `resume()` from outside the coroutine which puts the coroutine into state `RUNNING`.

This call to `resume()` will return when one of the following conditions is met:

 1. The coroutine function returns. The coroutine enters state `COMPLETED` and may **not** be resumed again.
 2. The coroutine function calls `yield()`. The coroutine enters state `SLEEPING` and can be resumed again later. Resuming continues execution of the coroutine function after the aforementioned  `yield()`.
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

This library resides in `namespace mcp`.
The namespace is omitted from the following exposition.

```c++
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

class coroutine
{
public:
   explicit coroutine( std::function< void() >&&, const std::size_t stack_size = 0 );
   explicit coroutine( const std::function< void() >&, const std::size_t stack_size = 0 );

   [[nodiscard]] mcp::state state() const noexcept;

   void abort();  // Called from outside the coroutine function.
   void clear();  // Called from outside the coroutine function.
   void resume();  // Called from outside the coroutine function.
   void yield();  // Called from within the coroutine function.

protected:
   std::shared_ptr< internal::implementation > m_impl;
};

void yield_running();  // Global function to yield the current coroutine, if any.
```

## Control Flow

Assume that `coro` is an `mcp::coroutine` created with a closure, function or functor `F` as first argument.
And remember that creating a coroutine does *not* yet call `F`.

| Action | Where | What | Where | What |
| --- | --- | --- | --- | --- |
| Start | Outside | Call `coro.resume()` | Inside | `F()` is called |
| Yield | Inside | Call `coro.yield()` | Outside | `coro.resume()` returns |
| Resume | Outside | Call `coro.resume()` | Inside | `coro.yield()` returns |
| Finish | Inside | Return from `F()` | Outside | `coro.resume()` returns |
| Throw | Inside | `F()` throws `E` | Outside | `coro.resume()` throws `E` |

A coroutine can be resumed multiple times, until it finishes or throws.

## Multithreading

This library is essentially thread agnostic and compatible with multi-threaded applications.

In a multi-threaded application it is safe for multiple threads to create and run coroutines.
The global `yield_running()` function applies to any coroutine running on the current thread.

Coroutines can be created on one thread and resumed on a different thread.
The usual care needs to be taken regarding concurrent access to shared data and the lifecycle of shared data.

A coroutine object **must not** be used in multiple threads simultaneously as bad things **will** happen.

## Development

This project was started to answer to investigate whether it is possible to make [minicoro](https://github.com/edubart/minicoro) more C++ compatible.
Two of the main points were (a) what is needed to make C++ exception propagate from inside a coroutine to outside, and (b) can the coroutine object itself use RAII to clean up after itself?
Turns out, the answer is "yes", however the necessary changes are quite fundamental and not well suited for additional or optional inclusion in minicoro.

---

Copyright (c) 2024 Dr. Colin Hirsch
