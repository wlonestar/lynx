# 日志库

## 结构

- 前端：供应用程序使用的接口，并生成日志消息
- 后端：负责把日志消息写到目的地
- 两部分的接口可能简单到只有一个回调函数：`void output(const char *message, size_t len);`

在多线程程序中，前端和后端都与单线程程序无区别，每个线程有自己的前端，整个程序共用一个后端。难点在于将日志数据从多个前端高效地传输到后端，是一个多生产者-单消费者问题。对于前端而言，要尽量做到低延迟、低 CPU 开销、无阻塞；对于后端而言，要做到足够大的吞吐量，并占用较少资源。

## 日志记录的信息

1. 收到的每条内部消息的 id；
2. 收到的每条外部消息的全文；
3. 发出的每条消息的全文，每条消息都有全局唯一的 id；
4. 关键内部状态的变更。

## 功能

1. 控制日志消息的输出级别（TRACE, DEBUG, INFO, WARN, ERROR, FATAL）
2. 日志文件的滚动归档（按照文件大小和时间）

日志文件名：`log_file_test.20240719-141503.hostname.30621.log`

- 进程名：通常由 `main()` 函数参数中的 `argv[0]` 的 `basename`；
- 文件的创建时间：当地时区
- 机器名称
- 进程 id
- 后缀名 .log

日志消息格式：`<日期> <时间>.<微秒> <线程id> <级别> <函数名> <正文> - <源文件名>:<行号>`

```log
20240719 14:18:40.304236  2670  INFO logInThread logInThread - logging_unittest.cpp:46
20240719 14:18:40.305947  2671  INFO logInThread logInThread - logging_unittest.cpp:46
20240719 14:18:40.307661  2672  INFO logInThread logInThread - logging_unittest.cpp:46
20240719 14:18:40.309467  2673  INFO logInThread logInThread - logging_unittest.cpp:46
20240719 14:18:40.310161  2674  INFO logInThread logInThread - logging_unittest.cpp:46
20240719 14:18:40.311660  2669  INFO main Hello - logging_unittest.cpp:73
20240719 14:18:40.311767  2669  WARN main World - logging_unittest.cpp:74
20240719 14:18:40.311825  2669 ERROR main Error - logging_unittest.cpp:75
20240719 14:18:40.311841  2669  INFO main 4056 - logging_unittest.cpp:76
20240719 14:18:40.311896  2669  INFO main 4024 - logging_unittest.cpp:77
20240719 14:18:40.311963  2669  INFO main 4016 - logging_unittest.cpp:78
```

每条日志尽量一行，前四个字段的宽度是固定的，以空格分隔，便于用脚本解析。避免在日志格式中出现正则表达式的元字符，便于查找字符串。

## 多线程异步日志

双缓存技术：准备两块 buffer：A 和 B，前端负责往 buffer A 填数据，后端负责将 buffer B 的数据写入文件。当 buffer A 写满之后，交换 A 和 B，让后端将 buffer A 的数据写入文件，而前端则往 buffer B 填入新的日志消息，如此往复。
