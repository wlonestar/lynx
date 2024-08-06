# HTTP 协议实现

实现 HTTP 服务端部分请求和响应，使用 Ragel 状态机解析 HTTP/1.1 协议，接收用户发送的请求封装成 HTTP Request，通过注册一个 HTTP 请求处理的回调函数，交给用户来处理不同的请求所对应的响应，设置 HTTP Response，再将响应发送到客户端。
