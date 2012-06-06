#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

#define DEBUG_LOG

struct NonTermRule {
    int left;
    std::vector<int> right;
    std::vector<bool> rightIsUserCode;
    std::vector<std::string> rightParams;
};

struct TermRule {
    int left;
    std::string right;
};

template <class T>
void setUnion(std::set<T>& targetSet, std::set<T> const& otherSet) {
    for (typename std::set<T>::const_iterator it = otherSet.begin(); it != otherSet.end(); ++it) {
        targetSet.insert(*it);
    }
}

class Grammar {
public:
    Grammar(std::string const& name)
        : dirName_(name + "_parser")
        , testsFileName_(name + "_tests.txt")
        , inHeadersBlock_(false)
        , inMembersBlock_(false)
        , headersBlock_("")
        , membersBlock_("") {
        std::ifstream f(name.c_str());
        int lineNum = 0;
        while (!f.eof()) {
            std::string s;
            getline(f, s);
            parseNewRule(s, ++lineNum);
        }
        f.close();
        fromNumber_[-1] = "eps";
    }
    
    ~Grammar() {
    }
    
    void generateParser() {
        if (nonTermRules_.empty()) {
            std::cerr << "No rules for non-terminals" << std::endl;
            return;
        }
        
        mkdir(dirName_.c_str(), 0777);
        substitutions_.clear();
        copyAndDoSubstitutions(testsFileName_, dirName_ + "/tests.txt");  //copying tests, no substitutions are made here really
        
        gen_TOKENS();
        //gen_CUR_CHAR_SWITCH();
        gen_REGEX_MATCH_TRIES();
        gen_NONTERMS_FUNC_DECLARATIONS();
        gen_START();
        gen_NONTERMS_FUNC_DEFINITIONS();
        
        std::ifstream templatesListFile("ParserGeneratorTemplates/TemplatesList.txt");
        while (!templatesListFile.eof()) {
            std::string s;
            getline(templatesListFile, s);
            if (s != "")
                copyAndDoSubstitutions("ParserGeneratorTemplates/" + s, dirName_ + "/" + s);
        }
        templatesListFile.close();
    }
private:
    void gen_TOKENS() {
        std::string res = "";
        
        std::set<int> tokensWritten;
        for (int i = 0; i < termRules_.size(); ++i) {
            if (tokensWritten.find(termRules_[i].left) == tokensWritten.end()) {
                tokensWritten.insert(termRules_[i].left);
                char buf[1024];
                sprintf(buf, "    TOKEN_%d,\n", termRules_[i].left);
                res += buf;
            }
        }
        
        substitutions_["@TOKENS@"] = res;
    }
    
    /*
    void gen_CUR_CHAR_SWITCH() {
        std::string res = "";
        
        for (int i = 0; i < termRules_.size(); ++i) {
            res += "        case '" + termRules_[i].right.substr(0, 1) + "':\n";
            for (int j = 0; j < termRules_[i].right.length() - 1; ++j) {
                res += "            nextChar();\n            if (curChar_ != '" + termRules_[i].right.substr(j + 1, 1) +
                       "')\n                throw ParseException(std::string(\"Illegal character '\") + curChar_ + std::string(\"' at position\"), curPos_);\n";
            }
            res += "            nextChar();\n";
            char buf[1024];
            sprintf(buf, "TOKEN_%d", termRules_[i].left);
            std::string sbuf = buf;
            res += "            curToken_ = " + sbuf + ";\n            break;\n";
        }
        
        substitutions_["@CUR_CHAR_SWITCH@"] = res;
    }
    */
    
