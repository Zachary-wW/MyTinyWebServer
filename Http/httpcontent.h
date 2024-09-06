#ifndef TINY_MUDUO_HTTPCONTENT_H_
#define TINY_MUDUO_HTTPCONTENT_H_

#include <algorithm>
#include <utility>

#include "buffer.h"
#include "httpparsestate.h"
#include "httprequest.h"

namespace tiny_muduo {

class HttpContent {
   public:
    HttpContent();
    ~HttpContent();

    // void ParseLine(Buffer* buffer);
    bool ParseContent(Buffer* buffer);

    bool GetCompleteRequest() {
        return parse_state_ == kParseGotCompleteRequest;
    }

    const HttpRequest& request() const { return request_; }

    void ResetContentState() {
        // 构建一个空的临时对象 swap后 临时对象析构
        HttpRequest tmp;
        request_.Swap(tmp);
        parse_state_ = kParseRequestLine;
    }

   private:
    HttpRequest request_;
    HttpRequestParseState parse_state_;
};

}  // namespace tiny_muduo
#endif