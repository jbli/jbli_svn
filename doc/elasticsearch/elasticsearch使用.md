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

设置udp监听端口

```
bulk.udp.enabled: true
bulk.udp.port: 9700
```

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

###write consistency
通过action.write_consistency设置写几个分片后，返回成功。可以设为one, quorum,all. 其中默认值为quorum(>replicas/2+1)

###asynchronous repliacation
默认当all shards都被复制后才返回。 可以通过设置replication为async，当primary shard操作成功后就返回。

###refresh
refresh true， 会影响性能

###timeout
timeout: 操作等待返回的时间，默认是1m.

```
curl -XPUT 'http://localhost:9200/twitter/tweet/1?timeout=5m' -d '{
    "user" : "kimchy",
    "post_date" : "2009-11-15T14:12:12",
    "message" : "trying out Elasticsearch"
}'
```

-----------------------------
##get api

```
curl -XGET 'http://localhost:9200/twitter/tweet/1'
````

###realtime
默认get API是实时的，不受刷新频率的影响。可以通过realtime=false或全局变量action.get.realtime＝false ，取到非实时数据，数据实时性受刷新频率控制

###optional type
_type=_all  在所有type中获取第一个匹配id的文档

###source filtering
默认返回整个_source内容，可以设置_source=false不返回_source内容

```
curl -XGET 'http://localhost:9200/twitter/tweet/1?_source=false'
```

可以通过 _source_include & _source_exclude 来过滤，如

```
curl -XGET 'http://localhost:9200/twitter/tweet/1?_source_include=*.id&_source_exclude=entities'
```
如果只有include,可以直接使用

```
curl -XGET 'http://localhost:9200/twitter/tweet/1?_source=message,*ser'
```
###fields
被 source filtering取代

###getging _source directly

```
curl -XGET 'http://localhost:9200/twitter/tweet/1/_source'
```

或

````
curl -XGET 'http://localhost:9200/twitter/tweet/1/_source?_source_include=*.id&_source_exclude=entities'
````

###routing
?routing=kimchy

###preference
preference 参数可以如下值：

- _primary 仅从primary shards查询
- _local 尽可能从本地取值
-  自定义字符串 作用类同web session id或user name

###refresh
###distributed

------------------------
##delete api

```
curl -XDELETE 'http://localhost:9200/twitter/tweet/1'
```

###versioning
###routing
###parent
###automatic index creation
delete 操作会自动创建index和type，如果它们原来不存在 ？？？？
###distributed
根据hash得到shard id,先删primary shard,再同步到整个id group
###replication type
replication=async/sync  是删除完primary shard上内容直接返回，再异步删整个id group,还是删完整个id group再返回。
###write consistency
consistency=one/quorum/all
###frefresh
###timeout

--------------------
##update api
update 是先get document,运行script,再返回结果。该操作完全是reindex document,但它比get delete get减少网络开销和version 冲突。<br/>
例子：

```
curl -XPUT localhost:9200/test/type1/1 -d '{
    "counter" : 1,
    "tags" : ["red"]
}'
```
更新counter

```
curl -XPOST 'localhost:9200/test/type1/1/_update' -d '{
    "script" : "ctx._source.counter += count",
    "params" : {
        "count" : 4
    }
}'
```
查看结果

```
curl -XGET 'localhost:9200/test/type1/1?pretty' 
```
如下所示

```
{
  "_index" : "test",
  "_type" : "type1",
  "_id" : "1",
  "_version" : 2,
  "found" : true, "_source" : {"counter":5,"tags":["red"]}
}
```
为数组中加一个元素

```
curl -XPOST 'localhost:9200/test/type1/1/_update' -d '{
    "script" : "ctx._source.tags += tag",
    "params" : {
        "tag" : "blue"
    }
}'
```
添加一个新参数

```
curl -XPOST 'localhost:9200/test/type1/1/_update' -d '{
    "script" : "ctx._source.remove(\"text\")"
}'
```
三元操作

```
// Will update
ctx._source.tags.contains(tag) ? (ctx.op = \"none\") : ctx._source.tags += tag
// Also works
if (ctx._source.tags.contains(tag)) { ctx.op = \"none\" } else { ctx._source.tags += tag }

```
也支持传入部分key/valuve,和原来的融合在一起

```
curl -XPOST 'localhost:9200/test/type1/1/_update' -d '{
    "doc" : {
        "name" : "new_name"
    }
}'
```
upsert 可以检查指定字段是否已存在，如果不存在，赋一个初值。

```
curl -XPOST 'localhost:9200/test/type1/1/_update' -d '{
    "script" : "ctx._source.counter += count",
    "params" : {
        "count" : 4
    },
    "upsert" : {
        "counter" : 1
    }
}'
```

doc_as_upsert也可以先判断是否存在，如果不存在更新。

```
curl -XPOST 'localhost:9200/test/type1/1/_update' -d '{
    "doc" : {
        "name" : "new_name"
    },
    "doc_as_upsert" : true
}'
```
------------------------
##multi get api
形式一：

```
curl 'localhost:9200/_mget' -d '{
    "docs" : [
        {
            "_index" : "test",
            "_type" : "type",
            "_id" : "1"
        },
        {
            "_index" : "test",
            "_type" : "type",
            "_id" : "2"
        }
    ]
}'
```
形式二：

```
curl 'localhost:9200/test/_mget' -d '{
    "docs" : [
        {
            "_type" : "type",
            "_id" : "1"
        },
        {
            "_type" : "type",
            "_id" : "2"
        }
    ]
}'
```
形式三:

```
curl 'localhost:9200/test/type/_mget' -d '{
    "docs" : [
        {
            "_id" : "1"
        },
        {
            "_id" : "2"
        }
    ]
}'
```
可以写为

```
curl 'localhost:9200/test/type/_mget' -d '{
    "ids" : ["1", "2"]
}'
```

###source filtering
```
curl 'localhost:9200/_mget' -d '{
    "docs" : [
        {
            "_index" : "test",
            "_type" : "type",
            "_id" : "1",
            "_source" : false
        },
        {
            "_index" : "test",
            "_type" : "type",
            "_id" : "2",
            "_source" : ["field3", "field4"]
        },
        {
            "_index" : "test",
            "_type" : "type",
            "_id" : "3",
            "_source" : {
                "include": ["user"],
                "_exclude": ["user.location"]
            }
        }
    ]
}'
```

--------
##bulk api
操作对象是index,可以批量地create delete和update在一次API调用。

##delete by query api
```
$ curl -XDELETE 'http://localhost:9200/twitter/tweet/_query?q=user:kimchy'

$ curl -XDELETE 'http://localhost:9200/twitter/tweet/_query' -d '{
    "term" : { "user" : "kimchy" }
}
'
$ curl -XDELETE 'http://localhost:9200/kimchy,elasticsearch/_query?q=tag:wow'
```

1.0与0.9版本变化
============================

- text 格式移除，用match 代替
- field 移除，用query_string 代替
- _boost移除，用function_score 代替


其它
============================
- [elasticsearch下数据可视化](http://www.elasticsearch.org/blog/data-visualization-with-elasticsearch-and-protovis/)

- 加入开机自启动

NOT starting elasticsearch by default on bootup, please execute
 sudo update-rc.d elasticsearch defaults 95 10

- 启动
In order to start elasticsearch, execute
 sudo /etc/init.d/elasticsearch start
 
- go客户端

https://github.com/mattbaird/elastigo

https://github.com/olivere/elastic