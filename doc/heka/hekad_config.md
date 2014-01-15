hekad
===========================
###安装
- apt-get install cmake mercurial  build-essential
- go 1.1

```
export GOROOT=/root/go  
export PATH=$PATH:$GOROOT/bin
export GOPATH=/root/golibs  
```
- git clone https://github.com/mozilla-services/heka.git
- 在/data/heka/cmake/externals.cmake 中加入自定义的插件地址

```
if (INCLUDE_MOZSVC)
    add_external_plugin(git https://github.com/mozilla-services/heka-mozsvc-plugins 6fe574dbd32a21f5d5583608a9d2339925edd2a7)
    add_external_plugin(git https://github.com/jbli/heka_plugin 3338501c72d599858262e867d46abca9a6ccbd62)
endif()
```

- cd heka && sh build.sh

### 配置文件
- 全局配置

```
[hekad]
cpuprof = "/var/log/hekad/cpuprofile.log"
decoder_poolsize = 10
max_message_loops = 4
max_process_inject = 10
max_timer_inject  = 10
maxprocs = 10
memprof = "/var/log/hekad/memprof.log"
plugin_chansize = 10
poolsize = 100
```
- 文件输入输出

```
[aabb]
type = "LogfileInput"
logfile = "/tmp/aabb.log"
logger = "apache_test"

[access_xueqiu]
type = "FileOutput"
message_matcher = "Type != 'ablogfile'"
path = "/tmp/access_xueqiu.log"
```

- nsq

```
[NsqInput_1]
type = "NsqInput"
address = "192.168.1.44:4161"
topic = "test"
channel = "test"
decoder = "ProtobufDecoder"
```

- nginx日志过滤

```
[aabb]
type = "LogfileInput"
logfile = "/tmp/aabb.log"
decoder = "apache_transform_decoder"
logger = "apache_test"

[apache_transform_decoder]
type = "PayloadRegexDecoder"
match_regex = '^(?P<RemoteIP>\S+) \S+ \S+ \[(?P<Timestamp>[^\]]+)\] "(?P<Method>[A-Z]+) (?P<Url>[^\s^\?]+)[^"]*" (?P<StatusCode>\d+) (?P<RequestSize>\d+) "(?P<Referer>[^\s^\?]+)[^"]*"' 
timestamp_layout = "02/Jan/2006:15:04:05 +0800"

[apache_transform_decoder.message_fields]
Type = "ApacheLogfile"
Logger = "apache"
RemoteIP = "%RemoteIP%"
Timestamp = "%Timestamp%"
Url|uri = "%Url%"
Status = "%StatusCode%"
RequestSize|B = "%RequestSize%"
Referer = "%Referer%"
```

### 注意事项
- LogfileInput类型文件信息存储在/var/cache/hekad/seekjournals/目录下。


### sandbox

- 加载和卸载

```
./heka-sbmgr -action="load" -config=PlatformDevs.toml -script="example.lua" -scriptconfig="example.toml"
./heka-sbmgr -action="unload" -config=PlatformDevs.toml -filtername=Example 
```
- 添加测试数据

```
./heka-inject   -heka="127.0.0.1:5565" -payload="testlua456"
```