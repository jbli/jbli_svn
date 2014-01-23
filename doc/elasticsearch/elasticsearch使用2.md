search apis
===
绝大多数 search APi是multi-index, multi-type, 其中Explain API例外<br\>
执行一个search,默认是广播到所有 index shards, 可以通过routing参数来限制到只有某一个(或多个)shards上查询

###uri search

```
curl -XGET 'http://localhost:9200/twitter/_search?q=user:kimchy'
curl -XGET 'http://localhost:9200/kimchy,elasticsearch/tweet/_search?q=tag:wow'
curl - XGET 'http://localhost:9200/_all/tweet/_search?q=tag:wow'
curl -XGET 'http://localhost:9200/_search?q=tag:wow'
```

其中 q maps to query_string
###request body search

```
curl -XGET 'http://localhost:9200/twitter/tweet/_search' -d '{
    "query" : {
        "term" : { "user" : "kimchy" }
    }
}
'
```
支持参数：

- timeout
- from
- size 返回结果的数量，默认为10
- search_type  dfs_query_then_fetch, dfs_query_and_fetch, query_then_fetch, query_and_fetch. Defaults to query_then_fetch

注意 search_type不能在request body被传递，只能通过做为REST parameter来传递

###from/size
要查询的结果中，从哪一条开始提取，提取多少条，使用from/size

```
{
    "from" : 0, "size" : 10,
    "query" : {
        "term" : { "user" : "kimchy" }
    }
}
``` 

###source filtering
```
{
    "_source": [ "obj1.*", "obj2.*" ],
    "query" : {
        "term" : { "user" : "kimchy" }
    }
}
```

```
{
    "_source": {
        "include": [ "obj1.*", "obj2.*" ],
        "exclude": [ "*.description" ],
    }
    "query" : {
        "term" : { "user" : "kimchy" }
    }
}
```

###script fields
```
{
    "query" : {
        ...
    },
    "script_fields" : {
        "test1" : {
            "script" : "doc['my_field_name'].value * 2"
        },
        "test2" : {
            "script" : "doc['my_field_name'].value * factor",
            "params" : {
                "factor"  : 2.0
            }
        }
    }
}
```
或者

```
    {
        "query" : {
            ...
        },
        "script_fields" : {
            "test1" : {
                "script" : "_source.obj1.obj2"
            }
        }
    }
```

doc['my_field'].value和_source.my_field的区别：

- doc[]会把该field加载到内存中，执行更快，但也会消耗内存
- _source 加载整个source,分析和返回相关部分

###field data fields
```
{
    "query" : {
        ...
    },
    "fielddata_fields" : ["test1", "test2"]
}
```
fielddata_fields会加载相应field的term到内存中(cached),使用更多内存。

###post filter
在查询结果的基础上进行过滤,例子如下：
创建两个文档

```
curl -XPUT 'localhost:9200/twitter/tweet/1' -d '
{
    "message" : "something blue",
    "tag" : "blue"
}
'

curl -XPUT 'localhost:9200/twitter/tweet/2' -d '
{
    "message" : "something green",
    "tag" : "green"
}
'

curl -XPOST 'localhost:9200/_refresh'
```
查询

```
curl -XPOST 'localhost:9200/twitter/_search?pretty=true' -d '
{
    "query" : {
        "term" : { "message" : "something" }
    },
    "facets" : {
        "tag" : {
            "terms" : { "field" : "tag" }
        }
    }
}
'
```

使用

```
curl -XPOST 'localhost:9200/twitter/_search?pretty=true' -d '
{
    "query" : {
        "term" : { "message" : "something" }
    },
    "post_filter" : {
        "term" : { "tag" : "green" }
    },
    "facets" : {
        "tag" : {
            "terms" : { "field" : "tag" }
        }
    }
}
'
```

###highlighting
###rescoring
###search type
###scroll
对于大的查询结果，可以使用scroll来一部分一部分返回。

```
curl -XGET 'http://localhost:9200/twitter/tweet/_search?scroll=5m' -d '{
    "query": {
        "query_string" : {
            "query" : "some query string here"
        }
    }
}
'
```
每次查询，都会返回一个scroll_id,下次查询时传入该值

```
curl -XGET 'http://localhost:9200/_search/scroll?scroll=5m&scroll_id=c2Nhbjs2OzM0NDg1ODpzRlBLc0FXNlNyNm5JWUc1'
```

###preference
- _primary
- _primary_first
- _local
- _only_node:xyz
- _prefer_node:xyz
- _shards:2,3
- Custom(string) value

###facets






