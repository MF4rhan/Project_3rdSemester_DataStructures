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
