FROM --platform=linux/amd64 ubuntu:22.04 as builder

RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential cmake freeglut3-dev clang

COPY . /repo
WORKDIR /repo/build
RUN cmake ..
RUN make -j8
RUN clang++ /repo/mayhem/fuzz_vnormalize.cpp -fsanitize=fuzzer,address -I /repo/ -o /fuzz

FROM ubuntu:22.04 as package
RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y libgomp1 freeglut3-dev
COPY --from=builder /repo/build/bin/nanosg /
COPY --from=builder /fuzz /fuzz_vnormalize
