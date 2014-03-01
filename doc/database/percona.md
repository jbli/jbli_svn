percona xtradb cluster
========
介绍
----------
Percona XtraDB Cluster是在 Percona servr with XtraDB，并且包启Write Set Replication patches. 它使用Galera library3.


Galra3.x支持新特性:

- Incermental State Transfer（IST）
- RSU Rolling Schema Update. Schema change does not block operations against table


安装
-------
##须知
开放3306 4444 4567 4568端口； 禁用SELinux和apparmor

##安装命令
```
apt-key adv --keyserver keys.gnupg.net --recv-keys 1C4CBDCDCD2EFD2A

deb http://repo.percona.com/apt precise main
deb-src http://repo.percona.com/apt precise main

apt-get update
apt-get install percona-xtradb-cluster-56
```
##配置 /etc/mysql/my.cnf 
```
[mysqld]
...
wsrep_provider=/usr/lib/libgalera_smm.so
wsrep_cluster_address=gcomm://192.168.1.10,192.168.1.11,192.168.1.12
binlog_format=ROW  
default_storage_engine=InnoDB
innodb_autoinc_lock_mode=2
wsrep_node_address=192.168.1.x
wsrep_cluster_name=my_ubuntu_cluster
wsrep_sst_method=xtrabackup-v2
wsrep_sst_auth="sstuser:s3cretPass"
```
说明:
- binlog_format 为了使galera正常工作，必须用ROW
- wsrep_slave_threads=8
- wsrep_sst_method=rsync/xtrabackup-v2

##启动
第一台： /etc/init.d/mysql bootstrap-pxc <br>
创建用户

```
mysql@pxc1> CREATE USER 'sstuser'@'localhost' IDENTIFIED BY 's3cretPass';
mysql@pxc1> GRANT RELOAD, LOCK TABLES, REPLICATION CLIENT ON *.* TO 'sstuser'@'localhost';
mysql@pxc1> FLUSH PRIVILEGES;
```
其余机器启动方式: /etc/init.d/mysql start
注意： 其余结点， 要依次加入， 不要同时加入

##测试
在三台机器上每台执行一条sql

```
CREATE DATABASE percona;

USE percona;
CREATE TABLE example (node_id INT PRIMARY KEY, node_name VARCHAR(30));

INSERT INTO percona.example VALUES (1, 'percona1');
```
在任意一台上查询结果

```
SELECT * FROM percona.example;
```

同步复制的工作原理
-----
对于事务，所有操作都在本地执行，至到commit阶段，才把执行的sql分发给其它节点，经各个节点认证通过后，返回结果给第一台机，第一台机器本地提交后，整个事务就完毕了，返回应用程序结果。而各个节点在后台执行第一个节点发过来的sql.在执行期间，对节点的读请求会被要求等待，至到该事务提交成功后，再给请求返回结果，这个情况是由变量wsrep_causal_reads=ON控制。

采用 optimistic locking model, 有可能commit失败，出现deadlock 和lock Timeout,要求应用程序对返回结果进行校验，对错误返回进行处理。

SST和IST
----
- State Snapshot Transfer(SST): 可以使用三种方式mysqldump、rsync、xtrabackup,其中mysqldump、rsync需要对群集加Read Lock,xtrabackup不需要。
- Incremental State Transfer(IST): 每个节点可以缓存前N次改变， 如果一个结点需要同步的更新少于N,就可以使用IST,否则只能为SST. 其中N可配置。


wsrep status variables
--------
- wsrep_local_state_uuid: 本地状态变量，如果各台机器该值相同，则表明数据同步一致
- wsrep_cluster_state_uuid 全局的状态变量， 每一个更新， 值会改变
- wsrep_local_state_comment 如果为Synced,表明结点已经准备好处理traffic


局限性
-------------
1.目前的复制仅仅支持InnoDB存储引擎。任何写入其他引擎的表，包括mysql.*表将不会复制。但是DDL语句会被复制的，因此创建用户将会被复制，但是insert into mysql.user…将不会被复制的。

2.DELETE操作不支持没有主键的表。没有主键的表在不同的节点顺序将不同，如果执行SELECT…LIMIT… 将出现不同的结果集。

3.在多主环境下LOCK/UNLOCK TABLES不支持。以及锁函数GET_LOCK(), RELEASE_LOCK()…

4.查询日志不能保存在表中。如果开启查询日志，只能保存到文件中。

5.允许最大的事务大小由wsrep_max_ws_rows和wsrep_max_ws_size定义。任何大型操作将被拒绝。如大型的LOAD DATA操作。

6.由于集群是乐观的并发控制，事务commit可能在该阶段中止。如果有两个事务向在集群中不同的节点向同一行写入并提交，失败的节点将中止。对于集群级别的中止，集群返回死锁错误代码(Error: 1213 SQLSTATE: 40001 (ER_LOCK_DEADLOCK)).

7.XA事务不支持，由于在提交上可能回滚。

8.整个集群的写入吞吐量是由最弱的节点限制，如果有一个节点变得缓慢，那么整个集群将是缓慢的。为了稳定的高性能要求，所有的节点应使用统一的硬件。

9.集群节点建议最少3个。

10.如果DDL语句有问题将破坏集群。

待解决
=====
1. 如何把多个server组成一个cluster;如何把一个cluster降为普通server
2. 如何加一个新机器
3. 如何指定drone
4. 测试脑裂
5. 如何加监控
