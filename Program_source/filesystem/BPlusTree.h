#pragma once
#include "LeafNode.h"
#include "InternalNode.h"
#include <optional>


template<typename K, typename V>
class BPlusTree
{

    private:
        BPlusNode<K,V>* m_root;
        int m_order; //ordin arbore

 
        LeafNode<K,V>* findLeaf(const K& key)const;
        void splitLeaf(LeafNode<K,V>* leaf,InternalNode<K,V>*parent ,int idx);
        void splitInternal(InternalNode<K,V>*leaf ,InternalNode<K,V>*parent,int idx);
        void insertInParent(BPlusNode<K,V>*left,const K& key,BPlusNode<K,V>*right);
        int findChildIndex(InternalNode<K, V>* parent, BPlusNode<K, V>* child);
        InternalNode<K, V>* findParent(BPlusNode<K, V>* target,std::vector<InternalNode<K, V>*>& path);
        void removeFromLeaf(LeafNode<K, V>* leaf, const K& key,std::vector<InternalNode<K, V>*>& path);
        void fixInternalUnderflow(InternalNode<K, V>* node,std::vector<InternalNode<K, V>*>& path);
        


    public:
        BPlusTree(int order=3);
        ~BPlusTree();

        void insert(const K& key,const V& value);
        std::optional<V> search(const K& key) const;
        void remove(const K& key);
        std::vector<V> rangeSearch(const K& k1,const K& k2)const;
        void display() const;

};


template <typename K, typename V>
LeafNode<K, V>* BPlusTree<K,V>::findLeaf(const K& key) const
{
    BPlusNode<K,V>*current = m_root;

    while(current!= nullptr && !current->isLeaf())
    {
        InternalNode<K,V>* node  =static_cast <InternalNode<K,V>*>(current);
        
        int i =0;

        //caut unde cobor
        while(i<(int)node->m_keys.size() && key >= node->m_keys[i])
        {
            i++;
        }
        
        if (i >= (int)node->m_children.size())
             i = node->m_children.size() - 1;
        current =node->m_children[i];
    }

    return static_cast<LeafNode<K,V>*>(current);

}

template <typename K, typename V>
std::optional<V> BPlusTree<K, V>::search(const K& key) const 
{
    if(!m_root) return std::nullopt;

    LeafNode<K,V>* leaf=findLeaf(key);
    for(int i =0 ;i<(int) leaf->m_keys.size();i++)
     {
        if(leaf->m_keys[i]==key)
        {
            return leaf->m_values[i]; //gasit
        }

     }
     return std::nullopt;
}

template <typename K, typename V>
inline void BPlusTree<K, V>::remove(const K &key)
{
    if (!m_root) return;

    // caut path-ul de la root la leaf
     std::vector<InternalNode<K, V>*> path;
    BPlusNode<K, V>* current = m_root;

    while (!current->isLeaf()) 
    {
        InternalNode<K, V>* node = static_cast<InternalNode<K, V>*>(current);
        path.push_back(node);
        int i = 0;
        while (i < (int)node->m_keys.size() && key >= node->m_keys[i]) i++;
        current = node->m_children[i];
    }

    LeafNode<K, V>* leaf = static_cast<LeafNode<K, V>*>(current);
    removeFromLeaf(leaf, key, path);
}

template <typename K, typename V>
std::vector<V> BPlusTree<K, V>::rangeSearch(const K& k1, const K& k2) const
{
    std::vector<V> results;
    if(!m_root) return results;

    LeafNode<K,V>* leaf=findLeaf(k1);
    while(leaf)
    {
        for(int i=0;i<(int)leaf->m_keys.size();i++)
        {
            if(leaf->m_keys[i]>=k1 && leaf->m_keys[i]<=k2)
            {
                results.push_back(leaf->m_values[i]);
            }
            if(leaf->m_keys[i]>k2)
            {
                return results ; //am depasit intervalul    
            }

        }
        leaf=leaf->m_next;
    }
   return results; 

}


template <typename K, typename V>
BPlusTree<K, V>::BPlusTree(int order) : m_root(nullptr), m_order(order)
{
    m_root=new LeafNode<K,V>();
}

template <typename K, typename V>
BPlusTree<K, V>::~BPlusTree()
{
    delete m_root;
}




