#ifndef _LEXICAL_ANALYZER_H_
#define _LEXICAL_ANALYZER_H_

#include <string>

enum Token {
@TOKENS@
    END
};

class LexicalAnalyzer {
public:
    LexicalAnalyzer(std::string const& input);
    void nextToken();
    Token curToken() const;
    std::string curTokenValue() const;
    int curPos() const;
private:
    std::string input_;
    int curPos_;
    Token curToken_;
    std::string curTokenValue_;
};

#endif //_LEXICAL_ANALYZER_H_
