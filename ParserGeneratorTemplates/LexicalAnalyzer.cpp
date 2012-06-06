#include <boost/regex.hpp>

#include "LexicalAnalyzer.h"

#include "ParseException.h"

LexicalAnalyzer::LexicalAnalyzer(std::string const& input)
    : input_(input)
    , curPos_(0)
    , curTokenValue_("")
    , newTokenValue_("") {
}

void LexicalAnalyzer::nextToken() {
    if (input_.length() == 0) {
        curToken_ = END;
        curTokenValue_ = newTokenValue_;
        newTokenValue_ = "";
        return;
    }
    boost::smatch regexMatchResults;
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
