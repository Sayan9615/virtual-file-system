#pragma once
#include "BPlusNode.h"

template<typename K, typename V>
class InternalNode: public BPlusNode<K,V> 
{
    public:
        std::vector <BPlusNode<K,V>*>m_children;

        InternalNode() :BPlusNode<K,V>(false){}
        ~InternalNode() = default;

        void display() const override 
        {
            for(const auto &key :this->m_keys)
            {
                std::cout<<"["<<key<<"]";
            }    

        }

        
       void insertAt(int idx, const K& key, BPlusNode<K,V>* child)
        {
            this->m_keys.insert(this->m_keys.begin() + idx, key);
            this->m_children.insert(this->m_children.begin() + idx + 1, child);
        }

        void removeAt(int idx)
        {
             this->m_keys.erase(this->m_keys.begin()+idx);
            this->m_children.erase(this->m_children.begin()+idx+1);
        }




};