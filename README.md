# MatchServer
问题匹配中间件.</br>
1.接收http短连接请求.</br>
2.查询sqlite内存数据库.</br>
3.返回json数据.</br>
4.同步mysql数据库数据到内存.</br>

测试环境: Macbook Pro, CPU : I7, VM虚拟机分配4核, 2G内存, Linux centos 2.6.32-431.el6.x86_64</br>
测试数据: 初始化每张表120万条数据</br>
测试性能: 单线程插入单份内存一次插入240万条数据8秒, 单次查询性能为(105次/秒, CPU:230%/400%)</br>
