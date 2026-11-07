#pragma once

#include <string>
#include <fstream>
#include "Node.h"

using namespace std;

// Function to reconstruct the Huffman tree from the serialized binary data.
// It reads the preorder sequence saved during compression (0 for internal node, 1 for leaf node).
// Returns a pointer to the root Node of the reconstructed Huffman tree.
Node* deserializeTree(ifstream& in_file);

// Helper function to recursively free heap memory used by the tree nodes.
// It traverses the tree in postorder (left child, right child, then root node) to avoid memory leaks.
void deleteTree(Node* root_node);

// Main decompression function.
// Takes the path of the compressed input file and the target path for the decompressed output file.
// Returns true if decompression succeeded, or false if an error occurred.
bool decompressFile(const string& input_file_path, const string& output_file_path);
