FROM ubuntu:16.04

RUN apt-get -y update && apt-get -y install gcc

RUN cc -o highload *.c

EXPOSE 80

CMD ./highload

