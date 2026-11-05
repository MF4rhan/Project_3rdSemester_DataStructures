#pragma once
#include "Node.h"
#include <fstream>
#include <string>
using namespace std;

void generateCodes(Node* root, string code, string codes[]);
Node* buildHuffmanTree(int freq[]);
void serializeHuffmanTree(Node* root, ofstream& out);

