FROM centos:7

RUN curl -L https://tarantool.io/xLHQQc/release/2.6/installer.sh | bash
RUN yum install -y tarantool tarantool-devel
RUN yum install -y make gcc cmake curl libtool automake autoconf zip unzip

ADD . /watchdog

WORKDIR /watchdog

ARG TAG=scm

RUN tarantoolctl rocks new_version --tag $TAG \
    && tarantoolctl rocks STATIC_BUILD=ON make watchdog-$TAG-1.rockspec \
    && tarantoolctl rocks pack watchdog $TAG
