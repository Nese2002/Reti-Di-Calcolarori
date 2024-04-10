FROM gcc:latest as builder
COPY . /usr/src/myapp
WORKDIR /usr/src/myapp
RUN gcc -o b_client_posix -static b_client_posix.c

FROM alpine:latest as runtime
COPY --from=builder /usr/src/myapp/b_client_posix /b_client_posix
ENTRYPOINT ["./b_client_posix"]

