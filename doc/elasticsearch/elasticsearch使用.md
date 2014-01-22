elasticsearch

======================
安装
下载源码版elasticsearch

```
apt-get install openjdk-6-jdk
启动
bin/elasticsearch -Xmx2g -Xms2g -Des.index.store.type=memory --node.name=my-node

```

配置
=======================

指定配置文件位置

```
elasticsearch -Des.config=/path/to/config/file
```
内存

- 启用mlockall
  修改config/elasticsearch.yml中bootstrap.mlockall为true,它可以防止swap分区交换。 另外， 要保证允许该进程锁住内存，如通过`ulimit -l unlimited`命令,同时也要保证ES_MIN_MEM和ES_MAX_MEM 相等，可以通过-Xmx和-Xms指定。另，-Xmn代表ES_HEAP_NEWSIZE(新生代)。  如果设置 ES_HEAP_NEWSIZE的值，相当于设置-Xmx和-Xms为相同值。
  
注意： 如果不设置ES_USE_GC_LOGGING，在elasticsearch.in.sh中默认设置loggc到/var/log/elasticsearch/gc.log,可以做如下修改

```
把
JAVA_OPTS="$JAVA_OPTS -Xloggc:/var/log/elasticsearch/gc.log"
修改为
JAVA_OPTS="$JAVA_OPTS -Xloggc:$ES_HOME/logs/elasticsearch/gc.log"
 ```

linux系统设置 max_file_descriptors为32k或64k,可以通过下面命令来验证

```
curl localhost:9200/_nodes/process?pretty
```
  
设置 path，同时创建plugins和work两个子目录

```
path.conf: /data/elasticsearch/conf
path.data: /data/elasticsearch/data
path.work: /data/elasticsearch/work
path.logs: /data/elasticsearch/logs
path.plugins: /data/elasticsearch/plugins
```

集群名 cluster.name

设置节点名 node.name， 默认为动态分配一个。

node.master 该节点是否可以被选举为master
node.data   该节点是否可以存储数据


启动
===========================
```
/data/elasticsearch/bin/elasticsearch -Xmx2g -Xms2g -Des.index.store.type=memory --node.name=my-node -Des.config=/data/elasticsearch/config/elasticsearch.yml 
```

查询
============================
curl localhost:9200/_nodes/process?pretty

GET /_cluster/state/nodes

GET /_nodes/stats/transport,http

在查询url后加?pretty=true或fromat=yaml，可以使结果更清晰可读<br/>

知识点
=========================
##multiple indices
形式如：

- test1,test2,test3
- _all
- test*
- +test*,-test3

支持参数:

- ignore_unavailable true/false 是否忽略无效或正在关闭的indices
- allow_no_indices true/false 是否允许没一个符合条件的indices
- expand_wildcards open/closed  ?

##common options
### pretty results
两种方式:

- ?pretty=true  注意仅在debug情况下使用
- format=yaml 

###human readable output
- 人可读如 1h, 1K, 机器可读如 36000,1024等，机器可读主要用于监控等场合
- ?human=false/true 控制输出形式，默认是false

###flat settings
- flat_settings is true

```
{
  "persistent" : { },
  "transient" : {
    "discovery.zen.minimum_master_nodes" : "1"
  }
}
```
- flat_settings is flase,默认值为false

```
{
  "persistent" : { },
  "transient" : {
    "discovery" : {
      "zen" : {
        "minimum_master_nodes" : "1"
      }
    }
  }
}
```

###parameters
Rest parameters 转换为url parameters时，用下划线“_”

###boolean values
false,0,no,off代表false, 其余都代表true

###number values
支持number作为string类型

###time units
时间段，像timeout参数，支持2d for 2 days

- y  Year
- M  Month
- W  Week
- d   Day
- h   Hours
- m   Minute
- s   Second

###distance units
###fuzziness
支持模糊查询<br/>
使用 fuzziness参数，对内容查询，依赖被查询的field类型，其中field类型有 string numeric,date,ipv4

###result casing
当参数case=camelCase，返回camel casing,否则返回下划线风格

###jsonp

###request body in query string
不接请求体为non-POST的请求，但是能传递请求体做为source query string parameter代替。

##document apis
主要描述CRUD APIs<br/>
Single document APIs

- Index API
- Get API
- Delete API
- Update API

Multi-document APIs

- Multi Get API
- Bulk API
- Bulk UDP API
- Delete By Query API

注意：所有CRUD APIs都是single-index, index parameter只接受single index name

###Index API
对文档增加或更新一个索引， index:"twitter";type:"tweet";id:"1"

```
curl -XPUT 'http://localhost:9200/twitter/tweet/1' -d '{
    "user" : "kimchy",
    "post_date" : "2009-11-15T14:12:12",
    "message" : "trying out Elasticsearch"
}'
```
返回结果

```
{
    "_index" : "twitter",
    "_type" : "tweet",
    "_id" : "1",
    "_version" : 1,
    "created" : true
}
```

###automatic index creation
index、type如果不存在，会自动创建。<br/>
可以通过action.auto_create_index和index.mapper.dynamic设为false，来禁用自动创建。<br/>
action.auto_create_index也可以使用`+aaa*,-bbb*,+ccc*,-*`,其中+表示允许，-表示禁止.

action.auto_create_index和index.mapper.dynamic设置可以在配置文件，可以针对特定的index.如：

```
curl -XPOST 'http://localhost:9200/twitter' -d '{
  "settings": {
    "index": {
      "mapping.allow_type_wrapper": true
    }
  }
}'
```
返回结果:

```
{"acknowledged":true}
```
###version
文档每次更新操作都会更新版本号。也可以指定更新版本号

```
curl -XPUT 'localhost:9200/twitter/tweet/1?version=2' -d '{
    "message" : "elasticsearch now has versioning support, double cool!"
}'
```
###operation type
op_type 可以用于创建操作，如果索引已存在，则返回失败。

```
curl -XPUT 'http://localhost:9200/twitter/tweet/1?op_type=create' -d '{
    "user" : "kimchy",
    "post_date" : "2009-11-15T14:12:12",
    "message" : "trying out Elasticsearch"
}'
```
或者写成如下形式:

```
curl -XPUT 'http://localhost:9200/twitter/tweet/1/_create' -d '{
    "user" : "kimchy",
    "post_date" : "2009-11-15T14:12:12",
    "message" : "trying out Elasticsearch"
}'
```
###automatic id generation

```
curl -XPOST 'http://localhost:9200/twitter/tweet/' -d '{
    "user" : "kimchy",
    "post_date" : "2009-11-15T14:12:12",
    "message" : "trying out Elasticsearch"
}'
```
注意要使用 "POST" 代替 “PUT”

### routing
默认基于document’s id 哈希分片存储。 可以使用routing来指定一个值，根据该值来哈希。注意， 如果存储是基于该值存储，那么读取也指定该routing,否则读不到值。

```
url -XPOST 'http://localhost:9200/twitter/tweet?routing=kimchy' -d '{
    "user" : "kimchy",
    "post_date" : "2009-11-15T14:12:12",
    "message" : "trying out Elasticsearch"
}'
```



1.0与0.9版本变化
============================

- text 格式移除，用match 代替
- field 移除，用query_string 代替
- _boost移除，用function_score 代替




===========================================

- 加入开机自启动

NOT starting elasticsearch by default on bootup, please execute
 sudo update-rc.d elasticsearch defaults 95 10

- 启动
In order to start elasticsearch, execute
 sudo /etc/init.d/elasticsearch start
 
- go客户端

https://github.com/mattbaird/elastigo

https://github.com/olivere/elastic