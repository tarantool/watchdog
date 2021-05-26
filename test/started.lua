#!/usr/bin/env tarantool

require('strict').on()
local log = require('log')
local clock = require('clock')
local fiber = require('fiber')
local watchdog = require('watchdog')

local t0 = clock.monotonic()
watchdog.start(1)

log.info("Running fiber.sleep(2)")
fiber.sleep(2)

local t1 = clock.monotonic()
log.info("Still alive after %f sec", t1-t0)

watchdog.stop()

-- Start with enable_coredump option
watchdog.start(1, true)

-- Exit tarantool without stopping watchdog explicitly.
-- It shouldn't raise.
os.exit(0)