    void gen_REGEX_MATCH_TRIES() {
        std::string res = "";
        
        for (int i = 0; i < termRules_.size(); ++i) {
            char buf[1024];
            sprintf(buf, "TOKEN_%d", termRules_[i].left);
            std::string sbuf = buf;
            res += "    boost::regex regex_" + sbuf + "(\"" + termRules_[i].right + "\");  //" + fromNumber_[termRules_[i].left] + "\n";
            res += "    if (boost::regex_search(input_, regexMatchResults, regex_" + sbuf + ", boost::match_default | boost::match_continuous)) {\n";
            res += "        curToken_ = " + sbuf + ";\n        curTokenValue_ = newTokenValue_;\n        newTokenValue_ = regexMatchResults[1];\n";
            res += "        input_ = input_.substr(regexMatchResults[0].length());\n        curPos_ += regexMatchResults[0].length();\n        return;\n    }\n";
        }
        res += "    throw ParseException(std::string(\"No matching regex at position\"), curPos_);\n";
        
        substitutions_["@REGEX_MATCH_TRIES@"] = res;
    }
    
    void gen_NONTERMS_FUNC_DECLARATIONS() {
        std::string res = "";
        
        std::set<int> nonTermsWritten;
        for (int i = 0; i < nonTermRules_.size(); ++i) {
            if (nonTermsWritten.find(nonTermRules_[i].left) == nonTermsWritten.end()) {
                nonTermsWritten.insert(nonTermRules_[i].left);
                char buf[1024];
                sprintf(buf, "    Tree* NONTERM_%d(", nonTermRules_[i].left);
                res += buf + nonTermParams_[fromNumber_[nonTermRules_[i].left]] + ");\n";
            }
        }
        
        substitutions_["@NONTERMS_FUNC_DECLARATIONS@"] = res;
    }
    
    void gen_START() {
        char buf[1024];
        sprintf(buf, "NONTERM_%d", nonTermRules_[0].left);
        substitutions_["@START@"] = buf;
    }
    
    void fill_FIRST() {
        for (int i = 0; i < termRules_.size(); ++i) {
            FIRST[termRules_[i].left].insert(termRules_[i].left);
        }
        FIRST[-1].insert(-1);  //eps
        bool changing = true;
        while (changing) {
            changing = false;
            for (int i = 0; i < nonTermRules_.size(); ++i) {
                int A = nonTermRules_[i].left;
                int sz = FIRST[A].size();
                
                int x = 0;
                while (nonTermRules_[i].rightIsUserCode[x])
                    ++x;
                setUnion(FIRST[A], FIRST[nonTermRules_[i].right[x]]);
                
                if (sz != FIRST[A].size())
                    changing = true;
            }
        }
#ifdef DEBUG_LOG
        std::set<int> nonTermsWrittenDEB;
        std::cout << "DEBUG: FIRST" << std::endl;
        for (int i = 0; i < nonTermRules_.size(); ++i) {
            if (nonTermsWrittenDEB.find(nonTermRules_[i].left) == nonTermsWrittenDEB.end()) {
                nonTermsWrittenDEB.insert(nonTermRules_[i].left);
                std::cout << " " << nonTermRules_[i].left << ":";
                for (std::set<int>::const_iterator it = FIRST[nonTermRules_[i].left].begin(); it != FIRST[nonTermRules_[i].left].end(); ++it)
                    std::cout << " " << *it;
                std::cout << std::endl;
            }
        }
#endif
    }
    
