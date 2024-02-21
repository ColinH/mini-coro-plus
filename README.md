# Mini Coro Plus

C++ Mini Coroutine Library

This library is based on [minicoro](https://github.com/edubart/minicoro) with two major changes.

 1. It is refactored into C++17 gaining support for exceptions in coroutines.
 2. A lot of supported platforms and features have been (temporarily?) removed.

The `coroutine` object itself is (TODO: mostly) exception safe and will clean up after itself in the case of exceptions.

When an exception is thrown but not caught within a coroutine it terminates the coroutine (sets its state to `DEAD`) and the exception is propagated to the caller of `resume()`.
There is nothing that prevents this from being arbitrarily nested, i.e. an exception flying across multiple `resume()` calls.

When a coroutine object in state `SUSPENDED` is destroyed then the coroutine is resumed one more and a special `terminator` exception thrown within the coroutine.
This allows the coroutine to clean up its stack and call the destructors of all local objects along the way.

Currently only x86-64 and AArch64 are supported and only on Posix-compatible platforms.

This library should **NOT** be considered "ready for action" yet!
