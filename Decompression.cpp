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

    // Check if the compressed file itself is completely empty (0 bytes)
    in_file.seekg(0, ios::end);
    streampos file_length = in_file.tellg();
    in_file.seekg(0, ios::beg);

    if (file_length == 0)
    {
        in_file.close();
        out_file.close();
        return true;
    }

    // Step 1: Read metadata (original file size - 8 bytes)
    uint64_t original_file_size = 0;
    in_file.read(reinterpret_cast<char*>(&original_file_size), sizeof(original_file_size));

    // Verify that all 8 bytes of metadata were successfully read
    if (in_file.gcount() < static_cast<streamsize>(sizeof(original_file_size)))
    {
        cerr << "Error: Could not read header metadata from file." << endl;
        in_file.close();
        out_file.close();
        return false;
    }

    // If the original file was empty, nothing left to decode
    if (original_file_size == 0)
    {
        in_file.close();
        out_file.close();
        return true;
    }

    // Step 2: Reconstruct the Huffman tree from the serialized bytes
    Node* root_node = deserializeTree(in_file);
    if (root_node == nullptr)
    {
        cerr << "Error: Failed to deserialize Huffman tree from compressed file." << endl;
        in_file.close();
        out_file.close();
        return false;
    }

    // Step 3: Handle special edge case: file contains only 1 unique character (e.g. "AAAAA")
    // When there is only 1 unique character, the root itself is a leaf node with no children.
    if (root_node->left == nullptr && root_node->right == nullptr)
    {
        // Write the single character original_file_size times to restore the exact original content
        for (uint64_t i = 0; i < original_file_size; i++)
        {
            out_file.put(root_node->character);
        }

        // Clean up heap memory and close streams
        deleteTree(root_node);
        in_file.close();
        out_file.close();
        return true;
    }

    // Step 4: Decode data using bit-by-bit tree traversal
    uint64_t decoded_bytes_count = 0;
    Node* current_node = root_node;
    char buffer_char;

    // Read byte by byte from the compressed file
    while (decoded_bytes_count < original_file_size && in_file.get(buffer_char))
    {
        unsigned char current_byte = static_cast<unsigned char>(buffer_char);

        // Read 8 bits: from bit 7 (MSB) down to bit 0 (LSB)
        for (int bit_index = 7; bit_index >= 0; bit_index--)
        {
            int bit_val = (current_byte >> bit_index) & 1;

            // Bit 0 means traverse left child; Bit 1 means traverse right child
            if (bit_val == 0)
            {
                current_node = current_node->left;
            }
            else
            {
                current_node = current_node->right;
            }

            // Safety check: if traversal leads to a null pointer, file is corrupted
            if (current_node == nullptr)
            {
                cerr << "Error: Corrupted bitstream encountered during tree traversal." << endl;
                deleteTree(root_node);
                in_file.close();
                out_file.close();
                return false;
            }

            // If we reached a leaf node, we found an original character!
            if (current_node->left == nullptr && current_node->right == nullptr)
            {
                out_file.put(current_node->character);
                decoded_bytes_count++;
                current_node = root_node; // Reset back to root for the next character
            }
        }
    }

    // Clean up allocated tree memory to prevent memory leaks
    deleteTree(root_node);

    // Close the file streams
    in_file.close();
    out_file.close();

    return true;
}
