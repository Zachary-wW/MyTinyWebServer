#include "httpresponse.h"

#include <stdio.h>

#include <string>

#include "buffer.h"

using namespace tiny_muduo;
// using std::string;

const string HttpResponse::server_name_ = "Tiny_muduo";

void HttpResponse::AppendToBuffer(Buffer* buffer) {
    char buf[32] = {0};

    snprintf(buf, sizeof(buf), "HTTP/1.1 %d ", status_code_);
    buffer->Append(buf);
    buffer->Append(status_message_);
    buffer->Append(CRLF);

    if (close_connection_) {
        // 短连接
        // 服务器在发送完响应后会立即关闭连接。这意味着服务器不会期望客户端发送任何后续请求，
        // 因此服务器 一般 不需要告诉客户端预期会接收多少字节的数据。
        buffer->Append("Connection: close\r\n");
    } else {
        // 长连接
        snprintf(
            buf, sizeof(buf), "Content-Length: %zd\r\n",
            body_.size());  // no need to memset this is longer than HTTP... one
        buffer->Append(buf);
        buffer->Append("Connection: Keep-Alive\r\n");
    }

    buffer->Append("Content-Type: ");
    buffer->Append(type_);
    buffer->Append(CRLF);

    buffer->Append("Server: ");
    buffer->Append(server_name_);
    buffer->Append(CRLF);
    
    buffer->Append(CRLF);

    buffer->Append(body_);
    return;
}