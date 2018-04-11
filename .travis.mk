all:
	docker run \
		--rm=true \
		--tty=true \
		-v $(CURDIR):/tmp/watchdog \
	tarantool/tarantool:1.7 \
	/bin/sh -c '\
		apk add --no-cache --virtual .build-deps gcc make cmake musl-dev; \
		cd /tmp/watchdog; \
		tarantoolctl rocks make; \
		make -C build.luarocks ARGS=-V test; \
	'
