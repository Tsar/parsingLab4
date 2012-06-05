#ifndef _TREE_H_
#define _TREE_H_

#include <string>
#include <vector>

class Tree {
public:
    Tree(std::string const& node)
        : node_(node)
        , height_(1) {
    }
    
    ~Tree() {
        for (size_t i = 0; i < children_.size(); ++i)
            delete children_[i];
    }
    
    void addChild(Tree* child) {
        children_.push_back(child);
        if (height_ < child->height_ + 1)
            height_ = child->height_ + 1;
    }
    
    std::string getNode() const {
        return node_;
    }
    
    std::vector<Tree*> const& getChildren() const {
        return children_;
    }
    
    int getHeight() const {
        return height_;
    }
private:
    std::string node_;
    std::vector<Tree*> children_;
    int height_;
};

#endif //_TREE_H_
