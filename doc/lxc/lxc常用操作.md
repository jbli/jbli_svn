lxc
===================
##安装
apt-get install lxc
##检查
lxc-checkconfig

##配置文件
/etc/lxc/lxc.conf
##模板
mkdir /data/lxc
mkdir /data/lxc/lxclib /data/lxc/lxccache
cd /usr/lib/lxc/templates
cp lxc-ubuntu lxc-xueqiu
##创建 
lxc-create -t xueqiu -n xueqiu --  --bindhome lijinbang --auth-key /home/lijinbang/.ssh/authorized_keys
##修改ip
vi /var/lib/lxc/nodejs/rootfs/etc/network/interfaces 
##启动
lxc-start -o /tmp/xueqiu01.debug -l DEBUG -n xueqiu01
lxc-start -n xueqiu
##停止
lxc-stop -n xueqiu01
##销毁
lxc-destroy -n xueqiu01

##添加开机自启动
ln -s /var/lib/lxc/imdb/config /etc/lxc/auto/imdb.conf


======================
#问题
1. 在lxc 中挂载nfs,报如下错：

```
mount: block device dm:/ is write-protected, mounting read-only
mount: cannot mount block device dm:/ read-only
```

解决办法：

在宿主机/etc/apparmor.d/lxc/lxc-default中加入下面三行

```
mount fstype=nfs,
mount fstype=nfs4,
mount fstype=rpc_pipefs,
```
重启服务
/etc/init.d/apparmor restart

问题二

```
If
 you get this error when running lxc-start ...
lxc-start: No such file or directory - failed to change apparmor profile to lxc-container-default
... add this to your container's config file
lxc.aa_profile = unconfined


error message " lxc-start: Permission denied - failed to mount /proc in the container.
lxc-start: failed to setup the container
lxc-start: invalid sequence number 1. expected 2
lxc-start: failed to spawn 'left' "
```

解决：

I have solved the problem by the following 2 commands 
sudo apparmor_parser -R /etc/apparmor.d/usr.bin.lxc-start
sudo ln -s /etc/apparmor.d/usr.bin.lxc-start /etc/apparmor.d/disabled/