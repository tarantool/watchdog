#!/usr/bin/env tarantool

require('strict').on()
local log = require('log')
local ffi = require('ffi')
local clock = require('clock')
local watchdog = require('watchdog')

local t0 = clock.monotonic()
watchdog.start(1)

ffi.cdef[[unsigned int sleep(unsigned int seconds);]]
log.info("Running ffi.sleep(2)")
ffi.C.sleep(2)

local t1 = clock.monotonic()
log.info("Still alive after %f sec", t1-t0)

os.exit(1)
