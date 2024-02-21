# Mini Coro Plus

C++ Mini Coroutine Library

This library is based on [minicoro](https://github.com/edubart/minicoro) with two major changes.

 1. It is refactored into C++17 gaining support for exceptions in coroutines.
 2. A lot of supported platforms and features have been (temporarily) removed.

Currently only x86-64 and Aarch64 are supported and only on Posix-compatible platforms.

The other main missing feature is destroying coroutines that have not run to completion.
