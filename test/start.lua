#!/usr/bin/env tarantool

require('strict').on()
local log = require('log')
local fiber = require('fiber')
local watchdog = require('watchdog')

local t0 = fiber.clock()
watchdog.start(1)
fiber.sleep(2)
local t1 = fiber.clock()
log.info("Still alive after %f", t1-t0)

-- watchdog.stop()
-- local ffi = require('ffi')
-- ffi.cdef[[
--     unsigned int sleep(unsigned int seconds);
-- ]]
-- log.warn("regular sleep ...")
-- ffi.C.sleep(2)

-- os.exit(0)
