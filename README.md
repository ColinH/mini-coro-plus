# Mini Coro Plus

Minimalistic asymmetric stackful [coroutines](https://en.wikipedia.org/wiki/Coroutine) for C++17.

Based on the context switching code copied from [minicoro](https://github.com/edubart/minicoro) C library.

### Status

Not quite finished yet, but a first set of unit tests shows that all the basics are functional.

Currently only x86-64 and AArch64 are supported and only on Posix-compatible platforms.
It might also work on Windows.

### Basics

A coroutine is created with a `std::function< void() >`, and optionally a stack size.

A newly created coroutine sits in state `STARTING` and does **not** implicitly jump into its function.

Running the coroutine function is achieve by calling `resume()` from outside the coroutine which puts the coroutine into state `RUNNING`.

This call to `resume()` will return when one of the following conditions is met:

 1. The coroutine function returns. The coroutine enters state `COMPLETED` and may **not** be resumed again.
 2. The coroutine function calls `yield()`. The coroutine enters state `SLEEPING` and can be resumed again later. Resuming continues execution of the coroutine function after the aforementioned  `yield()`.
 3. The coroutine function throws an exception. The coroutine enters state `COMPLETED` and may **not** be resumed again. In this case the call to `resume()` will throw the coroutine's exception rather than returning normally.

If the `resume()` is performed by another coroutine (itself in `RUNNING` state) then this calling coroutine transitions to state `CALLING`.

It is an error to call any function other than `state()` on a coroutine in `COMPLETED` state.

It is an error to call `abort()` on a coroutine that is in `RUNNING` or `CALLING` state.

### Destroy

Destroying a coroutine object in states `RUNNING` or `CALLING` is an error.

Destroying a coroutine object in states `STARTING` or `COMPLETED` does nothing special.

Destroying a coroutine in state `SLEEPING` performs an implicit `resume()` into the coroutine in order to clean up the coroutine by destroying all local objects currently on the coroutine stack.
This cleanup is achieved by throwing a dedicated exception from the `yield()` call inside the coroutine, the `yield()` call that last put the coroutine into `SLEEPING` state.

In order to not interfere with the cleanup, coroutine functions must take care to **not** accidentally catch and ignore exceptions of type `mcp::internal::terminator`!
These exceptions must escape from the coroutine function.

Calling `abort()` on a coroutine performs the cleanup for coroutines in state `SLEEPING` but not much else.

### Interface

```c++
enum class state : std::uint8_t
{
   STARTING,  // Created without entering the coroutine function.
   RUNNING,   // Entered the coroutine function and currently running it.
   SLEEPING,  // Entered the coroutine which then yielded back out again.
   CALLING,   // Entered the coroutine which then resumed a different one.
   COMPLETED  // Finished the coroutine function to completion.
};

void yield_running();  // Global function to yield the current coroutine, if any.

class coroutine
{
public:
   explicit coroutine( std::function< void() >&&, const std::size_t stack_size = 0 );
   explicit coroutine( const std::function< void() >&, const std::size_t stack_size = 0 );

   [[nodiscard]] mcp::state state() const noexcept;

   void abort();  // Called from outside the coroutine function.
   void resume();  // Called from outside the coroutine function.
   void yield();  // Called from within the coroutine function.

protected:
   std::shared_ptr< internal::implementation > m_impl;
};
```

### Development

This little project is my answer to "Is it possible to make minicoro C++ exception compatible?"

The first step was to strip minicoro down to barely "minimum viable product" levels.
Removed parts include asan and tsan support, the extra stack for argument passing, and most of the supported platforms.

The second step was the "C++-ification" of the remaining source code.
This includes some small changes, like using `enum class` for the state `enum`, to large ones, like using exceptions instead of return codes for reporting errors.

The third step was to add exception support, allowing coroutines to throw exceptions that propagate to the caller.
And the coroutine object itself uses RAII to behave well in the presence of exceptions.

Over time some of the removed features might get added again as the need arises.

---

Copyright (c) 2024 Dr. Colin Hirsch
