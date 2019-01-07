FROM fedora:28
LABEL Description="OpenDPS firmware, built using GCC 7.1.0"

RUN dnf update -y && dnf install -y arm-none-eabi-gcc arm-none-eabi-newlib git make python sudo findutils
RUN pip install Pillow

RUN useradd -d /home/opendps -m -s /bin/bash opendps
RUN echo "opendps ALL=(root) NOPASSWD:ALL" >> /etc/sudoers

COPY build.sh /build.sh
RUN chmod 755 /build.sh

USER opendps
