#include "HuffmanTree.h"
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
