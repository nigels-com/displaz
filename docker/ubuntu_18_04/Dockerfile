# Minimal displaz Qt5 build, test and install
# using Docker Ubuntu 18.04 (Bionic)

FROM ubuntu:bionic
MAINTAINER Nigel Stewart (nigels@nigels.com)

#
# Optional custom package mirror
#
# --build-arg MIRROR=http://au.archive.ubuntu.com/ubuntu/
#

ARG MIRROR
ENV MIRROR ${MIRROR:-http://archive.ubuntu.com/ubuntu}

RUN sed -i -r "/#.*/d"                                           /etc/apt/sources.list
RUN sed -i -r "/^$/d"                                            /etc/apt/sources.list
RUN cat /etc/apt/sources.list
RUN sed -i -r "s#deb .* bionic(-.*)? #deb ${MIRROR} bionic\\1 #" /etc/apt/sources.list
RUN cat /etc/apt/sources.list

#
# Update and install build tools, dependencies
#

ENV DEBIAN_FRONTEND noninteractive

RUN apt update
RUN apt dist-upgrade -y

RUN apt install -y build-essential git cmake doxygen graphviz cppcheck
RUN apt install -y g++ cmake qt5-default python-docutils

#
# Clone repo
#

ENV SCRATCH /scratch
RUN mkdir ${SCRATCH}
WORKDIR ${SCRATCH}

ARG REPO
ENV REPO ${REPO:-https://github.com/FugroRoames/displaz.git}
RUN git clone ${REPO} repo

#
# Build
#

RUN mkdir -p ${SCRATCH}/repo/build_external
WORKDIR ${SCRATCH}/repo/build_external
RUN cmake ../thirdparty/external
RUN make

RUN mkdir -p ${SCRATCH}/repo/build
WORKDIR ${SCRATCH}/repo/build
RUN cmake ..
RUN make

#
# Test
#

RUN ctest

#
# Install
#

RUN make install
RUN find /usr/local