template <typename K, typename V>
void BPlusTree<K, V>::splitLeaf(LeafNode<K, V>* leaf, InternalNode<K, V>* parent,int index)
{

    LeafNode<K,V>* newLeaf= new LeafNode<K,V>();
    int mid = m_order; // mijlocul pentru split

    //mut 1/2 la drepta
    newLeaf->m_keys.assign(leaf->m_keys.begin()+mid,leaf->m_keys.end());
    newLeaf->m_values.assign(leaf->m_values.begin()+mid,leaf->m_values.end());

    //pastrez 1/2 la stanga
    leaf->m_keys.resize(mid);
    leaf->m_values.resize(mid);

    //leg noul leaf
    newLeaf->m_next=leaf->m_next;
    leaf->m_next=newLeaf;

    //prima cheie din noul leaf 
    K keyToPromote=newLeaf->m_keys[0];

    //fac noul root daca nu am parinte
    if(m_root==leaf)
    {
        InternalNode<K,V>* newRoot =new InternalNode<K,V>();
        newRoot->m_keys.push_back(keyToPromote);
        newRoot->m_children.push_back(leaf);
        newRoot->m_children.push_back(newLeaf);
        m_root=newRoot;
    }
    else
    {
        insertInParent(leaf,keyToPromote,newLeaf);
    }

}


template <typename K, typename V>
void BPlusTree<K, V>::insert(const K& key, const V& value)
{
    LeafNode<K,V>*leaf =findLeaf(key);


    //gasesc pozitia 
    int i = 0;
    while(i<(int)leaf->m_keys.size() && leaf->m_keys[i]<key)
    {
        i++;
    }

    //daca exista key_update
    if(i<(int)leaf->m_keys.size() && leaf->m_keys[i]==key)
    {
        leaf->m_values[i] =value; //update
        return;
    }

    //inserez la pozitia buna
    leaf->insertAt(i,key,value);

    //verific daca trebuie sa fac split
    if((int)leaf->m_keys.size()>=2*m_order)
    {
        splitLeaf(leaf,nullptr,0);
    }

}


template <typename K, typename V>
void BPlusTree<K, V>::splitInternal(InternalNode<K, V>* node,  InternalNode<K, V>* parent, int index)
{
    InternalNode<K,V>* newNode = new InternalNode<K,V>();
    int mid = m_order-1;   //mijlocul pentru split

    //mid urca root
    K keyToPromote=node->m_keys[mid];

    //mut 1/2 la dreapta
    newNode->m_keys.assign(node->m_keys.begin()+mid+1,node->m_keys.end());
    newNode->m_children.assign(node->m_children.begin()+mid+1,node->m_children.end());

    //mut 1/2 la stanga
    node->m_keys.resize(mid);
    node->m_children.resize(mid+1);


    if(m_root==node)
    {
        InternalNode<K,V>* newRoot =new InternalNode<K,V>();
        newRoot->m_keys.push_back(keyToPromote);
        newRoot->m_children.push_back(node);
        newRoot->m_children.push_back(newNode);
        m_root=newRoot;
    }
    else
    {
        insertInParent(node,keyToPromote,newNode);
    }

}

template <typename K, typename V>
void BPlusTree<K, V>::insertInParent(BPlusNode<K, V>* left, const K& key,  BPlusNode<K, V>* right) 
{
    //caut parintele
    if(m_root->isLeaf()) return; //nu am parinte

    BPlusNode<K,V>* current=m_root;

    //stack pentru a gasi parintele

    std::vector<InternalNode<K,V>*> path;
    InternalNode<K,V>* parent = findParent(left, path);

    if (!parent) return;



    //gasesc pozitia pentru noul key
    int idx=0;
    while(idx<(int)parent->m_keys.size() && key > parent->m_keys[idx])
    {
        idx++;
    }

    parent->insertAt(idx,key,right);

    //verific daca trebuie sa fac split
    if((int)parent->m_keys.size()>=2*m_order)
    {
        InternalNode<K,V>* grandParent =path.size()>1 ? path[path.size()-2]: nullptr;
        splitInternal(parent,grandParent,idx);
    }

}

template <typename K, typename V>
inline int BPlusTree<K, V>::findChildIndex(InternalNode<K, V> *parent, BPlusNode<K, V> *child)
{
    for(int i = 0 ;i<(int)parent->m_children.size();i++)
    {
        if(parent->m_children[i]==child)
        return i;
    }

    return -1;
}

