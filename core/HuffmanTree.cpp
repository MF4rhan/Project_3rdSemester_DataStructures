#include "HuffmanTree.h"
#include "MinHeap.h"
#include <string>
#include <fstream>
using namespace std;

void generateCodes(Node* root, string code, string codes[]) {
	if (root == nullptr) return;
	
	if (root->left == nullptr && root->right == nullptr) {
		codes[root->character] = (code == "") ? "0" : code;
		return;
	}
	
	generateCodes(root->left, code + '0', codes);
	generateCodes(root->right, code + '1', codes);
}

Node* buildHuffmanTree(int freq[]) {
	MinHeap myMinHeap;

    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) myMinHeap.insert(new Node(static_cast<unsigned char>(i), freq[i]));
        //static_cast expicity converts between types
    }
    
    while (myMinHeap.getSize() > 1) {
    	Node* leftNode = myMinHeap.extractMin();
    	Node* rightNode = myMinHeap.extractMin();
    	
    	Node* parent = new Node('\0', leftNode->freq + rightNode->freq);
    	
    	parent->left = leftNode;
    	parent->right = rightNode;
    	
    	myMinHeap.insert(parent);
	}
	
	return myMinHeap.extractMin();
}

void serializeHuffmanTree(Node* root, ofstream& out) {
	if (root == nullptr) return;
	
	if (root->left == nullptr && root->right == nullptr) {
		out.put(1);
		out.put(root->character);
	}
	else {
		out.put(0);
		serializeHuffmanTree(root->left, out);
		serializeHuffmanTree(root->right, out);
	}
}
