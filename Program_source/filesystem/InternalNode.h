#pragma once
#include "BPlusNode.h"

template<typename K, typename V>
class InternalNode: public BPlusNode<K,V> 
{
    public:
        std::vector <BPlusNode<K,V>>m_children;

        InternalNode() :BPlusNode<K,V>(false){}
        ~InternalNode()
        {
            for(auto it :m_children)
            {
                delete it;
            }
        }

        void display() const override 
        {
            for(const auto &key :this->m_keys)
            {
                std::cout<<"["<<key<<"]";
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