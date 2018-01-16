package = 'watchdog'
version = 'scm-1'
source  = {
    url    = 'git://github.com/tarantool/watchdog.git',
    branch = 'master',
}
description = {
    summary  = "Simple watchdog module for Tarantool",
    homepage = 'https://github.com/tarantool/watchdog',
    license  = 'BSD'
}
dependencies = {
    'lua >= 5.1'
}
external_dependencies = {
    TARANTOOL = {
        header = "tarantool/module.h"
    }
}
build = {
    type = 'builtin',
    modules = {
        ['watchdog'] = {
            sources = "watchdog.c",
            incdirs = {
                "$(TARANTOOL_INCDIR)"
            }
        }
    }
}
-- vim: syntax=lua