    void fill_FOLLOW() {
        FOLLOW[nonTermRules_[0].left].insert(-2);  //dollar '$' [END]
        bool changing = true;
        while (changing) {
            changing = false;
            for (int i = 0; i < nonTermRules_.size(); ++i) {
                int A = nonTermRules_[i].left;
                for (int j = 0; j < nonTermRules_[i].right.size(); ++j) {
                    if (nonTermRules_[i].rightIsUserCode[j])
                        continue;
                    int B = nonTermRules_[i].right[j];
                    int sz = FOLLOW[B].size();
                    
                    int x = j + 1;
                    while (x < nonTermRules_[i].right.size() && nonTermRules_[i].rightIsUserCode[x])
                        ++x;
                    int gamma;
                    if (x == nonTermRules_[i].right.size())
                        gamma = -1;
                    else
                        gamma = nonTermRules_[i].right[x];
                    
                    setUnion(FOLLOW[B], FIRST[gamma]);
                    FOLLOW[B].erase(-1);  //eps
                    if (FIRST[gamma].find(-1) != FIRST[gamma].end())
                        setUnion(FOLLOW[B], FOLLOW[A]);
                    
                    if (sz != FOLLOW[B].size())
                        changing = true;
                }
            }
        }
#ifdef DEBUG_LOG
        std::set<int> nonTermsWrittenDEB;
        std::cout << "DEBUG: FOLLOW" << std::endl;
        for (int i = 0; i < nonTermRules_.size(); ++i) {
            if (nonTermsWrittenDEB.find(nonTermRules_[i].left) == nonTermsWrittenDEB.end()) {
                nonTermsWrittenDEB.insert(nonTermRules_[i].left);
                std::cout << " " << nonTermRules_[i].left << ":";
                for (std::set<int>::const_iterator it = FOLLOW[nonTermRules_[i].left].begin(); it != FOLLOW[nonTermRules_[i].left].end(); ++it)
                    std::cout << " " << *it;
                std::cout << std::endl;
            }
        }
#endif
    }
    
    void gen_NONTERMS_FUNC_DEFINITIONS() {
        fill_FIRST();
        fill_FOLLOW();
        
        std::string res = "";
        
        std::set<int> nonTermsWritten;
        for (int i = 0; i < nonTermRules_.size(); ++i) {
            if (nonTermsWritten.find(nonTermRules_[i].left) == nonTermsWritten.end()) {
                nonTermsWritten.insert(nonTermRules_[i].left);
                char buf[1024];
                sprintf(buf, "Tree* Parser::NONTERM_%d(", nonTermRules_[i].left);
                res += buf + nonTermParams_[fromNumber_[nonTermRules_[i].left]] + ") {\n";
                res += "    Tree* res = new Tree(\"" + fromNumber_[nonTermRules_[i].left] + "\");\n";
                res += "    switch (lex_->curToken()) {\n";
                for (int j = 0; j < nonTermRules_.size(); ++j) {
                    if (nonTermRules_[i].left != nonTermRules_[j].left)
                        continue;
                    int x = 0;
                    while (nonTermRules_[j].rightIsUserCode[x])
                        ++x;
                    
                    int alpha = nonTermRules_[j].right[x];
                    std::set<int> iterateOver;
                    if (FIRST[alpha].find(-1) == FIRST[alpha].end()) {
                        iterateOver = FIRST[alpha];
                    } else {
                        iterateOver = FIRST[alpha];
                        iterateOver.erase(-1);
                        setUnion(iterateOver, FOLLOW[nonTermRules_[j].left]);
                    }
                    
                    for (std::set<int>::const_iterator it = iterateOver.begin(); it != iterateOver.end(); ++it) {
                        sprintf(buf, "        case TOKEN_%d:\n", *it);
                        res += (*it == -2) ? "        case END:\n" : buf;
                        for (int k = 0; k < nonTermRules_[j].right.size(); ++k) {
                            if (nonTermRules_[j].rightIsUserCode[k]) {
                                res += "            " + userCode_[nonTermRules_[j].right[k]] + "  //user code\n";
                            } else {
                                int X = nonTermRules_[j].right[k];
                                res += "            //" + fromNumber_[X] + "\n";
                                if (terms_.find(X) != terms_.end()) {
                                    char buf2[1024];
                                    sprintf(buf2, "TOKEN_%d", X);
                                    std::string sbuf2 = (X == -2) ? "END" : buf2;
                                    res += "            if (lex_->curToken() != " + sbuf2 + ")\n                throw ParseException(\"Terminal '" + fromNumber_[X] + "' expected at position\", lex_->curPos() - 1);\n";
                                    res += "            res->addChild(new Tree(\"" + fromNumber_[X] + "\"));\n            lex_->nextToken();\n";
                                } else {
                                    char buf2[4096];
                                    sprintf(buf2, "NONTERM_%d(%s)", X, nonTermRules_[j].rightParams[k].c_str());
                                    std::string sbuf2 = (X == -1) ? "new Tree(\"eps\")" : buf2;
                                    res += "            res->addChild(" + sbuf2 + ");\n";
                                }
                            }
                        }
                        res += "            break;\n";
                    }
                }
                res += "        default:\n            throw ParseException(\"Unexpected token at position\", lex_->curPos() - 1);\n    }\n    return res;\n}\n\n";
            }
        }
        
        substitutions_["@NONTERMS_FUNC_DEFINITIONS@"] = res;
    }
    