template <typename K, typename V>
inline InternalNode<K, V> *BPlusTree<K, V>::findParent(BPlusNode<K, V> *target, std::vector<InternalNode<K, V> *> &path)
{
    //parinte + path

    if (target->m_keys.empty()) return nullptr;

    if (m_root == target) return nullptr;

    BPlusNode<K, V>* current = m_root;
    path.clear();

    while (!current->isLeaf()) 
    {
        InternalNode<K, V>* node = static_cast<InternalNode<K, V>*>(current);
        for (auto child : node->m_children) 
        {
            if (child == target) 
            {
                path.push_back(node);
                return node;
            }
        }
        path.push_back(node);
        // cobor spre target
        int i = 0;
        while (i < (int)node->m_keys.size() &&target->m_keys[0] >= node->m_keys[i]) 
        {
            i++;
        }
        current = node->m_children[i];
    }
    return nullptr;
}

template <typename K, typename V>
inline void BPlusTree<K, V>::removeFromLeaf(LeafNode<K, V> *leaf, const K &key, std::vector<InternalNode<K, V> *> &path)
{
    // sterg cheia din leaf
    int i = 0;
    while (i < (int)leaf->m_keys.size() && leaf->m_keys[i] != key) i++;
    
    if (i == (int)leaf->m_keys.size())
    {
        std::cout << "Key not found!" <<std::endl;
        return;
    }
    leaf->removeAt(i);

    // cazul 1 — suficiente chei, gata
    if ((int)leaf->m_keys.size() >= m_order || m_root == leaf) return;

    // underflow — trebuie să rezolvăm
    InternalNode<K, V>* parent = path.empty() ? nullptr : path.back();
    if (!parent) return;

    int childIdx = findChildIndex(parent, leaf);

    // cazul 2a — redistribute de la fratele din dreapta
    if (childIdx < (int)parent->m_children.size() - 1) 
    {
        LeafNode<K, V>* rightSibling = static_cast<LeafNode<K, V>*>(parent->m_children[childIdx + 1]);

        if ((int)rightSibling->m_keys.size() > m_order) 
        {
            // împrumutăm prima cheie din fratele drept
            leaf->insertAt((int)leaf->m_keys.size(),
                           rightSibling->m_keys[0],
                           rightSibling->m_values[0]);
            rightSibling->removeAt(0);

            // actualizez cheia din parinte
            parent->m_keys[childIdx] = rightSibling->m_keys[0];
            return;
        }
    }

    // cazul 2b — redistribute de la fratele din stânga
    if (childIdx > 0) 
    {
        LeafNode<K, V>* leftSibling = static_cast<LeafNode<K, V>*>(parent->m_children[childIdx - 1]);

        if ((int)leftSibling->m_keys.size() > m_order) 
        {
            // împrumutăm ultima cheie din fratele stâng
            int lastIdx = (int)leftSibling->m_keys.size() - 1;
            leaf->insertAt(0, leftSibling->m_keys[lastIdx],leftSibling->m_values[lastIdx]);
            leftSibling->removeAt(lastIdx);

            // actualizez cheia din parinte
            parent->m_keys[childIdx - 1] = leaf->m_keys[0];
            return;
        }
    }

    // cazul 2c — merge cu fratele din dreapta
    if (childIdx < (int)parent->m_children.size() - 1) 
    {
        LeafNode<K, V>* rightSibling = static_cast<LeafNode<K, V>*>( parent->m_children[childIdx + 1]);

        // mut tot din fratele drept în leaf
        for (int j = 0; j < (int)rightSibling->m_keys.size(); j++) 
        {
            leaf->m_keys.push_back(rightSibling->m_keys[j]);
            leaf->m_values.push_back(rightSibling->m_values[j]);
        }

        // actualizăm linked list-ul
        leaf->m_next = rightSibling->m_next;

        // sterg cheia și copilul din parinte
        parent->m_keys.erase(parent->m_keys.begin() + childIdx);
        parent->m_children.erase(parent->m_children.begin() + childIdx + 1);
        rightSibling->m_next = nullptr; // ca să nu fac delete recursiv
        delete rightSibling;

        // verific dacă parintele a intrat în underflow
        fixInternalUnderflow(parent, path);
        return;
    }

    // cazul 2c — merge cu fratele din stanga
    if (childIdx > 0) 
    {
        LeafNode<K, V>* leftSibling = static_cast<LeafNode<K, V>*>(parent->m_children[childIdx - 1]);

        // mut tot din leaf în fratele stang
        for (int j = 0; j < (int)leaf->m_keys.size(); j++) 
        {
            leftSibling->m_keys.push_back(leaf->m_keys[j]);
            leftSibling->m_values.push_back(leaf->m_values[j]);
        }

        leftSibling->m_next = leaf->m_next;

        parent->m_keys.erase(parent->m_keys.begin() + childIdx - 1);
        parent->m_children.erase(parent->m_children.begin() + childIdx);
        leaf->m_next = nullptr;
        delete leaf;

        fixInternalUnderflow(parent, path);
    }
}

