FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y \
    gcc \
    make \
    libpam0g-dev \
    libssl-dev \
    pkg-config \
    binutils \
    file

WORKDIR /src
COPY . .

CMD ["bash", "build.sh"]
