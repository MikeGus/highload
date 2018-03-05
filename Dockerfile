FROM alpine

RUN apk add --no-cache build-base

ADD . .

RUN gcc -o highload *.c

EXPOSE 80

CMD ./highload

