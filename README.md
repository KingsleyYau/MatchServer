# MatchServer
问题匹配中间件.
1.接收http短连接请求,
2.查询sqlite内存数据库,
3.返回json数据,
4.同步mysql数据库数据到内存.

# 配置文件
[BASE]              # 基本域
PORT=9876           # 外部端口
MAXCLIENT=100000    # 最大连接数
MAXMEMORYCOPY=1     # 内存副本数目
MAXHANDLETHREAD=2   # 接收处理线程
LOGLEVEL=5          # 日志级别
LOGDIR=./log        # 日志路径
[DB]                # Database域
DBHOST=127.0.0.1    # mysql地址
DBPORT=3306         # mysql端口
DBUSER=root         # mysql用户名
DBPASS=123456       # mysql密码
DBNAME=qpidnetwork  # mysql数据库名
MAXDATABASETHREAD=4 # mysql连接池线程数目
SYNCHRONIZETIME=30  # 同步时间间隔(分钟)

