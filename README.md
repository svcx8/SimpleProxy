# SimpleProxy
A simple `socks5` protocol proxy server.

## Supported Features
- Authentication: none
- Protocol: TCP
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
# apache listening port 8080
./httpd

# SimpleProxy
./plow http://127.0.0.1:8080/ram/1m -c 500 -n 10000 --socks5 127.0.0.1:2333 -d 10s > bench.log

# v2ray
./plow http://127.0.0.1:8080/ram/1m -c 500 -n 10000 --socks5 127.0.0.1:1343 -d 10s > bench_v2.log
```

`result`
```
# bench.log
Benchmarking http://127.0.0.1:8080/ram/1m with 10000 request(s) for 10s using 500 connection(s).
@ Real-time charts is listening on http://[::]:18888

Summary:
  Elapsed      10.001s
  Count           9921
    2xx           9921
  RPS          991.933
  Reads    977.873MB/s
  Writes     0.063MB/s

Statistics    Min      Mean      StdDev      Max   
  Latency   1.032ms  318.795ms  1.11364s  9.869586s
  RPS        69.1     1098.66    658.55    1861.32 

Latency Percentile:
  P50         P75        P90       P95        P99      P99.9    P99.99  
  47.974ms  78.533ms  145.726ms  1.71303s  5.174249s  8.1733s  9.869586s

Latency Histogram:
  77.486ms   9435  95.10%
  1.955322s    21   0.21%
  3.233196s    62   0.62%
  4.004592s     7   0.07%
  5.138695s   347   3.50%
  7.263726s    28   0.28%
  8.287944s    20   0.20%
  9.869586s     1   0.01%
```

```
# bench_v2.log
Benchmarking http://127.0.0.1:8080/ram/1m with 10000 request(s) for 10s using 500 connection(s).
@ Real-time charts is listening on http://[::]:18888

Summary:
  Elapsed      10.001s
  Count           9983
    2xx           9983
  RPS          998.160
  Reads    952.118MB/s
  Writes     0.066MB/s

Statistics    Min      Mean     StdDev     Max   
  Latency   9.777ms  427.182ms  530.6ms  4.28179s
  RPS       682.29    1101.83   184.32   1247.55 

Latency Percentile:
  P50           P75        P90        P95       P99       P99.9     P99.99 
  186.204ms  505.574ms  996.951ms  2.002762s  2.44607s  2.710178s  4.28179s

Latency Histogram:
  141.882ms  5215  52.24%
  350.271ms  2260  22.64%
  543.187ms  1015  10.17%
  869.664ms   599   6.00%
  1.494362s   470   4.71%
  2.214105s   359   3.60%
  2.476924s    53   0.53%
  2.689832s    12   0.12%
```

## Open Source Libraries
- [cpr](https://github.com/whoshuu/cpr) - C++ Requests: Curl for People, a spiritual port of Python Requests.
- [rapidjson](https://github.com/Tencent/rapidjson) - A fast JSON parser/generator for C++ with both SAX/DOM style API.
- [benchmark](https://github.com/six-ddc/plow) - A high-performance HTTP benchmarking tool with real-time web UI and terminal displaying.
