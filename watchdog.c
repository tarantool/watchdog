#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <stdatomic.h>

#include <lua.h>
#include <lauxlib.h>
#include <module.h> // tarantool

volatile static atomic_bool enable_coredump = false;
volatile static atomic_ullong pettime = 0.;
volatile static atomic_ullong timeout = 0.;
static struct fiber *f_petting = NULL;
static struct fiber *f_timer = NULL;

static ssize_t
coio_timer(va_list ap)
{
	(void)ap;

	uint64_t prev = clock_monotonic64();
	uint64_t tt;
	while (tt = atomic_load_explicit(&timeout, memory_order_acquire), tt) {
		uint64_t now = clock_monotonic64();

		// Pettime is updated every timeout/4 seconds. We want the
		// real timeout event to occur not earlier than timeout,
		// thus add spare 25% here.
		if (now > atomic_load_explicit(&pettime, memory_order_acquire) + (1.25 * tt)) {
			if (now - prev > 1) {
				// nanosleep took > 1 sec instead of 200ms
				// maybe system was suspended for a while
				// thus timeout should be ignored once
				atomic_store_explicit(&pettime, clock_monotonic64(), memory_order_release);
			} else {
				say_error("Watchdog timeout %.1f sec. Aborting", now - pettime);
				/** after exit() process doesn't save coredump but abort does */
				if (atomic_load_explicit(&enable_coredump, memory_order_acquire))
					abort();
				else
					exit(SIGABRT);
			}
		} else {
			struct timespec ms200 = {.tv_nsec = 200L * 1000 * 1000};
			nanosleep(&ms200, NULL);
		}

		prev = now;
	}

	say_info("Watchdog stopped");
	return 0;
}

static int
fiber_timer(va_list ap)
{
	(void)ap;
	fiber_set_joinable(fiber_self(), true);
	return coio_call(coio_timer);
}

static int
fiber_petting(va_list ap)
{
	(void)ap;
	uint64_t t;
	fiber_set_cancellable(true);
	while (!fiber_is_cancelled() && (t = atomic_load_explicit(&timeout, memory_order_acquire))) {
		atomic_store_explicit(&pettime, clock_monotonic64(), memory_order_release);
		fiber_sleep(t / 4. / 1000000000);
	}

	return 0;
}


static int
start(lua_State *L)
{
	double t = luaL_checknumber(L, 1);
	if (!isfinite(t) || t < 0) {
		return luaL_argerror(L, 1, "timeout must be positive");
	}

	if (lua_gettop(L) == 2) {
		atomic_store_explicit(&enable_coredump, lua_toboolean(L, 2), memory_order_release);
	}

	if (atomic_load_explicit(&timeout, memory_order_relaxed) != 0) {
		atomic_store_explicit(&timeout, (uint64_t)(t * 1000000000), memory_order_release);
		atomic_store_explicit(&pettime, clock_monotonic64(), memory_order_release);

		say_info("Watchdog timeout changed to %.1f sec (coredump %s)",
		   t, enable_coredump ? "enabled" : "disabled");
	} else {
		atomic_store_explicit(&timeout, (uint64_t)(t * 1000000000), memory_order_release);

		assert(f_petting == NULL);
		f_petting = fiber_new("watchdog_petting", fiber_petting);
		fiber_start(f_petting);

		assert(f_timer == NULL);
		f_timer = fiber_new("watchdog_timer", fiber_timer);
		fiber_start(f_timer);

		say_info("Watchdog started with timeout %.1f sec (coredump %s)",
		   t, enable_coredump ? "enabled" : "disabled");
	}

	return 0;
}

static int
stop(lua_State *L)
{
	(void)L;
	atomic_store_explicit(&timeout, 0, memory_order_release);

	if (f_petting != NULL) {
		fiber_cancel(f_petting);
		f_petting = NULL;
	}

	if (f_timer != NULL) {
		fiber_join(f_timer);
		f_timer = NULL;
	}

	return 0;
}

static void
watchdog_atexit(void)
{
	atomic_store_explicit(&timeout, 0, memory_order_release);
}

/* ====================LIBRARY INITIALISATION FUNCTION======================= */

LUA_API int
luaopen_watchdog(lua_State *L)
{

	lua_getglobal(L, "box"); // -0 +1
	if (!lua_istable(L, -1)) {
		return luaL_error(L, "assertion failed! missing global 'box'");
	}

	lua_pushstring(L, "ctl"); // -0 +1
	lua_rawget(L, -2); // -1 +1
	if (!lua_istable(L, -1)) {
		return luaL_error(L, "assertion failed! missing module 'box.ctl'");
	}

	lua_getfield(L, -1, "on_shutdown"); // -0 +1

	if (lua_isfunction(L, -1)) {
		lua_pushcfunction(L, stop); // -0 +1
		lua_call(L, 1, 0); // -2 +0
	} else {
		atexit(watchdog_atexit);
	}

	static const struct luaL_Reg lib [] = {
		{"start", start},
		{"stop", stop},
		{NULL, NULL}
	};

	luaL_newlib(L, lib); // -0 +1

	return 1;
}
