FROM archlinux:base-devel

ARG UID=1000
ARG GID=1000

RUN groupadd -g "${GID}" dev \
    && useradd --create-home --no-log-init -u "${UID}" -g "${GID}" dev \
    && passwd -d dev \
    && printf 'dev ALL=(ALL) ALL\n' | tee -a /etc/sudoers

RUN pacman -Syu --noconfirm \
    && pacman -S --noconfirm \
      git libnotify spdlog luajit wayland-protocols \
      cmake ninja sccache nlohmann-json cli11 cargo

USER dev

RUN mkdir /tmp/pikaur

WORKDIR /tmp/pikaur

RUN git clone https://aur.archlinux.org/pikaur.git\
    && cd pikaur \
    && makepkg -si --noconfirm

RUN pikaur -S --noconfirm sol2 wlroots-git

WORKDIR /home/dev/ura

CMD ["/bin/bash"]
