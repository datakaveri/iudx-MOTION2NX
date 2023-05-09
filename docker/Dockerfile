FROM ghcr.io/datakaveri/motion2nx:build as build   
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
FROM ubuntu:jammy-20230425 as final

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
# hint for volume  mount to store log files
VOLUME /usr/share/app/logs
# Copy motion2nx binaries from build stage
COPY --from=build /usr/share/app/build_debwithrelinfo_gcc/bin build_debwithrelinfo_gcc/bin
ENV BASE_DIR=/usr/share/app
COPY scripts scripts
COPY circuits circuits
RUN ldconfig
