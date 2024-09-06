#ifndef TINY_MUDUO_HTTPREQUEST_H_
#define TINY_MUDUO_HTTPREQUEST_H_

#include <map>
#include <string>

#include "httpparsestate.h"

using std::string;

namespace tiny_muduo {

static const char http[] = "HTTP/1.";

// 请求方法
enum Method { kGet, kPost, kPut, kDelete, kTrace, kOptions, kConnect, kPatch };

// HTTP版本
enum Version { kUnknown, kHttp10, kHttp11 };

class HttpRequest {
   public:
    HttpRequest();
    ~HttpRequest();

    bool ParseRequestMethod(const char* start, const char* end);

    bool ParseRequestLine(const char* start, const char* end);

    bool ParseHeaders(const char* start, const char* colon, const char* end);

    bool ParseBody(const char* start, const char* end);

    Method method() const { return method_; }
    const string& path() const { return path_; }
    const string& query() const { return query_; }
    Version version() const { return version_; }
    const std::unordered_map<string, string>& headers() const { return headers_; }

    void Swap(HttpRequest& req);

    string GetHeader(const string& header) const {
        string ret = "";
        auto iter = headers_.find(header);
        return iter == headers_.end() ? ret : iter->second;
    }

   private:
    Method method_;
    Version version_;
    string path_;
    string query_;
    std::unordered_map<string, string> headers_;
};

}  // namespace tiny_muduo
#endif