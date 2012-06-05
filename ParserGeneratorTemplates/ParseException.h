#ifndef _PARSE_EXCEPTION_H_
#define _PARSE_EXCEPTION_H_

#include <string>

class ParseException {
public:
    ParseException(std::string const& message, int errorOffset)
        : message_(message)
        , errorOffset_(errorOffset) {
    }
    
    std::string getMessage() const {
        return message_;
    }
    
    int getErrorOffset() const {
        return errorOffset_;
    }
private:
    std::string message_;
    int errorOffset_;
};

#endif //_PARSE_EXCEPTION_H_
