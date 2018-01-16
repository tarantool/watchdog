#!/bin/bash

TESTDIR=$(dirname $0)

trap "" SIGABRT
${TESTDIR}/abort.lua

# SIGABRT == 6
# 128 + 6 == 134
[ $? -eq 134 ]
