FROM ubuntu:16.04

RUN apt-get -y update && apt-get -y install gcc

ADD .

RUN cc -o highload *.c

EXPOSE 80

CMD ./highload

