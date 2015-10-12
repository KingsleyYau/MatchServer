# MatchServer
问题匹配中间件.</br>
1.接收http短连接请求</br>
2.查询sqlite内存数据库</br>
3.返回json数据</br>
4.同步mysql数据库数据到内存.</br>

# 配置文件
[BASE]              # 基本域</br>
PORT=9876           # 外部端口</br>
MAXCLIENT=100000    # 最大连接数</br>
MAXMEMORYCOPY=1     # 内存副本数目</br>
MAXHANDLETHREAD=2   # 接收处理线程</br>
LOGLEVEL=5          # 日志级别</br>
LOGDIR=./log        # 日志路径</br>
[DB]                # Database域</br>
DBHOST=127.0.0.1    # mysql地址</br>
DBPORT=3306         # mysql端口</br>
DBUSER=root         # mysql用户名</br>
DBPASS=123456       # mysql密码</br>
DBNAME=qpidnetwork  # mysql数据库名</br>
MAXDATABASETHREAD=4 # mysql连接池线程数目</br>
SYNCHRONIZETIME=30  # 同步时间间隔(分钟)</br>

