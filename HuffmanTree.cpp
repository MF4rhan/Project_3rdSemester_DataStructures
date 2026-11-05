#include <iostream>
using namespace std;
#include <string>
#include <fstream>

class Node {
public:
       char character;
       int freq;
       Node * left;
       Node * right;
       
       Node(char c = 'g', int f = 0) : character(c), freq(f), left(nullptr), right(nullptr) {}

};


class MinHeap {
private:
        Node* heap[256];
        int size;
        
public:
		MinHeap(int s) : size(s) {}
		
		int getSize() const { return size; }
	
       	void insert(Node * newNode) {
       		heap[size] = newNode;
       		size++;
       		heapifyUp(size - 1);
       }
       
       	Node* extractMin() {
	   		if (size == 0) return nullptr;
	   		
	   		Node* minNode = heap[0];
	   		
	   		heap[0] = heap[size - 1];
	   		size--;
	   		
	   		if (size > 0) heapifyDown(0);
	   		
	   		return minNode;
       }
       
       
       	void heapifyUp(int index) {
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
       
       
       	void heapifyDown(int index) {
            int smallest = index;
            
            while (true) {
            	int leftChild = 2 * index + 1;
            	int right = 2 * index + 2;
            	int smallest = index;
            	
            	if (left < size && heap[left]->freq < heap[smallest]->freq) smallest = left;
            	
            	if (right < size && heap[right]->freq < heap[smallest]->freq) smallest = right;
            	
            	if (smallest == index) break;
            	
            	Node * temp = heap[index];
            	heap[index] = heap[smallest];
            	heap[smallest] = temp;
            	
            	index = smallest;
			}
       }
       
};


int main() {
	
	MinHeap myMinHeap;
	
    Node * nodeArray = new * Node[256];
    int freq[256] = {0};
    string inString = "ASDWRASDW";
    
    for (char c : inString) freq[c]++;
    
    int nodeArrayi = 0;
    
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) myMinHeap.insert(new Node(char(i), freq[i]));
    }
    
    while (myMinHeap.getSize() > 1) {
    	Node* leftNode = myMinHeap.extractMin();
    	Node* rightNode = myMinHeap.extractMin();
    	
    	Node* parent = new Node('\0', leftNode->freq + rightNode->freq);
    	
    	parent->left = leftNode;
    	parent->right = rightNode;
    	
    	myMinHeap.insert(parent);
	}
	
	Node * root = myMinHeap.extractMin();
    
    
    
    
    
    return 0;
}
