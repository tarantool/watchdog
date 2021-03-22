#!/bin/sh

TESTDIR=$(dirname $0)
${TESTDIR}/abort.lua false
[ $? -eq 6 ]
echo "Abort without coredump - OK"

TESTDIR=$(dirname $0)
${TESTDIR}/abort.lua
[ $? -eq 6 ]
echo "Abort without coredump (by default) - OK"

${TESTDIR}/abort.lua true
[ $? -eq 134 ]  # 134 == 128 + 6
echo "Abort with coredump - OK"
