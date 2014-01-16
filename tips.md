常用小技巧
==========================

###文件传输
使用nc
```
nc -l 5555 | tar xvf -
tar -cf -   hekad | nc 192.168.1.44 5555   
```
使用ftp
```
ftp -n 192.168.1.32 7521 <<_EOF
user lijinbang  qwer@1234
prompt off
bin
put tengine-extras_1.5.1-1_amd64.deb
bye
_EOF
```
####系统诊断
tcpdump 抓包
```
 tcpdump -s 0 -iem1 host 8.8.8.8 and  dst port 80 -w tcpbao.cap  
 nohup tcpdump -s 0 -i any host ! 10.1.1.40 and dst port 8080 -w tcpbao_8080  1>8080.log 2>&1 &
```
统计连接的各种状态
```
netstat -n 2>/dev/null | awk '/^tcp/ {++state[$NF]} END {for(key in state) print key,"\t",state[key]}'
```
或者
```
ss state ESTABLISHED|wc -l
ss state TIME-WAIT|wc -l
ss -t -a -o excl ESTABLISHED |grep -v TIME-WAIT
ss state ESTABLISHED sport eq :80 and not dst 127.0.0.1
```
得到crontab
crontab -l|awk '{print $7}'|awk -F')' '{ if($1~/home/) print $1,"\\"}'

###lvs
```
ipvsadm -L -n -t 10.110.10.188:7004
ipvsadm -e -t 10.110.10.186:7004 -r 10.110.10.27:7004 -w 0
ipvsadm -e -t 10.110.10.188:7004 -r 10.110.10.27:7004 -w 0
```
使用pidstat -d 1 查看进程请求io信息
使用strace查看进程在读写哪个文件描述符
查看/proc/PID/fd目录下，该文件描述符对应的文件名

#### 用户管理
转换Key
```
 ssh-keygen -i -f Identity.pub > authorized_keys
 ```
 创建新用户
```
username=xxx
useradd -s /bin/bash -m $username &&cd /home/$username  && mkdir .ssh && cd .ssh
echo "xx">/home/$username/.ssh/authorized_keys
cd .. && chmod 700 .ssh && chmod 600 .ssh/authorized_keys && chown -R $username:$username .ssh 
```

###日志查询
查看某个目录下所有日志文件
```
find . -name '*.log' | xargs tail -F
```
awk传参
```
echo | vnstat -tr 5 -i eth1 |egrep 'rx|tx'|awk  '{print tm,$1,$2,$3,$4,$5,tm1}' tm="$now" tm1="$now1"
```

####测试
window 网络
```
tracert 220.181.99.41
```
对于限定域名的服务器访问特定前台
```
curl -v -s -H"Host: www.xx.com" "http://8.8.8.8/arg" >/dev/null 
```

### 限流措施
iptables
```
iptables -A INPUT -i eth0 -s 192.168.100.250 -d 192.168.100.1 -p tcp --dport 22 -j ACCEPT
-A INPUT -s 218.107.55.172 -i eth0 -p tcp -m tcp --dport 80 -j DROP
```
nginx 过滤请求
```
        if ($query_string ~ product=cloudstorage) {
                return 403;
        }
```

###  vim
取消自动缩进 :set noautoindent

