#include "httpcontent.h"

#include <algorithm>

#include "httpparsestate.h"
#include "httprequest.h"

using namespace tiny_muduo;

HttpContent::HttpContent() : parse_state_(kParseRequestLine) {}

HttpContent::~HttpContent() {}

bool HttpContent::ParseContent(Buffer* buffer) {
    bool linemore = true;
    bool parseok = true;
    const char* CRLF = nullptr;

    while (linemore) {
        CRLF = nullptr;
        if (parse_state_ == kParseRequestLine) {
            // 找到 \r\n 位置
            CRLF = buffer->FindCRLF();
            if (CRLF) {
                // 读取请求行
                parseok = request_.ParseRequestLine(buffer->Peek(), CRLF);
                if (parseok) {
                    parse_state_ = kParseHeaders;
                } else {
                    linemore = false;
                }
            } else {
                linemore = false;
            }
        } else if (parse_state_ == kParseHeaders) {
            CRLF = buffer->FindCRLF();
            if (CRLF) {
                const char* colon =
                    // find返回last指针代表没有寻找的内容
                    std::find((const char*)buffer->Peek(), CRLF, ':');
                if (colon == CRLF) {
                    parse_state_ = kParseGotCompleteRequest;
                    linemore = false;
                } else {
                    parseok =
                        request_.ParseHeaders(buffer->Peek(), colon, CRLF);
                    if (!parseok) linemore = false;
                }
            } else {
                linemore = false;
            }
        } else if (parse_state_ == kParseGotCompleteRequest) {
            linemore = false;
        } else if (parse_state_ == kParseBody) {
            // FIXME 当前仅支持GET方法 不解析请求body
        }

        if (CRLF) {
            // readerIndex 向后移动位置直到 crlf + 2
            buffer->RetrieveUntilIndex(CRLF + 2);
        }
    }

    return parseok;
}