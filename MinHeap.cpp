#include "MinHeap.h"

MinHeap::MinHeap(int s) : size(s) {}

int MinHeap::getSize() const { return size; }
	
void MinHeap::insert(Node * newNode) {
	heap[size] = newNode;
  size++;
  heapifyUp(size - 1);
}
       
Node* MinHeap::extractMin() {
	if (size == 0) return nullptr;
	   		
	Node* minNode = heap[0];
	   		
	heap[0] = heap[size - 1];
	size--;
	   		
	if (size > 0) heapifyDown(0);
	   		
	return minNode;
}
       
       
void MinHeap::heapifyUp(int index) {
	while (index > 0) {
  	int parentIndex = (index - 1) / 2;
                   
  	if (heap[index]->freq < heap[parentIndex]->freq) {
  		Node * temp = heap[index];
  		heap[index] = heap[parentIndex];
  		heap[parentIndex] = temp;
                     
  		index = parentIndex;            
  	} else {
   			break;
  	}
	}
}
       
       
void MinHeap::heapifyDown(int index) {      
	while (true) {
  	int leftChild = 2 * index + 1;
    int rightChild = 2 * index + 2;
    int smallest = index;
            	
    if (leftChild < size && heap[leftChild]->freq < heap[smallest]->freq) smallest = leftChild;
           	
    if (rightChild < size && heap[rightChild]->freq < heap[smallest]->freq) smallest = rightChild;
            	
  	if (smallest == index) break;
            	
    Node * temp = heap[index];
  	heap[index] = heap[smallest];
    heap[smallest] = temp;
           	
    index = smallest;
	}
}
