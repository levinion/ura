FROM archlinux:base-devel

ARG UID=1000
ARG GID=1000

RUN groupadd -g "${GID}" dev \
    && useradd --create-home --no-log-init -u "${UID}" -g "${GID}" dev \
    && passwd -d dev \
    && printf 'dev ALL=(ALL) ALL\n' | tee -a /etc/sudoers

RUN pacman -Syu --noconfirm \
    && pacman -S --noconfirm \
      git libnotify spdlog luajit wlroots0.20 wayland-protocols \
      cmake ninja sccache nlohmann-json cxxopts cargo abseil-cpp

USER dev

WORKDIR /home/dev/ura

CMD ["/bin/bash"]
