#pragma once
#include "BPlusNode.h"

template<typename K, typename V>

class LeafNode : public BPlusNode<K,V>{

    public:
        std::vector<V>m_values;
        LeafNode<K,V>*m_next;

        LeafNode() :BPlusNode<K,V>(true),m_next(nullptr){}
        ~LeafNode() =default;

        void display() const override
        {
            for(int i =0 ;i<(int)this->m_keys.size();i++)
            {
                std::cout <<"["<<this->m_keys[i]<<"]";
            }    
        }

        void insertAt(int idx,const K& key,const V& value)
        {
            this->m_keys.insert(this->m_keys.begin()+idx,key);
            this->m_values.insert(this->m_values.begin()+idx,value);
        }

        void removeAt(int idx)
        {
             this->m_keys.erase(this->m_keys.begin()+idx);
            this->m_values.erase(this->m_values.begin()+idx);
        }

};