SPDY 3.1翻译
====================
[原文](http://www.chromium.org/spdy/spdy-protocol/spdy-protocol-draft3-1)

## Overview(概述)
One of the bottlenecks of HTTP implementations is that HTTP relies on multiple connections for concurrency. This causes several problems, including additional round trips for connection setup, slow-start delays, and connection rationing by the client, where it tries to avoid opening too many connections to any single server. HTTP pipelining helps some, but only achieves partial multiplexing. In addition, pipelining has proven non-deployable in existing browsers due to intermediary interference.

HTTP的一个瓶颈，是需要通过建立多个连接来实现并发。这个会带来很多问题，包括为连接的建立的额外开支，启动慢带来的延迟，当用户想避免打开针对某个服务器过多的访问可能需要涉及这些访问连接的配给问题。HTTP应用流水线设计技术可以解决部分问题，但是只能实现部分的多分路复用。另外，流水线设计技术已经证实，因为中介的干预，在现有浏览器的应用中是无法开展的。

SPDY adds a framing layer for multiplexing multiple, concurrent streams across a single TCP connection (or any reliable transport stream). The framing layer is optimized for HTTP-like request-response streams, such that applications which run over HTTP today can work over SPDY with little or no change on behalf of the web application writer.

SPDY为多路复用增加了一个框架层，通过一个单独的TCP实现同步的信息流（任何可信赖的输串流）。这个框架层能最大化的利用类似HTTP下请求-回复的输串流，以致web端应用程序的编写人员在使用时基本不需要做改变即可完成那些现在在HTTP下的应用。

The SPDY session offers four improvements over HTTP:
	•	Multiplexed requests: There is no limit to the number of requests that can be issued concurrently over a single SPDY connection.
	•	Prioritized requests: Clients can request certain resources to be delivered first. This avoids the problem of congesting the network channel with non-critical resources when a high-priority request is pending.
	•	Compressed headers: Clients today send a significant amount of redundant data in the form of HTTP headers. Because a single web page may require 50 or 100 subrequests, this data is significant.
	•	Server pushed streams: Server Push enables content to be pushed from servers to clients without a request.
	
SPDP相对于HTTP提供了四个改进：
	•	多路复用的请求：在一个单独的SPDY连接上，不限制可同时发布的请求数量。
	•	优先请求：用户可以要求一些资源被优先传送。这样可以避免因为不重要的资源拥堵网络渠道，导致优先级比较高的请求不能及时得到处理。
	•	被压缩的标题：现在的用户会以HTTP header的形式发送很多冗余的数据，因为一个单独的网页可以需要50或100个子标题，所以数据非常重要。
	•	服务器推动的信息流。因为服务器推动可以无需请求就使内容由服务器直接传动给用户。
	
SPDY attempts to preserve the existing semantics of HTTP. All features such as cookies, ETags, Vary headers, Content-Encoding negotiations, etc work as they do with HTTP; SPDY only replaces the way the data is written to the network.

SPDY试图保留现有HTTP的语法。所有的特点，比如cookies，ETags，Vary headers，Content-Encoding negotiations等等都和HTTP下一样，SPDY只是改变了数据被写到网络中的方式。

##SPDY Framing Layer
###Session(Connections)
SPDY framing layer是运行在tcp之上。 client是tcp连接的发起者，SPDY建立的连接是持久连接。</br>
从性能角度考虑，client不主动关闭连接，除非用户不再使用该连接。 server也尽可能长地保留连接，只在必要时中断空闲连接。任何一端如果要关闭连接，在关闭之前，首先必须发一个 GOAWAY frame,来检查请求是否已经完成。

###Framing
