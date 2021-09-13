# SimpleProxy
A simple `socks5` protocol proxy server.

## Supported Features
- Authentication: none
- Protocal: TCP
- Address Type: IPv4, IPv6, Domain
- Supports DoH (DNS over HTTPS)

## Installation
```shell
git clone --recursive https://github.com/svcx8/SimpleProxy

apt update
apt install zlib1g-dev
apt install libssl-dev
apt install libcurl4-openssl-dev

cd SimpleProxy
cmake .
make Proxy
cd BinOutput
echo '{"EnableDoH":true}' > proxy.json
./Proxy
```

## Configuration
`proxy.json` is a json format file, you can change `DoH Server` on it.
```json
{
    "EnableDoH": true,
    "DoHServer": "1.1.1.1"
    // Default address is "https://doh.pub/dns-query"
}
```

## Starting Test
```shell
# IPv4
curl -4 "http://ip.p3terx.com/" -x socks5://127.0.0.1:2333

# IPv6
curl -6 "http://ip.p3terx.com/" -x socks5://127.0.0.1:2333

# Domain
curl "https://www.bing.com/" -x socks5h://127.0.0.1:2333
```

## Benchmark
v2ray configuration
```json
"inbounds": [
    {
        "tag": "local",
        "port": 1343,
        "listen": "127.0.0.1",
        "protocol": "socks",
        "settings": {
            "auth": "noauth",
            "udp": false
        }
    }
]
```

command
```shell
# apache listening port 80
./httpd

# SimpleProxy
benchmark -ignore-err -c 500 -n 10000 -proxy socks5://127.0.0.1:2333 http://127.0.0.1 > bench.log

# v2ray
benchmark -ignore-err -c 500 -n 10000 -proxy socks5://127.0.0.1:1343 http://127.0.0.1 > bench_v2.log
```

result
```
# bench.log
Running 10000 test @ 127.0.0.1:80 by 500 connections
Request as following format:

GET / HTTP/1.1
Host: 127.0.0.1


10000 requests in 45.80s, 140.83MB read, 413.12KB write
Requests/sec: 218.36
Transfer/sec: 3.08MB
Error(s)    : 284
Percentage of the requests served within a certain time (ms)
    50%				47
    65%				55
    75%				67
    80%				74
    90%				93
    95%				1094
    98%				6947
    99%				7659
   100%				9951
```

```
# bench_v2.log
Running 10000 test @ 127.0.0.1:80 by 500 connections
Request as following format:

GET / HTTP/1.1
Host: 127.0.0.1


10000 requests in 54.20s, 140.83MB read, 448.77KB write
Requests/sec: 184.49
Transfer/sec: 2.61MB
Error(s)    : 1220
Percentage of the requests served within a certain time (ms)
    50%				53
    65%				57
    75%				62
    80%				65
    90%				112
    95%				205
    98%				6902
    99%				8822
   100%				9691
```

## Open Source Libraries
- [cpr](https://github.com/whoshuu/cpr) - C++ Requests: Curl for People, a spiritual port of Python Requests.
- [rapidjson](https://github.com/Tencent/rapidjson) - A fast JSON parser/generator for C++ with both SAX/DOM style API.
- [benchmark](https://github.com/cnlh/benchmark) - A simple benchmark testing tool implemented in golang with some small features.
