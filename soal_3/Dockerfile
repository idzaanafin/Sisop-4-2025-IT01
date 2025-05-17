FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y \
    gcc make libfuse3-dev fuse3 \
    && rm -rf /var/lib/apt/lists/*



COPY src/antink.c /app/antink.c

WORKDIR /app


RUN gcc antink.c -o antink -lfuse3


CMD bash -c "mkdir -p /antink_mount && ./antink /antink_mount & tail -f /var/log/it24.log"




