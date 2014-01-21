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