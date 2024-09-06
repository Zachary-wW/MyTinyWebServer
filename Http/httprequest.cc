#include "httprequest.h"

#include <algorithm>
#include <string>
#include <utility>

#include "httpparsestate.h"

using namespace tiny_muduo;
using tiny_muduo::HttpRequestParseState;
using tiny_muduo::Method;

HttpRequest::HttpRequest() {}

HttpRequest::~HttpRequest() {}

// 分析请求方法
bool HttpRequest::ParseRequestMethod(const char* start, const char* end) {
    string request_method(start, end);
    bool has_method = true;
    if (request_method == "GET") {
        method_ = kGet;
    } else if (request_method == "POST") {
        method_ = kPost;
    } else if (request_method == "PUT") {
        method_ = kPut;
    } else if (request_method == "DELETE") {
        method_ = kDelete;
    } else if (request_method == "TRACE") {
        method_ = kTrace;
    } else if (request_method == "OPTIONS") {
        method_ = kOptions;
    } else if (request_method == "CONNECT") {
        method_ = kConnect;
    } else if (request_method == "PATCH") {
        method_ = kPatch;
    } else {
        has_method = false;
    }

    return has_method;
}

bool HttpRequest::ParseRequestLine(const char* start, const char* end) {
    const char* space = nullptr;
    // 找到第一个空格
    space = std::find(start, end, ' ');
    if (space == end) {
        return false;
    }
    // 不是预设的方法类型
    if (!ParseRequestMethod(start, space)) {
        return false;
    }
    start = space + 1;
    // 读取requestline中的路径
    space = std::find(start, end, ' ');
    if (space == end) {
        return false;
    }

    const char* query_ptr = std::find(start, end, '?');
    if (query_ptr != end) {
      // 给string赋予新的值，这里将C风格的字符串赋给string
        path_.assign(start, query_ptr);
        query_.assign(query_ptr + 1, space);
    } else {
        path_.assign(start, space);
    }
    start = space + 1;

    // HTTP/1.1正好8位
    // 判断是否为HTTP/1.1或者HTTP/1.0
    bool parsehttp = (start + 8 == end) && std::equal(start, end - 1, http);
    if (!parsehttp || (*(end - 1) != '0' && *(end - 1) != '1')) {
        version_ = kUnknown;
        return false;
    }

    if (*(end - 1) == '0') {
        version_ = kHttp10;
    } else {
        version_ = kHttp11;
    }

    return true;
}

// 分析requestline中的header
bool HttpRequest::ParseHeaders(const char* start, const char* colon,
                               const char* end) {
    const char* vaildstart = colon + 1;
    // 为了跳过头部字段值前面的所有空格
    while (*vaildstart == ' ') {
        ++vaildstart;
    }
    // 赋值map
    headers_[std::move(string(start, colon))] =
        std::move(string(vaildstart, end));
    return true;
}

bool HttpRequest::ParseBody(const char* start, const char* end) {
    (void)start;
    (void)end;
    return true;
}

void HttpRequest::Swap(HttpRequest& req) {
    // method_ = req.method_;
    // version_ = req.version_;
    std::swap(method_, req.method_);
    std::swap(version_, req.version_);
    path_.swap(req.path_);
    query_.swap(req.query_);
    headers_.swap(req.headers_);
}