#include "httprequest.h"

#include <utility>
#include <algorithm>

#include "httpparsestate.h"

using namespace tiny_muduo;
using tiny_muduo::Method;
using tiny_muduo::HttpRequestParseState;

HttpRequest::HttpRequest() {
}

HttpRequest::~HttpRequest() {
}

bool HttpRequest::ParseRequestMethod(const char* start, const char* end) {
  string method(start, end);
  bool has_method = true;
  if (method == "GET") {
    method_ = kGet;
  } else if (method == "POST") {
    method_ = kPost;
  } else if (method == "PUT") {
    method_ = kPut;
  } else if (method == "DELETE") {
    method_ = kDelete;
  } else if (method == "TRACE") {
    method_ = kTrace;
  } else if (method == "OPTIONS") {
    method_ = kOptions;
  } else if (method == "CONNECT") {
    method_ = kConnect;
  } else if (method == "PATCH") {
    method_ = kPatch;
  } else {
    has_method = false;
  }

  return has_method;
}

void HttpRequest::ParseRequestLine(const char* start, const char* end, 
                      HttpRequestParseState& state) {
  {// 设置成块：逻辑清晰便于维护；防止局部变量命名冲突      
    const char* space = std::find(start, end, ' ');
    if (space == end) {
      state = kParseErrno;
      return;
    }
  
    if (!ParseRequestMethod(start, space)) {
      state = kParseErrno;
      return;
    }
    start = space + 1;
  }
 
  {
    const char* space = std::find(start, end, ' ');
    if (space == end) {
      state = kParseErrno;
      return;
    }

    const char* query = std::find(start, end, '?'); 
    if (query != end) {
      path_ = std::move(string(start, query));
      query_ = std::move(string(query + 1, space));
    } else {
      path_ = std::move(string(start, space));
    }        
    start = space + 1;
  }
   
  {
    const int httplen = sizeof(http) / sizeof(char) - 1;
    const char* httpindex = std::search(start, end, http, http + httplen);
    if (httpindex == end) {
      state = kParseErrno;
      return;
    }

    const char chr = *(httpindex + httplen); // 获取"http"之后紧跟的字符，用以判断HTTP版本。
    if (httpindex + httplen + 1 == end && (chr == '1' || chr == '0')) {
      if (chr == '1') {
        version_ = kHttp11;
      } else {
        version_ = kHttp10;
      }
    } else {
      state = kParseErrno;
      return;
    }
  }

  state = kParseHeaders;
}

void HttpRequest::ParseBody(const char* start, const char* end, 
                            HttpRequestParseState& state) {
}

void HttpRequest::ParseHeaders(const char* start, const char* end, 
                      HttpRequestParseState& state) {
  if (start == end && *start == '\r' && *(start + 1) == '\n') {
    state = kParseGotCompleteRequest;
    return;
  }

  const char* colon = std::find(start, end, ':');
  if (colon == end) {
    state = kParseErrno;
    return;
  }

  const char* vaild = colon + 1;
  while (*(vaild++) != ' ') {}
  headers_[std::move(string(start, colon))] = std::move(string(colon + 1, vaild));
  return;
}
