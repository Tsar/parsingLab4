#include "LexicalAnalyzer.h"

#include "ParseException.h"

LexicalAnalyzer::LexicalAnalyzer(std::string const& input)
    : input_(input + "$")
    , curPos_(0) {
    nextChar();
}

void LexicalAnalyzer::nextToken() {
    while (isBlank(curChar_))
        nextChar();
    switch (curChar_) {
@CUR_CHAR_SWITCH@
        case '$':
            curToken_ = END;
            break;
        default:
            throw ParseException(std::string("Illegal character '") + curChar_ + std::string("' at position"), curPos_);
    }
}

Token LexicalAnalyzer::curToken() const {
    return curToken_;
}

int LexicalAnalyzer::curPos() const {
    return curPos_;
}

bool LexicalAnalyzer::isBlank(char c) const {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

void LexicalAnalyzer::nextChar() {
    curChar_ = input_[curPos_++];
}
