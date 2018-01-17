[![Build Status](https://travis-ci.org/tarantool/watchdog.svg?branch=master)](https://travis-ci.org/tarantool/watchdog)

# Simple watchdog module for Tarantool

The watchdog module spawns a thread and continuously checks the value
of an internal variable. The variable is periodically being updated
by a separate tarantool fiber.

Whenever a problem with an update fiber occurs the watchdog thread
sends the ABRT signal to the tarantool process thus committing suicide.

## Installing

```bash
$ tarantoolctl rocks install watchdog
```

## Usage

```lua
local watchdog = require('watchdog')
watchdog.start(1) -- timeout in seconds (double)
```
