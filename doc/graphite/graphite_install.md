graphite详细安装步骤
==========================

###简述
安装 graphite-web+carbon+whisper+gdash
安装环境: ubuntu12.04 server (内核升级为3.8)

1.安装依赖
````
apt-get install python-dev  python-pip  pkg-config libcairo2-dev libapr1 libaprutil1 libaprutil1-dbd-sqlite3 build-essential libapache2-mod-wsgi libaprutil1-ldap memcached python-cairo-dev python-django python-ldap python-memcache python-pysqlite2 sqlite3 erlang-os-mon erlang-snmp rabbitmq-server bzr expect ssh libapache2-mod-python python-setuptools
```
2  下载源码包并安装
```
mkdir -p  /data/source
cd /data/source 
git clone https://github.com/graphite-project/graphite-web.git
git clone https://github.com/graphite-project/carbon.git
git clone https://github.com/graphite-project/whisper.git
```
在graphite-web、carbon、whisper三个目录分别执行下面安装命令
````
pip install -r requirements.txt
python setup.py install
```
3 carbon配置修改和启动
添加配置和创建目录
```
mkdir -p /opt/graphite/storage/log/carbon-cache
cd /opt/graphite/conf/
cp carbon.conf.example carbon.conf
cp storage-schemas.conf.example  storage-schemas.conf
```
其中/opt/graphite/conf/storage-schemas.conf做如下修改
```
[carbon]
pattern = ^carbon\.
retentions = 60:90d

[default]
pattern = .*
retentions = 60s:7d,300s:30d,900s:365d
#retentions = 60s:1d
```
注意点， retentions中后一阶段要是前一个阶段的整数倍
 ```
 Valid:    60s:7d,300s:30d (300/60 = 5)
 Invalid:  180s:7d,300s:30d (300/180 = 3.333)
```
启动carbon
/opt/graphite/bin/carbon-cache.py  start

4. 初始化数据库 参考(https://github.com/graphite-project/graphite-web/blob/master/INSTALL)
执行如下操作
```
export GRAPHITE_ROOT=/opt/graphite
export PYTHONPATH=$PYTHONPATH:$GRAPHITE_ROOT/webapp
cd $GRAPHITE_ROOT/webapp/graphite/
cd /opt/graphite/webapp/graphite
cp /data/source/graphite-web/webapp/manage.py  ./
python manage.py syncdb
```
设置密码 root/1234

5. graphite-web配置和启动
```
cp  /opt/graphite/webapp/graphite/local_settings.py.example /opt/graphite/webapp/graphite/local_settings.py
```
并做如下修改
```
TIME_ZONE = 'Asia/Chongqing’
```
如果使用memcached做graphite-web展示时的缓存，添加下面一行，不过我添加后更新好像不正常
```
MEMCACHE_HOSTS = ['127.0.0.1:11211’]
```
启动graphite-web
gunicorn_django -b 192.168.1.100:7880     /opt/graphite/webapp/graphite/settings.py

6  安装
wget -O numpy-1.8.0.tar.gz  http://sourceforge.net/projects/numpy/files/NumPy/1.8.0/numpy-1.8.0.tar.gz/download
解压后python setup.py install
如果不安装该模块，图片显示不出来， 页面并报“ImportError: No module named numpy”

7 安装gdash
```
git clone https://github.com/ripienaar/gdash.git
vi Gemfile
source 'http://ruby.taobao.org/'

apt-get install rubygems ruby-bundler
bundle install —deployment  
cd /data/source/gdash/config
cp gdash.yaml-sample  gdash.yaml
```
修改如下部分
````
:graphite: http://123.123.1.3:9002
:prefix: "/dashboard"
:whisper_dir: "/opt/graphite/storage/whisper"
```
简单启动,在前台
```
/data/source/gdash
bundle exec rackup  
```
写一个脚本start.sh来启动
```
#! /bin/sh
cd /data/source/gdash
bundle exec /data/source/gdash/vendor/bundle/ruby/1.8/bin/rackup 1>/dev/null 2>&1 -D
```

启动后发现有一个css不是从本地服务器下载，而是从code.jquery.com下载，把它下载到本地服务器保存。方法如下:
- 先把code.jquery.com/ui/1.10.0/themes/smoothness/jquery-ui.css保存在/data/source/gdash/public/js目录下
- 然后把/data/source/gdash/config/view/layout.erb
```
<link rel="stylesheet" type="text/css" href="//code.jquery.com/ui/1.10.0/themes/smoothness/jquery-ui.css”>
```
改为
```
<link rel="stylesheet" type="text/css" href="<%= @prefix %>/js/jquery-ui.css”>
```

8 nginx做前面代理
在nginx配置中加入下内容
```
  server {
        listen       9002;
        server_name 123.103.16.116;
         location / {
        auth_basic      "Xueqiu Access";
        auth_basic_user_file /data/server/ngx_openresty-1.4.2.5/nginx/conf/auth;
            proxy_pass_header Server;
            proxy_set_header Host $http_host;
            proxy_redirect off;
            proxy_set_header X-Real-IP $remote_addr;
            proxy_set_header X-Scheme $scheme;
            proxy_connect_timeout 10;
            proxy_read_timeout 30;
                proxy_pass http://backend_graphite;
        }
        location /dashboard/ {
          auth_basic      "Xueqiu Access";
          auth_basic_user_file /data/server/ngx_openresty-1.4.2.5/nginx/conf/auth;
          proxy_pass_header Server;
          proxy_set_header Host $http_host;
          proxy_redirect off;
          proxy_set_header X-Real-IP $remote_addr;
          proxy_set_header X-Scheme $scheme;
          proxy_connect_timeout 10;
          proxy_read_timeout 30;
          rewrite ^/dashboard/(.*)$  /$1  break;
          proxy_pass http://192.168.1.46:9292;
       }
    }
```