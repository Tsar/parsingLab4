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
    std::vector<int>  right;
    std::vector<bool> rightIsUserCode;
};

struct TermRule {
    int left;
    std::string right;
};

class Grammar {
public:
    Grammar(std::string const& name)
        : dirName_(name + "_parser") {
        std::ifstream f(name.c_str());
        int lineNum = 0;
        while (!f.eof()) {
            std::string s;
            getline(f, s);
            parseNewRule(s, ++lineNum);
        }
        f.close();
    }
    
    ~Grammar() {
    }
    
    void generateParser() {
        if (nonTermRules_.empty()) {
            std::cerr << "No rules for non-terminals" << std::endl;
            return;
        }
        
        gen_TOKENS();
        gen_CUR_CHAR_SWITCH();
        gen_NONTERMS_FUNC_DECLARATIONS();
        gen_START();
        gen_NONTERMS_FUNC_DEFINITIONS();
        
        mkdir(dirName_.c_str(), 0777);
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
    
    void gen_NONTERMS_FUNC_DECLARATIONS() {
        std::string res = "";
        
        std::set<int> nonTermsWritten;
        for (int i = 0; i < nonTermRules_.size(); ++i) {
            if (nonTermsWritten.find(nonTermRules_[i].left) == nonTermsWritten.end()) {
                nonTermsWritten.insert(nonTermRules_[i].left);
                char buf[1024];
                sprintf(buf, "    Tree* NONTERM_%d();\n", nonTermRules_[i].left);
                res += buf;
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
            FIRST[termRules_[i].left].insert(termRules_[i].right[0]);
        }
        bool changing = true;
        while (changing) {
            changing = false;
            for (int i = 0; i < nonTermRules_.size(); ++i) {
                int sz = FIRST[nonTermRules_[i].left].size();
                int x = 0;
                while (nonTermRules_[i].rightIsUserCode[x])
                    ++x;
                for (std::set<char>::const_iterator it = FIRST[nonTermRules_[i].right[x]].begin(); it != FIRST[nonTermRules_[i].right[x]].end(); ++it)
                    FIRST[nonTermRules_[i].left].insert(*it);
                if (sz != FIRST[nonTermRules_[i].left].size())
                    changing = true;
            }
        }
#ifdef DEBUG_LOG
        std::set<int> nonTermsWrittenDEB;
        std::cout << "DEBUG: FIRST" << std::endl;
        for (int i = 0; i < nonTermRules_.size(); ++i) {
            if (nonTermsWrittenDEB.find(nonTermRules_[i].left) == nonTermsWrittenDEB.end()) {
                nonTermsWrittenDEB.insert(nonTermRules_[i].left);
                std::cout << " " << nonTermRules_[i].left << ": ";
                for (std::set<char>::const_iterator it = FIRST[nonTermRules_[i].left].begin(); it != FIRST[nonTermRules_[i].left].end(); ++it)
                    std::cout << *it;
                std::cout << std::endl;
            }
        }
#endif
    }
    
    void fill_FOLLOW() {
        bool changing = true;
        while (changing) {
            changing = false;
            for (int i = 0; i < nonTermRules_.size(); ++i) {
                int sz = FOLLOW[nonTermRules_[i].left].size();
                
                if (sz != FOLLOW[nonTermRules_[i].left].size())
                    changing = true;
            }
        }
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
                sprintf(buf, "Tree* Parser::NONTERM_%d() {\n", nonTermRules_[i].left);
                res += buf;
                
                res += "}\n\n";
            }
        }
        
        substitutions_["@NONTERMS_FUNC_DEFINITIONS@"] = res;
    }
    
    void parseNewRule(std::string const& s, int lineNum) {
        if (s == "")
            return;
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
            newNonTermRule.left = toNumber(s.substr(0, p1));
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
                            i = p5;
                        } else {
                            std::cerr << "Rule on line " << lineNum << " incorrect (has only '{', no '}')" << std::endl;
                            return;
                        }
                        break;
                    default:
                        p6 = r.find(" ", i + 1);
                        if (p6 != std::string::npos) {
                            newNonTermRule.right.push_back(toNumber(r.substr(i, p6 - i)));
                            newNonTermRule.rightIsUserCode.push_back(false);
                            i = p6;
                        } else {
                            newNonTermRule.right.push_back(toNumber(r.substr(i)));
                            i = r.length();
                        }
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
        
        for (std::map<std::string, std::string>::const_iterator it = substitutions_.begin(); it != substitutions_.end(); ++it) {
            size_t p = fileContents.find(it->first);
            while (p != std::string::npos) {
                fileContents.replace(p, it->first.length(), it->second);
                p = fileContents.find(it->first, p + 1);
            }
        }
        
        std::ofstream g(dest.c_str());
        g << fileContents;
        g.close();
    }
    
    int toNumber(std::string const& a) {
        std::string aCopy = a;
        while (aCopy[0] == ' ')
            aCopy = aCopy.substr(1);
        while (aCopy[aCopy.length() - 1] == ' ')
            aCopy = aCopy.substr(0, aCopy.length() - 1);
        if (aCopy == "eps")
            return -1;
        if (toNumber_.find(aCopy) != toNumber_.end()) {
            return toNumber_[aCopy];
        } else {
            int newNumber = toNumber_.size();
            toNumber_[aCopy] = newNumber;
            return newNumber;
        }
    }

    std::string dirName_;
    std::map<std::string, int> toNumber_;
    std::vector<NonTermRule> nonTermRules_;
    std::vector<TermRule> termRules_;
    std::vector<std::string> userCode_;
    std::map<std::string, std::string> substitutions_;
    std::map<int, std::set<char> > FIRST;
    std::map<int, std::set<char> > FOLLOW;
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
