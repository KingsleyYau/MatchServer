# MatchServer
问题匹配中间件.</br>
1.接收http短连接请求</br>
2.查询sqlite内存数据库</br>
3.返回json数据</br>
4.同步mysql数据库数据到内存.</br>

# 配置文件
基本域</br>
[BASE]</br>
外部端口</br>
PORT=9876
最大连接数</br>
MAXCLIENT=100000</br>
内存副本数目</br>
MAXMEMORYCOPY=1</br>     
接收处理线程</br>
MAXHANDLETHREAD=2</br>
日志级别</br>
LOGLEVEL=5</br>
日志路径</br>
LOGDIR=./log</br>
Database域</br>
[DB]</br>
mysql地址</br>
DBHOST=127.0.0.1</br>
mysql端口</br>
DBPORT=3306口</br>
mysql用户名</br>
DBUSER=root</br>
mysql密码</br>
DBPASS=123456</br>
mysql数据库名</br>
DBNAME=qpidnetwork</br>
mysql连接池线程数目</br>
MAXDATABASETHREAD=4</br>
同步时间间隔(分钟)</br>
SYNCHRONIZETIME=30</br>