    void parseNewRule(std::string const& s, int lineNum) {
        if (inHeadersBlock_) {
            if (s.substr(0, 10) == "[/headers]")
                inHeadersBlock_ = false;
            else
                headersBlock_ += s + "\n";
            return;
        }
        if (inMembersBlock_) {
            if (s.substr(0, 10) == "[/members]")
                inMembersBlock_ = false;
            else
                membersBlock_ += s + "\n";
            return;
        }
        if (s == "")
            return;
        if (s.substr(0, 9) == "[headers]") {
            inHeadersBlock_ = true;
            return;
        }
        if (s.substr(0, 9) == "[members]") {
            inMembersBlock_ = true;
            return;
        }
        size_t p1 = s.find("->");
        if (p1 == std::string::npos) {
            std::cerr << "Rule on line " << lineNum << " incorrect (doesn't have '->')" << std::endl;
            return;
        }
        size_t p2 = s.find("\"");
        size_t p4 = s.find("{");
        if (p2 != std::string::npos && p4 == std::string::npos) {
            size_t p3 = s.find("\"", p2 + 1);
            if (p3 != std::string::npos) {
                TermRule newTermRule;
                newTermRule.left = toNumber(s.substr(0, p1));
                newTermRule.right = s.substr(p2 + 1, p3 - p2 - 1);
                terms_.insert(newTermRule.left);
                termRules_.push_back(newTermRule);
#ifdef DEBUG_LOG
                std::cout << "DEBUG: added term rule     [" << newTermRule.left << ": '" << newTermRule.right << "']" << std::endl;
#endif
            } else {
                std::cerr << "Rule on line " << lineNum << " incorrect (has only one [\"])" << std::endl;
                return;
            }
        } else {
            NonTermRule newNonTermRule;
            newNonTermRule.left = toNumber(s.substr(0, p1), true, true);
            std::string r = s.substr(p1 + 2);
            size_t p5, p6;
            for (int i = 0; i < r.length(); ++i) {
                switch (r[i]) {
                    case ' ':
                        break;
                    case '{':
                        p5 = r.find("}", i + 1);
                        if (p5 != std::string::npos) {
                            userCode_.push_back(r.substr(i + 1, p5 - i - 1));
                            newNonTermRule.right.push_back(userCode_.size() - 1);
                            newNonTermRule.rightIsUserCode.push_back(true);
                            newNonTermRule.rightParams.push_back("");
                            i = p5;
                        } else {
                            std::cerr << "Rule on line " << lineNum << " incorrect (has only '{', no '}')" << std::endl;
                            return;
                        }
                        break;
                    default:
                        p6 = r.find(" ", i + 1);
                        std::string prms = "";
                        if (p6 != std::string::npos) {
                            newNonTermRule.right.push_back(toNumber(r.substr(i, p6 - i), true, false, &prms));
                            i = p6;
                        } else {
                            newNonTermRule.right.push_back(toNumber(r.substr(i), true, false, &prms));
                            i = r.length();
                        }
                        newNonTermRule.rightIsUserCode.push_back(false);
                        newNonTermRule.rightParams.push_back(prms);
                        break;
                }
            }
            nonTermRules_.push_back(newNonTermRule);
#ifdef DEBUG_LOG
            std::cout << "DEBUG: added non-term rule [" << newNonTermRule.left << ":";
            for (int i = 0; i < newNonTermRule.right.size(); ++i) {
                if (newNonTermRule.rightIsUserCode[i]) {
                    std::cout << " {" << userCode_[newNonTermRule.right[i]] << "}";
                } else {
                    std::cout << " " << newNonTermRule.right[i];
                }
            }
            std::cout << "]" << std::endl;
#endif
        }
    }
    
