FROM ubuntu:22.04 as build
SHELL ["/bin/bash", "-c"]
ARG BOOST_VERSION=1.79.0
ARG NUM_JOBS=8
ARG EIGEN_VERSION=3.4.0

ENV DEBIAN_FRONTEND noninteractive

# Install package dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        g++ \
        autotools-dev \
        libicu-dev \
        libbz2-dev  \
        build-essential \
        cmake \
        python3-dev \
        openssl \
        libssl-dev \
        ca-certificates \
        wget \
        git \
        language-pack-en \
        locales \
        locales-all \
        vim && \
    apt-get clean

# System locale
# Important for UTF-8
ENV LC_ALL en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US.UTF-8


# Install Boost
# https://www.boost.org/doc/libs/1_80_0/more/getting_started/unix-variants.html
RUN cd /tmp && \
    BOOST_VERSION_MOD=$(echo $BOOST_VERSION | tr . _) && \
    wget --progress=bar:force  https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION_MOD}.tar.bz2 && \
    tar --bzip2 -xf boost_${BOOST_VERSION_MOD}.tar.bz2 && \
    cd boost_${BOOST_VERSION_MOD} && \
    ./bootstrap.sh --prefix=/usr/local --with-libraries=atomic,date_time,exception,filesystem,iostreams,locale,program_options,regex,system,test,thread,timer,log,json,context,fiber && \
    ./b2 --with=all -j ${NUM_JOBS} install && \
    rm -rf /tmp/*

# Install Eigen
# https://eigen.tuxfamily.org/
RUN cd /tmp && \
    wget --progress=bar:force https://gitlab.com/libeigen/eigen/-/archive/${EIGEN_VERSION}/eigen-${EIGEN_VERSION}.tar.bz2 && \
    tar --bzip2 -xf eigen-${EIGEN_VERSION}.tar.bz2 && \
    cd eigen-${EIGEN_VERSION} && \
    mkdir build_dir && \
    cd build_dir && \
    cmake ../../eigen-${EIGEN_VERSION} && \
    make install  && \
    rm -rf /tmp/*
   
WORKDIR /usr/share/app

# Download submodules and checkout to needed commit
RUN git init && \
    git submodule add https://github.com/fmtlib/fmt.git extern/fmt && cd extern/fmt && git checkout c85efef && cd - && \
    git submodule add https://github.com/google/flatbuffers.git extern/flatbuffers && cd extern/flatbuffers && git checkout bcd58a1  && cd -  && \
    git submodule add https://github.com/google/googletest.git extern/googletest &&  cd extern/googletest && git checkout e2239ee  && cd -  && \
    git submodule add https://github.com/google/benchmark.git extern/benchmark  && cd extern/benchmark && git checkout f91b6b4 && cd -

# Copy MOTION2NX source files
COPY CMakeLists.txt .
COPY cmake cmake
COPY src src
COPY fbs fbs
COPY Dataprovider Dataprovider
COPY flatbuffers-build flatbuffers-build
COPY circuits circuits

# AVX instruction to compile with
# Possible values : OFF, AVX, AVX2,AVX512
ARG AVX=AVX2
# Build MOTION2NX
# Delete source, build files and keep binaries
RUN CC=gcc CXX=g++ cmake \
    -B build_debwithrelinfo_gcc \
    -DCMAKE_BUILD_TYPE=DebWithRelInfo \
    -DMOTION_BUILD_EXE=On \
    -DMOTION_BUILD_TESTS=On \
    -DMOTION_USE_AVX=${AVX} && \
    cmake --build build_debwithrelinfo_gcc
RUN ldconfig
# Final Image (stage)
FROM ubuntu:22.04 as final

ENV DEBIAN_FRONTEND noninteractive

# Install package dependencies needed for motion2nx
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        g++ \
        jq \
        bc \
        net-tools \
        dnsutils \
        libicu-dev \
        openssl \
        libssl-dev \
        ca-certificates \
        language-pack-en \
        locales \
        locales-all \
        vim && \
    apt-get clean

# System locale
# Important for UTF-8
ENV LC_ALL en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US.UTF-8

# Copy libraries needed for motion2nx
COPY --from=build  /usr/local/ /usr/local/

WORKDIR /usr/share/app
# hint for volume  mount to store share files, debug files
VOLUME /usr/share/app/build_debwithrelinfo_gcc/server0
VOLUME /usr/share/app/build_debwithrelinfo_gcc/server1
# Copy motion2nx binaries from build stage
COPY --from=build /usr/share/app/build_debwithrelinfo_gcc/bin build_debwithrelinfo_gcc/bin
ENV BASE_DIR=/usr/share/app
COPY scripts scripts
COPY circuits circuits
RUN ldconfig
