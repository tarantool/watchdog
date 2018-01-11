#!/usr/bin/env tarantool

local log = require('log')
local fiber = require('fiber')
require('strict').on()
local watchdog = require('watchdog')

watchdog.start(1)
log.warn("fiber sleep ...")
fiber.sleep(2)
watchdog.stop()
-- local ffi = require('ffi')
-- ffi.cdef[[
--     unsigned int sleep(unsigned int seconds);
-- ]]
-- log.warn("regular sleep ...")
-- ffi.C.sleep(2)

os.exit(0)
