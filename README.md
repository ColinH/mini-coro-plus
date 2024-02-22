# Mini Coro Plus

Minimalistic asymmetric stackful [coroutines](https://en.wikipedia.org/wiki/Coroutine) for C++17.

Based on the context switching code copied from [minicoro](https://github.com/edubart/minicoro) C library.

### Status

Not quite "ready for action" yet.
Once all details are finalised I'll write some unit tests and make a release.

Currently only x86-64 and AArch64 are supported and only on Posix-compatible platforms.
It might also work on Windows.

### Behaviour

A coroutine is created with a callable function suitable for `std::function< void() >` and optionally a stack size.

A newly created coroutine sits in state `STARTING` and does **not** jump into its callable function.

A `resume` initially starts or later resumes a coroutine (again).
This puts the coroutine into state `RUNNING`.

The call to `resume` will return when the coroutine finishes execution and is `COMPLETED`, when the coroutine performs a `yield` into state `SLEEPING`, or when the coroutine throws an exception.

In the exception case the `resume` call will appear to throw the exception thrown from within the coroutine.
The coroutine that threw an exception is put into state `COMPLETED`.

If `resume` is performed by another coroutine that was itself `RUNNING` then this calling coroutine is put into state `CALLING`.

When a coroutine is destroyed in state `SLEEPING` a final `resume` into the coroutine is required.
It throws a special terminator exception from the `yield` that suspended the coroutine into the `SLEEPING` state in order to clean up the coroutine by calling the destructors of all local objects currently on the stack.

When a coroutine is destroyed in states `STARTING` or `COMPLETED` nothing special happens.

A coroutine object can **not** be destroyed in states `RUNNING` or `CALLING`.

Calling `abort` on a coroutine similarly performs the same operation without actually destroying the coroutine object yet.

It is an error to call any function other than `state` on a coroutine in `COMPLETED` state.

The coroutine object is a handle to the actual coroutine and has **pointer semantics**.

### Interface

```c++
enum class state : std::uint8_t
{
   STARTING,  // Created but has not started execution.
   RUNNING,   // Has started execution and is executing this very moment.
   SLEEPING,  // Has started execution but is not curently executing.
   CALLING,   // Has started execution but has called another coroutine.
   COMPLETED  // Has finished execution and MUST NOT be resumed again.
};

void yield_running();  // Global function to yield the current coroutine, if any.

[[nodiscard]] state running_state() noexcept;

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
