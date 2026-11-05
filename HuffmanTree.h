#pragma once
#include "Node.h"
#include <fstream>
#include <string>
using namespace std;

void generateCodes(Node* root, string code, string codes[]);
void serializeHuffmanTree(Node* root, ofstream& out);
