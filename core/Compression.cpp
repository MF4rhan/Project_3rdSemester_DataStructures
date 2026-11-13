#include "Compression.h"
#include "HuffmanTree.h"
#include "Decompression.h"   // reused for deleteTree()
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <string_view>
#include <cstdint>

using namespace std;

namespace {
    filesystem::path pathFromUtf8(const string& path_str)
    {
        return filesystem::path(
            u8string_view(reinterpret_cast<const char8_t*>(path_str.data()), path_str.size()));
    }

    void reportError(string* error_message, const string& message)
    {
        cerr << message << endl;
        if (error_message != nullptr)
        {
            *error_message = message;
        }
    }
}

// Main compression function.
// Output file format (must match Decompression.cpp):
// 1) Metadata: original file size (8 bytes)
// 2) Serialized Huffman tree (preorder: 0 = internal node, 1 = leaf + character)
// 3) Compressed data: Huffman codes packed into bytes
bool compressFile(
    const string& input_file_path,
    const string& output_file_path,
    string* error_message,
    int* out_frequencies,
    string* out_codes)
{
    if (error_message != nullptr)
    {
        error_message->clear();
    }

    // Open the original file in binary mode so every byte is read exactly
    ifstream in_file(pathFromUtf8(input_file_path), ios::binary);
    if (!in_file.is_open())
    {
        reportError(error_message, "Could not open input file for compression: " + input_file_path);
        return false;
    }

    // Open the output file in binary mode to write the compressed data
    ofstream out_file(pathFromUtf8(output_file_path), ios::binary);
    if (!out_file.is_open())
    {
        reportError(error_message, "Could not open output file for compression: " + output_file_path);
        in_file.close();
        return false;
    }

    // Step 1: First pass - count how many times each byte appears
    int freq[256] = {0};
    uint64_t original_file_size = 0;
    int byte_read;

    while ((byte_read = in_file.get()) != EOF)
    {
        freq[byte_read]++;
        original_file_size++;
    }

    // Step 2: Write metadata (original file size - 8 bytes)
    // The decompressor uses this to know when to stop (ignores padding bits)
    out_file.write(reinterpret_cast<const char*>(&original_file_size), sizeof(original_file_size));

    // Edge case: empty file, only the metadata is needed
    if (original_file_size == 0)
    {
        if (out_frequencies != nullptr)
        {
            for (int i = 0; i < 256; i++)
            {
                out_frequencies[i] = 0;
            }
        }
        if (out_codes != nullptr)
        {
            for (int i = 0; i < 256; i++)
            {
                out_codes[i].clear();
            }
        }

        in_file.close();
        out_file.flush();
        out_file.close();
        bool write_ok = static_cast<bool>(out_file);
        if (!write_ok)
        {
            reportError(error_message, "Failed while writing compressed metadata to: " + output_file_path);
            return false;
        }
        return true;
    }

    // Step 3: Build the Huffman tree and generate a code for each byte (Member 1's functions)
    Node* root_node = buildHuffmanTree(freq);
    string codes[256] = {};
    generateCodes(root_node, "", codes);

    if (out_frequencies != nullptr)
    {
        for (int i = 0; i < 256; i++)
        {
            out_frequencies[i] = freq[i];
        }
    }
    if (out_codes != nullptr)
    {
        for (int i = 0; i < 256; i++)
        {
            out_codes[i] = codes[i];
        }
    }

    // Step 4: Save the tree so the decompressor can rebuild it
    serializeHuffmanTree(root_node, out_file);

    // Step 5: Second pass - go back to the start of the original file
    in_file.clear();
    in_file.seekg(0, ios::beg);

    // Bits are collected in this byte until it has 8 bits, then it is written out
    unsigned char bit_buffer = 0;
    int bit_count = 0;

    while ((byte_read = in_file.get()) != EOF)
    {
        // Get the Huffman code (a string of '0' and '1') for this byte
        const string& code = codes[byte_read];

        for (char bit_char : code)
        {
            // Shift buffer left and add the new bit at the end
            bit_buffer = static_cast<unsigned char>(bit_buffer << 1);
            if (bit_char == '1')
            {
                bit_buffer = bit_buffer | 1;
            }
            bit_count++;

            // When 8 bits are collected, write them as one byte
            if (bit_count == 8)
            {
                out_file.put(static_cast<char>(bit_buffer));
                bit_buffer = 0;
                bit_count = 0;
            }
        }
    }

    // Step 6: If the last byte is not full, fill the rest with 0 bits (padding)
    // Decompression ignores these because it stops after original_file_size bytes
    if (bit_count > 0)
    {
        bit_buffer = static_cast<unsigned char>(bit_buffer << (8 - bit_count));
        out_file.put(static_cast<char>(bit_buffer));
    }

    // Step 7: Free the tree memory and close the files
    deleteTree(root_node);
    in_file.close();
    out_file.flush();
    out_file.close();
    bool write_ok = static_cast<bool>(out_file);

    if (!write_ok)
    {
        reportError(error_message, "Failed while writing compressed data to: " + output_file_path);
        return false;
    }

    return true;
}