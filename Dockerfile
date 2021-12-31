FROM ubuntu:focal

ARG REPO="http://mirrors.tuna.tsinghua.edu.cn"
ARG TIMEZONE="Asia/Shanghai"
ARG X=xxx-xxx.tar.gz

WORKDIR /

RUN if [ -n "$REPO" ]; then sed -i "s%https\\?://\\(archive\\|security\\).ubuntu.com%${REPO}%g" /etc/apt/sources.list; fi && \
    apt-get update --error-on=any
RUN ln -fs /usr/share/zoneinfo/"$TIMEZONE" /etc/localtime && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y tzdata && \
    dpkg-reconfigure --frontend noninteractive tzdata

RUN apt-get install -y build-essential
RUN apt-get install -y bc

COPY $X /

ENV X ${X}
CMD ["bash", "-c", "tar zxvf $X && cd /turing-project && make -j6 test"]
