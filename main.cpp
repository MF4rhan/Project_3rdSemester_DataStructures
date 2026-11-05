#include <iostream>
#include <string>
#include <fstream>
#include "HuffmanTree.h"

using namespace std;

int main() {
    int freq[256] = {0};
    string inString = "ASDWRASDW";

    for (unsigned char c : inString) freq[c]++;

    Node* root = buildHuffmanTree(freq);

    string codes[256] = {};

    generateCodes(root, "", codes);

    // Display generated codes for testing
    for (int i = 0; i < 256; i++) {
        if (codes[i] != "")
            cout << static_cast<char>(i) << " : " << codes[i] << endl;
    }

    return 0;
}
