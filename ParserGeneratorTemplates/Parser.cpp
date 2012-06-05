#include "Parser.h"

Parser::Parser()
    : lex_(0) {
}

Parser::~Parser() {
    if (lex_)
        delete lex_;
}

Tree* Parser::parse(std::string const& input) {
    if (lex_)
        delete lex_;
    lex_ = new LexicalAnalyzer(input);
    lex_->nextToken();
    Tree* res = @START@();
    if (lex_->curToken() != END)
        throw ParseException("Unexpected token at position", lex_->curPos() - 1);
    return res;
}

@NONTERMS_FUNC_DEFINITIONS@
