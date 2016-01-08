# SocketCan <=> auml tcp proxy

can_proxy is a network proxy between linux SocketCan framework, and the auml syntax of packing can packets in tcp connections

can_proxy can be used in two modes: a server, accepting tcp connections, or a client to connect to a auml can daemon (or another can_proxy)

Useful to integrate [Projekt Auml](http://projekt.auml.se) with [SocketCan](https://en.wikipedia.org/wiki/SocketCAN)

Also useful to bridge two socket can connections on different machines, for example have a can interface connected to one machine, and bring that interface to another machine using vcan and userspace daemons.

## Building

```bash
autoreconf -i
./configure
make
```

## Examples

For most users, create a vcan0 interface:
```bash
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```

Start a server exposing vcan0:
```bash
./can_proxy -i vcan0 -l -p 1200
```

Link vcan0 to a remote server:
```bash
./can_proxy -i vcan0 -a 129.168.56.1 -p 1200
```
