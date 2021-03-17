[![Build Status](https://travis-ci.org/tarantool/watchdog.svg?branch=master)](https://travis-ci.org/tarantool/watchdog)

# Simple watchdog module for Tarantool

The watchdog module is useful when
the responsiveness of the system is important.

The watchdog module spawns a thread and continuously checks the value
of an internal variable. The variable is updated periodically
by a separate tarantool fiber.

The fiber update period equals to 1/2 of the `timeout` parameter.
The watchdog period is hardcoded to be 200ms.

Whenever a problem with an update fiber occurs the watchdog thread
performs `exit(6)` if coredump is disabled otherwise `abort()`.
The problem may be caused by using blocking signals or
by mistakes in other modules (e.g. `while true ... end`)

## Installing

```bash
$ tarantoolctl rocks install watchdog
```

## Usage

```lua
local watchdog = require('watchdog')
watchdog.start(1) -- timeout in seconds (double)
watchdog.start(1, true) -- timeout in seconds (double) and coredump is enabled
```
