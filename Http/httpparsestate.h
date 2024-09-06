#ifndef TINY_MUDUO_HTTPSTATE_H_
#define TINY_MUDUO_HTTPSTATE_H_

namespace tiny_muduo {

// HTTP请求状态
enum HttpRequestParseState {
    kParseRequestLine,
    kParseHeaders,
    kParseBody,
    kParseGotCompleteRequest, // 解析请求完成
    kParseErrno,
};

}

#endif