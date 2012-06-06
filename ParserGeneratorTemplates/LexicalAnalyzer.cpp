#include <boost/regex.hpp>

#include "LexicalAnalyzer.h"

#include "ParseException.h"

LexicalAnalyzer::LexicalAnalyzer(std::string const& input)
    : input_(input)
    , curPos_(0) {
}

void LexicalAnalyzer::nextToken() {
@REGEX_MATCH_TRIES@
}

Token LexicalAnalyzer::curToken() const {
    return curToken_;
}

std::string LexicalAnalyzer::curTokenValue() const {
    return curTokenValue_;
}

int LexicalAnalyzer::curPos() const {
    return curPos_;
}
