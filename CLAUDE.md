# Ulti Jarvis — Rules for Claude

Read this before doing anything in this repository. These rules are absolute.

## Project context

**Ulti Jarvis** is a cross-platform consumer desktop app (formerly "Pardus Jarvis" — that
name is dead, do not use it anywhere). Private repo, free, no monetisation. It is a real
shipping application, not a code exercise: every DLL / dylib / .so it needs is bundled so
that a normal user can install it and run it without installing anything themselves.

Three binaries: the daemon (wakeword listener), the main app (executes voice commands), and
the settings GUI.

Platform status:
- **macOS** — major progress, working
- **Windows 11** — major progress, working
- **Linux (Debian/apt + Fedora/rpm)** — NOT STARTED. This is the current job.

Current task: produce working **.deb** and **.rpm** packages with all required shared
objects bundled, installable on a bare Debian and a bare Fedora system that do not have the
dependencies present.

## RULE 1 — Never edit without explicit permission

Do not create, modify, or delete a single file without asking first and getting a clear yes.
There are linking bugs in this project that took **100+ hours** to resolve. An unrequested
edit can destroy that work.

Propose the change, show what it would be, wait for approval. Every time. No exceptions.

## RULE 2 — Linux code only, always inside platform guards

Every line of code written goes inside a Linux guard:

```cpp
#if defined(__linux__)
    // linux-only code here
#endif
```

or `#ifdef __linux__`. This matches the existing convention the macOS and Windows work used
(`#if defined(__APPLE__)`, `#ifdef _WIN32`).

Never write code that executes on macOS or Windows. Never touch code inside an `__APPLE__`
or `_WIN32` guard.

## RULE 3 — Global code is BANNED from modification

The app works perfectly as it stands. Shared/global/cross-platform code is off limits.

- You may **add** new Linux-guarded code.
- You may **change** existing code that is already inside a Linux guard.
- You may **not** change anything else — not shared logic, not headers used by all
  platforms, not the macOS or Windows paths, not build logic that affects other platforms.

If a Linux fix appears to require a global change, **stop and ask**. Do not do it
unilaterally.

## RULE 4 — NO ABSOLUTE PATHS. NONE.

Absolute paths are banned. This is critical for the packaging work.

- Shared object (`.so`) locations: **relative to the project / install root only**. Never an
  absolute path. Use `$ORIGIN`-relative RPATH, relative lookups, and relative CMake install
  destinations.
- Never hardcode `/home/...`, `/usr/lib/x86_64-linux-gnu/...`, or any machine-specific path
  for locating bundled libraries or assets.

The **only** acceptable exception: a path the user themselves chooses at install time (e.g.
the installer prompting for an install destination). Nothing else.

## RULE 5 — Forget git

No commits. No pushes. No staging. Nothing.

Git is completely off the table until the app is confirmed **100% working** and the user
explicitly says otherwise. Ask before any git operation that changes state.

## RULE 6 — Never build, configure or package

You do not run `cmake`, `ninja`, `make`, `cpack`, or any configure/build/package command.
Ever. Building is the user's job.

You write code. The user compiles it, packages it, and tests it. If you want to know
whether something compiles, ask the user to build — do not run it yourself.

Static checks that touch nothing (reading files, `sh -n`, grep, checking `if`/`endif`
balance) are fine. Anything that writes to `build/`, re-runs CMake, or produces a package
is not.

## Working method

1. Try to compile. Expect 100+ compilation and linking errors on the first attempt — that is
   the expected starting point, not a failure.
2. Work through them by trial and error, only ever adding or changing Linux-guarded code.
3. The user is setting up bare Debian and Fedora VMs to verify the packages install and run
   on systems that lack the needed `.so` files.
4. This will take a long time. Keep notes on what matters — findings, root causes, and
   anything that would be expensive to rediscover.

## Reference

Read the Windows and macOS platform files to understand the established pattern before
writing anything. Read them — do not edit them.