    void copyAndDoSubstitutions(std::string const& source, std::string const& dest) {
        std::string fileContents;
        std::ifstream f(source.c_str());
        while (!f.eof()) {
            std::string s;
            getline(f, s);
            fileContents += s + "\n";
        }
        f.close();
        
        substitutions_["@HEADERS_BLOCK@"] = headersBlock_;
        substitutions_["@MEMBERS_BLOCK@"] = membersBlock_;
        
        size_t p;
        for (std::map<std::string, std::string>::const_iterator it = substitutions_.begin(); it != substitutions_.end(); ++it) {
            p = fileContents.find(it->first);
            while (p != std::string::npos) {
                fileContents.replace(p, it->first.length(), it->second);
                p = fileContents.find(it->first, p + 1);
            }
        }
        std::string def_LAST_TOKEN = "$LAST_TOKEN$";
        p = fileContents.find(def_LAST_TOKEN);
        while (p != std::string::npos) {
            fileContents.replace(p, def_LAST_TOKEN.length(), "lex_->curTokenValue()");
            p = fileContents.find(def_LAST_TOKEN, p + 1);
        }
        
        std::ofstream g(dest.c_str());
        g << fileContents;
        g.close();
    }
    
    int toNumber(std::string const& a, bool lookupParams = false, bool leftNonTerm = false, std::string* paramsRes = 0) {
        std::string aCopy = a;
        while (aCopy[0] == ' ')
            aCopy = aCopy.substr(1);
        while (aCopy[aCopy.length() - 1] == ' ')
            aCopy = aCopy.substr(0, aCopy.length() - 1);
        if (lookupParams && leftNonTerm) {
            int p1 = aCopy.find("(");
            int p2 = aCopy.find(")");
            if (p1 != std::string::npos && p2 != std::string::npos) {
                std::string params = aCopy.substr(p1 + 1, p2 - p1 - 1);
                aCopy = aCopy.substr(0, p1);
                while (aCopy[aCopy.length() - 1] == ' ')
                    aCopy = aCopy.substr(0, aCopy.length() - 1);
                nonTermParams_[aCopy] = params;
            }
        } else if (lookupParams) {
            int p1 = aCopy.find("(");
            int p2 = aCopy.find(")");
            if (p1 != std::string::npos && p2 != std::string::npos) {
                if (paramsRes != 0)
                    *paramsRes = aCopy.substr(p1 + 1, p2 - p1 - 1);
                aCopy = aCopy.substr(0, p1);
                while (aCopy[aCopy.length() - 1] == ' ')
                    aCopy = aCopy.substr(0, aCopy.length() - 1);
            }
        }
        if (aCopy == "eps")
            return -1;
        if (toNumber_.find(aCopy) != toNumber_.end()) {
            return toNumber_[aCopy];
        } else {
            int newNumber = toNumber_.size();
            toNumber_[aCopy] = newNumber;
            fromNumber_[newNumber] = aCopy;
#ifdef DEBUG_LOG
            std::cout << "DEBUG: num[" << aCopy << "] = " << newNumber << std::endl;
#endif
            return newNumber;
        }
    }

    std::string dirName_, testsFileName_;
    std::map<std::string, int> toNumber_;
    std::map<int, std::string> fromNumber_;
    std::vector<NonTermRule> nonTermRules_;
    std::map<std::string, std::string> nonTermParams_;
    std::vector<TermRule> termRules_;
    std::set<int> terms_;
    std::vector<std::string> userCode_;
    std::map<std::string, std::string> substitutions_;
    std::map<int, std::set<int> > FIRST;
    std::map<int, std::set<int> > FOLLOW;
    bool inHeadersBlock_, inMembersBlock_;
    std::string headersBlock_, membersBlock_;
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ParserBuilder <grammar_file_name>" << std::endl;
        return 1;
    }
    Grammar g(argv[1]);
    g.generateParser();
    return 0;
}
