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

