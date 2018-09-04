FROM fedora:28
LABEL Description="OpenDPS firmware, built using GCC 7.1.0" 

RUN dnf update -y && dnf install -y arm-none-eabi-gcc arm-none-eabi-newlib git make python sudo findutils

ENV user opendps
RUN useradd -d /home/$user -m -s /bin/bash $user
RUN echo "$user ALL=(root) NOPASSWD:ALL" >> /etc/sudoers

USER $user
WORKDIR /home/$user
RUN mkdir -pv code
COPY . ./code/
RUN sudo chown $user.$user -R /home/$user/code
WORKDIR /home/$user/code/
RUN git submodule init
RUN git submodule update
RUN make -j -C libopencm3
RUN make -C opendps elf bin
RUN make -C dpsboot elf bin
