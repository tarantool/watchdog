#!/bin/sh

TESTDIR=$(dirname $0)
${TESTDIR}/abort.lua
[ $? -eq 6 ]
