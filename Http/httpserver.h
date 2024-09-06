#ifndef TINY_MUDUO_HTTPSERVER_H_
#define TINY_MUDUO_HTTPSERVER_H_

#include <stdio.h>

#include <functional>
#include <memory>
#include <utility>

#include "buffer.h"
#include "callback.h"
#include "eventloop.h"
#include "httpcontent.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "logging.h"
#include "tcpconnection.h"
#include "tcpserver.h"
#include "timestamp.h"

using tiny_muduo::HttpStatusCode;

namespace tiny_muduo {

static const double kIdleConnectionTimeOuts = 8.0;

class HttpServer {
    typedef std::function<void(const HttpRequest&, HttpResponse&)>
        HttpResponseCallback;

   public:
    HttpServer(EventLoop* loop, const Address& address,
               bool auto_close_idleconnection = false);
    ~HttpServer();

    void Start() { server_.Start(); }

    void HttpDefaultCallback(const HttpRequest& request,
                             HttpResponse& response) {
        response.SetStatusCode(k404NotFound);
        response.SetStatusMessage("Not Found");
        response.SetCloseConnection(true);
        (void)request;
    }
    // 当不拥有对象的所有权但是又想安全访问对象时使用weak ptr
    // 缺点：在定时任务的第一次检查周期内，如果连接在开始时发送了消息，那么即使在随后的时间内没有任何活动，
    // 连接也不会在第一次检查时被关闭。这可能导致连接在实际空闲后，需要等待两倍的空闲时间间隔才会被关闭。
    void HandleIdleConnection(std::weak_ptr<TcpConnection>& connection) {
        TcpConnectionPtr conn(connection.lock());
        if (conn) {
            if (Timestamp::AddTime(conn->timestamp(), kIdleConnectionTimeOuts) <
                Timestamp::Now()) {
                // 超时
                conn->Shutdown();
            } else {
                // 如果没有超过 加入一个定时器
                loop_->RunAfter(
                    kIdleConnectionTimeOuts,
                    std::move(std::bind(&HttpServer::HandleIdleConnection, this,
                                        connection)));
            }
        }
    }

    void ConnectionCallback(const TcpConnectionPtr& connection) {
        if (auto_close_idleconnection_) {
            loop_->RunAfter(
                kIdleConnectionTimeOuts,
                std::bind(&HttpServer::HandleIdleConnection, this,
                          std::weak_ptr<TcpConnection>(connection)));
        }
    }

    void MessageCallback(const TcpConnectionPtr& connection, Buffer* buffer);

    void SetHttpResponseCallback(
        const HttpResponseCallback& response_callback) {
        response_callback_ = response_callback;
    }

    void SetHttpResponseCallback(HttpResponseCallback&& response_callback) {
        response_callback_ = std::move(response_callback);
    }

    void SetThreadNums(int thread_nums) { server_.SetThreadNums(thread_nums); }

    void DealWithRequest(const HttpRequest& request,
                         const TcpConnectionPtr& connection);

   private:
    EventLoop* loop_;
    TcpServer server_;
    bool auto_close_idleconnection_;

    HttpResponseCallback response_callback_;
};

}  // namespace tiny_muduo

#endif