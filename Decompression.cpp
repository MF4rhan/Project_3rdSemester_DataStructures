#include "Decompression.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>

using namespace std;

// Recursively frees memory allocated for the Huffman tree.
// We use postorder traversal (left child, right child, then current node)
// so that children are freed before the parent pointer is destroyed.
void deleteTree(Node* root_node)
{
    // Base case: if the node pointer is null, nothing to delete
    if (root_node == nullptr)
    {
        return;
    }

    // First delete the left subtree
    deleteTree(root_node->left);

    // Next delete the right subtree
    deleteTree(root_node->right);

    // Finally delete the current node from the heap
    delete root_node;
}

// Reconstructs the Huffman tree from the serialized binary data in the file.
// In Member 1's serialization:
// - Marker 1 indicates a leaf node, followed by 1 byte for the character.
// - Marker 0 indicates an internal node (to be added in next commit).
Node* deserializeTree(ifstream& in_file)
{
    // Read the 1-byte marker
    int marker_byte = in_file.get();

    // If we reached the end of the file unexpectedly, return null
    if (marker_byte == EOF)
    {
        return nullptr;
    }

    // Marker 1 indicates a leaf node
    if (marker_byte == 1)
    {
        int char_byte = in_file.get();
        if (char_byte == EOF)
        {
            return nullptr;
        }

        // Create a new leaf node storing the character
        return new Node(static_cast<unsigned char>(char_byte), 0);
    }

    return nullptr;
}
