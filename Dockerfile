FROM ubuntu:16.04

RUN apt-get -y update && apt-get -y install gcc

ENV WORK /mikegus

WORKDIR $WORK/

ADD . .

RUN cc -o highload *.c

EXPOSE 80

CMD ./highload

