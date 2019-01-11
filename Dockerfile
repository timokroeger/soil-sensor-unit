FROM ubuntu:disco

COPY lib/mcuboot/scripts/requirements.txt requirements.txt

RUN apt-get update && apt-get install -y build-essentials cmake curl ninja-build python3-pip && rm -rf /var/lib/apt/lists/* && \
    pip3 install -r requirements.txt
RUN curl -L https://developer.arm.com/-/media/Files/downloads/gnu-rm/8-2018q4/gcc-arm-none-eabi-8-2018-q4-major-linux.tar.bz2 | tar -xjv --strip-components=1 -C /usr/local

WORKDIR /source

CMD bash
