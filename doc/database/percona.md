percona
========
安装
-------
```
apt-key adv --keyserver keys.gnupg.net --recv-keys 1C4CBDCDCD2EFD2A

deb http://repo.percona.com/apt precise main
deb-src http://repo.percona.com/apt precise main

apt-get update

sudo apt-get install percona-xtradb-cluster-client-5.5 \
percona-xtradb-cluster-server-5.5 percona-xtradb-cluster-galera-2.x

如果是升级，要改软件包顺序
apt-get install percona-xtradb-cluster-galera-2.x percona-xtradb-cluster-server-5.5 percona-xtradb-cluster-client-5.5
```