#pragma once
#include "Node.h"

class MinHeap {
private:
        Node* heap[256];
        int size;
        
public:
	  MinHeap(int s = 0);
  	int getSize() const;
    void insert(Node * newNode);
    Node* extractMin();
    void heapifyUp(int index);
    void heapifyDown(int index);
       
};
