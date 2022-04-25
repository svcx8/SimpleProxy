# SimpleProxy
A simple `socks5` protocol proxy server.

## Supported Features
- Authentication: none
- Protocol: TCP, UDP
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
./BinOutput/Release/Proxy
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
`v2ray configuration`
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

`command`
```shell
# nginx listening port 80
sudo systemctl start nginx

# SimpleProxy
./plow http://127.0.0.1/1mb -c 500 -n 50000 --socks5 127.0.0.1:2333 -d 10s

# v2ray
./plow http://127.0.0.1/1mb -c 500 -n 50000 --socks5 127.0.0.1:1343 -d 10s
```

`result`
```
# bench.log
~/Desktop  ./plow http://127.0.0.1/1mb -c 500 -n 50000 --socks5 127.0.0.1:2333 -d 10s
Benchmarking http://127.0.0.1/1mb with 50000 request(s) for 10s using 500 connection(s).
@ Real-time charts is listening on http://[::]:18888

Summary:
  Elapsed           10s
  Count           19172
    2xx           18991
    4xx             181
  RPS          1917.187
  Reads    1942.801MB/s
  Writes      0.107MB/s

Statistics    Min      Mean     StdDev      Max   
  Latency    864µs   50.169ms  184.078ms  3.59011s
  RPS       1032.52  1917.13     399.7    2296.51 

Latency Percentile:
  P50         P75       P90       P95       P99       P99.9     P99.99 
  23.884ms  35.261ms  51.998ms  78.99ms  1.149299s  2.540157s  3.59011s

Latency Histogram:
  31.27ms    16894  88.12%
  96.379ms    2165  11.29%
  1.501629s     46   0.24%
  1.711818s     25   0.13%
  2.116271s     19   0.10%
  2.527967s      6   0.03%
  3.341321s     13   0.07%
  3.555097s      4   0.02%
```

```
# bench_v2.log
~/Desktop  ./plow http://127.0.0.1/1mb -c 500 -n 50000 --socks5 127.0.0.1:1343 -d 10s
Benchmarking http://127.0.0.1/1mb with 50000 request(s) for 10s using 500 connection(s).
@ Real-time charts is listening on http://[::]:18888

Summary:
  Elapsed           10s
  Count           12551
    2xx           12551
  RPS          1255.071
  Reads    1281.243MB/s
  Writes      0.070MB/s

Statistics    Min      Mean      StdDev       Max   
  Latency   1.636ms  384.049ms  309.081ms  1.814186s
  RPS       868.78    1254.15    161.42     1411.17 

Latency Percentile:
  P50           P75       P90        P95        P99       P99.9     P99.99  
  309.039ms  536.101ms  853.22ms  980.234ms  1.380579s  1.787726s  1.814186s

Latency Histogram:
  146.73ms   5261  41.92%
  349.532ms  3514  28.00%
  555.62ms   1962  15.63%
  845.14ms   1207   9.62%
  1.040642s   367   2.92%
  1.268473s   154   1.23%
  1.492041s    66   0.53%
  1.702685s    20   0.16%
```

## Open Source Libraries
- [abseil](https://github.com/abseil/abseil-cpp) - Abseil Common Libraries (C++).
- [cpr](https://github.com/whoshuu/cpr) - C++ Requests: Curl for People, a spiritual port of Python Requests.
- [rapidjson](https://github.com/Tencent/rapidjson) - A fast JSON parser/generator for C++ with both SAX/DOM style API.
- [plow](https://github.com/six-ddc/plow) - A high-performance HTTP benchmarking tool with real-time web UI and terminal displaying.
