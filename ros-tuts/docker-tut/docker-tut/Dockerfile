#Add whereever you want to
FROM osrf/ros:humble-desktop-full

RUN apt-get update && apt-get install -y nano && rm -rf /var/lib/apt/lists/*

COPY config/ /site_config/
    
ARG USERNAME=ros
ARG USER_UID=1000
ARG USER_GID=${USER_UID}

RUN groupadd --gid ${USER_GID} ${USERNAME} && \
    useradd -s /bin/bash --uid ${USER_UID} --gid ${USER_GID} -m ${USERNAME} && \
    mkdir -p /home/${USERNAME}/.config && \
    chown ${USER_UID}:${USER_GID} /home/${USERNAME}/.config

#Use apt-get instead of apt as more reliable in scripts

#Updated bc there is a chance it doesnt run the stuff it thinks it doesnt need

#backslash lets you split a command on multiple lines, can also pin point error

#rm -rf deletes the package list as it saves on size

# Set up sudo
RUN apt-get update \
    && apt-get install -y sudo \
    && echo "$USERNAME ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME \
    && rm -rf /var/lib/apt/lists/*

COPY entrypoint.sh /entrypoint.sh

ENTRYPOINT ["/bin/bash", "/entrypoint.sh"]

CMD ["bash"]