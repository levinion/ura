FROM archlinux:base-devel

ARG UID=1000
ARG GID=1000

RUN pacman -Syu --noconfirm \
    && pacman -S --noconfirm \
      git libnotify spdlog wlroots0.19 luajit \
      cmake ninja sccache nlohmann-json cli11 cargo \
    && groupadd -g "${GID}" dev \
    && useradd --create-home --no-log-init -u "${UID}" -g "${GID}" dev

USER dev

WORKDIR /home/dev/ura

CMD ["/bin/bash"]
