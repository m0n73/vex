# vex
`vex` is an asynchronous reverse connection helper for SOCKS4/5. 
`vex` will connect to a remote SOCKS proxy and request it
to `BIND`. The other end of the connection will either be `vex`
binding to a port on the local machine or it connecting to
another listener.

This code is not meant to encourage you to do someting stupid,
please use `vex` for educational purposes or with the express
permission of the target system's owner.

## Building vex
```bash
# to make 
[vex@linux]$ mkdir build && make 

# to clean
[vex@linux]$ make clean
```

## Usage
```
./vex <-f proxy_list> <-l addr:port> [options]

REQUIRED:
  -f	proxy_list	Path to the proxy list (must be in
			address:port format, one per line)
  -l	addr:port	Local address and port

GENERAL OPTIONS:
  -t	addr:port	Target address [default: "255.255.255.255:0"]
  -u	username	Username for SOCKS5 authentication, or for
			the SOCKS4 userid field [default: "anonymous"]
  -b			Bind at the local address, instead of connecting
  -x	seconds		Connection/SOCKS negotiation timeout [default: 20]
  -h			Display this message

SOCKS5 OPTIONS:
  -5			Use SOCKS5 [default: SOCKS4]
  -p	password	Password for SOCKS5 authentication; this option
			enables User/Password authentication
```

`vex` expects a proxy-list text file. This file should contain addresses in
`address:port` format, one per-line.
`vex` supports domain names, IPv4, and IPv6. The latter *must* be in the
following format: `[ipv6]:port` (e.g. `[fe80::1337:2]:4201`). 
Domain names are not supported when specifying the target address (via `-t`).

## Examples

### An illustrated SOCKS5 example.

![illustrated](img/illustrated.jpg)

### Requesting BIND with a SOCKS4 proxy. 
In this example `vex` will forward all communications from the bound SOCKS4
proxy to another machine (like in the example above).
```
[vex@linux]$ ./vex -f socks4list.txt -l <another_machine_address>:9999  
[*] Trying the proxy at <proxy_address>:<proxy_port> 
[+] Connected to <proxy_address>:<proxy_port> 
[+] SOCKS4 MSG: GRANTED (<bound_proxy_address>:<bound_proxy_port>)
```
At this point the reverse connection can be initated to the proxy and will
be forwarded to `<another_machine_address>:9999`. When the reverse connection is
established you will see something like this:
```
[+] SOCKS4 MSG: GRANTED (<incoming_address>:<incoming_port>)
```

### Requesting BIND with a SOCKS5 proxy. 
In contrast to the previous examples, here `vex` will bind to a local port 
(`-b`), rather than connecting to another machine.
```
[vex@linux]$ ./vex -f socks5list.txt -l localhost:9999 -t <target_address>:0 -5 -b
[*] Trying the proxy at <proxy_address>:<proxy_port> 
[+] Connected to <proxy_address>:<proxy_port> 
[+] SOCKS5 MSG: SUCCESS (<bound_proxy_address>:<bound_proxy_port>)
```
At this point the reverse connection can be initated to the proxy and will
be forwarded via `localhost:9999`. When the reverse connection is established 
you will see something like this:
```
[+] SOCKS5 MSG: SUCCESS (<target_address>:<incoming_port>)
```
Note that SOCKS5 proxies often (if not always) requrire a legit target address in the BIND request.
