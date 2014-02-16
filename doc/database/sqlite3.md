sqlite3 使用
====
##安装

```
sudo apt-get install sqlite3  
sudo apt-get install libsqlite3-dev
```

##创建数据库或打开数据库

```
sqlite3 test.db 
```

##创建表

```
sqlite> create table mytable(id integer primary key, value text);  
```

##插入数据

```
sqlite> insert into mytable(id, value) values(1, 'Micheal');  
sqlite> insert into mytable(id, value) values(2, 'Jenny');  
sqlite> insert into mytable(value) values('Francis');  
```

##查询

```
sqlite> select * from test;  
```

设置格式化查询结果

```
sqlite> .mode column;  
sqlite> .header on;  
sqlite> select * from test;  	
```

##显示表结构：
```
sqlite> .schema [table] 
```
##获取所有表和视图：
```
sqlite > .tables 
```
##获取指定表的索引列表：

```
sqlite > .indices [table ] 
```

##改变表结构
```
sqlite> alter table mytable add column email text not null '' collate nocase;; 
```
##创建索引
```
sqlite> create index test_idx on mytable(value); 
```

##创建视图
```
sqlite> create view nameview as select * from mytable; 
```

##导出导入 SQL 文件：
```
sqlite > .output [filename ]  
sqlite > .dump  
sqlite > .output stdout 
```
从 SQL 文件导入数据库：
```
sqlite > .read [filename ] 
```
##备份数据库：

```
sqlite3 mytable.db .dump > backup.sql 
```
恢复数据库：
```
sqlite3 mytable.db < backup.sql 
```