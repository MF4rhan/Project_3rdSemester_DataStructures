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
       void insert(Node * node) {
            int currentIndex = size;
            heap[size++] = node;
            
       }
       
       Node* extractMin() {
             
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
             
       }
};


int main() {
    Node * nodeArray = new * Node[256];
    int freq[256] = {0};
    string inString = "ASDWRASDW";
    
    for (char c : inString) freq[c]++;
    
    int nodeArrayi = 0;
    
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) nodeArray[nodeArrayi++] = new Node(char(i), freq[i]);
    }
    
    
    
    
    return 0;
}
