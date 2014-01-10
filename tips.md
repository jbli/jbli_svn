常用小技巧
==========================

###文件传输
-使用nc
```
nc -l 5555 | tar xvf -
tar -cf -   hekad | nc 192.168.1.44 5555   
```
-使用ftp
```
ftp -n 192.168.1.32 7521 <<_EOF
user lijinbang  qwer@1234
prompt off
bin
put tengine-extras_1.5.1-1_amd64.deb
bye
_EOF
```

###日志查询
-查看某个目录下所有日志文件
```
find . -name '*.log' | xargs tail -F
```