template <typename K, typename V>
inline void BPlusTree<K, V>::fixInternalUnderflow(InternalNode<K, V> *node, std::vector<InternalNode<K, V> *> &path)
{
    // root poate rămâne cu 0 chei — devine copilul sau
    if (node == m_root) {
        if (node->m_keys.empty() && !node->m_children.empty()) 
        {
            m_root = node->m_children[0];
            node->m_children.clear();
            delete node;
        }
        return;
    }

    // suficiente chei = gata
    if ((int)node->m_keys.size() >= m_order - 1) return;

    // gasesc parintele
    path.pop_back();
    if (path.empty()) return;
    InternalNode<K, V>* parent = path.back();

    int childIdx = findChildIndex(parent, node);

    // redistribuit de la fratele din dreapta
    if (childIdx < (int)parent->m_children.size() - 1) 
    {
        InternalNode<K, V>* rightSibling = static_cast<InternalNode<K, V>*>(parent->m_children[childIdx + 1]);

        if ((int)rightSibling->m_keys.size() > m_order - 1) 
        {
            node->m_keys.push_back(parent->m_keys[childIdx]);
            node->m_children.push_back(rightSibling->m_children[0]);

            parent->m_keys[childIdx] = rightSibling->m_keys[0];
            rightSibling->m_keys.erase(rightSibling->m_keys.begin());
            rightSibling->m_children.erase(rightSibling->m_children.begin());
            return;
        }
    }

    // redistribuit de la fratele din stanga
    if (childIdx > 0) 
    {
        InternalNode<K, V>* leftSibling = static_cast<InternalNode<K, V>*>(
            parent->m_children[childIdx - 1]);

        if ((int)leftSibling->m_keys.size() > m_order - 1)
         {
            node->m_keys.insert(node->m_keys.begin(),parent->m_keys[childIdx - 1]);
            node->m_children.insert(node->m_children.begin(),leftSibling->m_children.back());

            parent->m_keys[childIdx - 1] = leftSibling->m_keys.back();
            leftSibling->m_keys.pop_back();
            leftSibling->m_children.pop_back();
            return;
        }
    }

    // merg cu fratele din dreapta
    if (childIdx < (int)parent->m_children.size() - 1) 
    {
        InternalNode<K, V>* rightSibling = static_cast<InternalNode<K, V>*>(
            parent->m_children[childIdx + 1]);

        node->m_keys.push_back(parent->m_keys[childIdx]);
        for (auto& k : rightSibling->m_keys) node->m_keys.push_back(k);
        for (auto& c : rightSibling->m_children) node->m_children.push_back(c);

        parent->m_keys.erase(parent->m_keys.begin() + childIdx);
        parent->m_children.erase(parent->m_children.begin() + childIdx + 1);

        rightSibling->m_children.clear();
        delete rightSibling;

        fixInternalUnderflow(parent, path);
        return;
    }

    // merge cu fratele din stanga
    if (childIdx > 0) {
        InternalNode<K, V>* leftSibling = static_cast<InternalNode<K, V>*>(parent->m_children[childIdx - 1]);

        leftSibling->m_keys.push_back(parent->m_keys[childIdx - 1]);
        for (auto& k : node->m_keys) leftSibling->m_keys.push_back(k);
        for (auto& c : node->m_children) leftSibling->m_children.push_back(c);

        parent->m_keys.erase(parent->m_keys.begin() + childIdx - 1);
        parent->m_children.erase(parent->m_children.begin() + childIdx);

        node->m_children.clear();
        delete node;

        fixInternalUnderflow(parent, path);
    }
}

template <typename K, typename V>
void BPlusTree<K, V>::display() const
{

    if(!m_root) 
    {
        std::cout<<"Empty tree\n";
        return;
    }

    std::vector<BPlusNode<K,V>*> current_level;
    std::vector<BPlusNode<K,V>*> next_level;

    current_level.push_back(m_root); 

    int level = 0;
    while(!current_level.empty())
    {
        std::cout << "Nivel " << level++ << ": ";
        for(auto* node : current_level) 
        {
            node->display();
            std::cout<<" | ";
            
            if(!node->isLeaf())
             {
                InternalNode<K,V>* internal=static_cast<InternalNode<K,V>*>(node);
                for(auto* child : internal->m_children)
                {
                    next_level.push_back(child);
                }
             }
        }
        std::cout<<"\n";
        current_level=next_level;
        next_level.clear();
    }

}