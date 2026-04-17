#pragma once
#include <vector>
#include <iostream>


template<typename K, typename V>

class BPlusNode {
    public:
        std::vector<K>m_keys;
        bool m_isLeaf;


        BPlusNode (bool isLeaf) : m_isLeaf(isLeaf) {}
        virtual ~BPlusNode() = default;

        bool isLeaf() const {return m_isLeaf;}
        virtual void display() const =0;
};       