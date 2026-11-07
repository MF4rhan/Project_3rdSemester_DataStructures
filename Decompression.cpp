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
// - Marker 0 indicates an internal node, followed by the left subtree and right subtree.
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
    // Marker 0 indicates an internal node
    else if (marker_byte == 0)
    {
        // Internal nodes do not hold characters, so we store a null char '\0'
        Node* internal_node = new Node('\0', 0);

        // Recursively reconstruct the left subtree first
        internal_node->left = deserializeTree(in_file);

        // Then recursively reconstruct the right subtree
        internal_node->right = deserializeTree(in_file);

        // Safety check: if file was truncated and either child failed to build
        if (internal_node->left == nullptr || internal_node->right == nullptr)
        {
            deleteTree(internal_node);
            return nullptr;
        }

        return internal_node;
    }

    return nullptr;
}

// Main decompression function.
// Takes the compressed input file path and the destination output file path.
// Opens binary streams, validates them, and will coordinate decompression in subsequent steps.
bool decompressFile(const string& input_file_path, const string& output_file_path)
{
    // Open the compressed file in binary mode
    ifstream in_file(input_file_path, ios::binary);
    if (!in_file.is_open())
    {
        cerr << "Error: Could not open input file for decompression: " << input_file_path << endl;
        return false;
    }

    // Open the output file in binary mode to write the recovered data
    ofstream out_file(output_file_path, ios::binary);
    if (!out_file.is_open())
    {
        cerr << "Error: Could not open output file for decompression: " << output_file_path << endl;
        in_file.close();
        return false;
    }

    // Decompression steps will be added here in upcoming commits

    // Close the file streams
    in_file.close();
    out_file.close();

    return true;
}
