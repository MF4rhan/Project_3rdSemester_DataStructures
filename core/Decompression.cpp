#include "Decompression.h"
#include "HuffmanTree.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
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

    filesystem::path makeUniqueSiblingPath(const filesystem::path& dest_path, const char* tag)
    {
        static atomic<uint64_t> counter{0};
        const filesystem::path dir = dest_path.has_parent_path()
            ? dest_path.parent_path()
            : filesystem::path(".");
        const uint64_t tick = static_cast<uint64_t>(
            chrono::steady_clock::now().time_since_epoch().count());

        for (int attempt = 0; attempt < 64; ++attempt)
        {
            const uint64_t seq = counter.fetch_add(1, memory_order_relaxed);
            string suffix = string(".") + tag + "_" + to_string(tick) + "_" + to_string(seq);
            filesystem::path candidate = dir / (dest_path.filename().native() + filesystem::path(suffix).native());
            error_code ec;
            if (!filesystem::exists(candidate, ec))
            {
                return candidate;
            }
        }
        return dir / (dest_path.filename().native() + filesystem::path(string(".") + tag).native());
    }

    void removeQuietly(const filesystem::path& p)
    {
        error_code ec;
        filesystem::remove(p, ec);
    }

    bool publishTempFile(const filesystem::path& tmp_path, const filesystem::path& dest_path, string* error_message, const string& dest_utf8)
    {
        error_code ec;
        filesystem::rename(tmp_path, dest_path, ec);
        if (!ec)
        {
            return true;
        }

        // Portable fallback if rename cannot directly replace an existing destination
        error_code exists_ec;
        if (filesystem::exists(dest_path, exists_ec))
        {
            const filesystem::path backup_path = makeUniqueSiblingPath(dest_path, "huff_bak");
            error_code bak_ec;
            filesystem::rename(dest_path, backup_path, bak_ec);
            if (bak_ec)
            {
                removeQuietly(tmp_path);
                reportError(error_message, "Could not replace existing output file: " + dest_utf8 + " (" + bak_ec.message() + ")");
                return false;
            }

            error_code replace_ec;
            filesystem::rename(tmp_path, dest_path, replace_ec);
            if (!replace_ec)
            {
                removeQuietly(backup_path);
                return true;
            }

            // Restore original file before failing
            error_code restore_ec;
            filesystem::rename(backup_path, dest_path, restore_ec);
            removeQuietly(tmp_path);
            reportError(error_message, "Could not finalize output file: " + dest_utf8 + " (" + replace_ec.message() + ")");
            return false;
        }

        removeQuietly(tmp_path);
        reportError(error_message, "Could not finalize output file: " + dest_utf8 + " (" + ec.message() + ")");
        return false;
    }
}

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
bool decompressFile(
    const string& input_file_path,
    const string& output_file_path,
    string* error_message,
    string* out_codes)
{
    if (error_message != nullptr)
    {
        error_message->clear();
    }
    if (out_codes != nullptr)
    {
        for (int i = 0; i < 256; i++)
        {
            out_codes[i].clear();
        }
    }

    // Open the compressed file in binary mode
    ifstream in_file(pathFromUtf8(input_file_path), ios::binary);
    if (!in_file.is_open())
    {
        reportError(error_message, "Could not open input file for decompression: " + input_file_path);
        return false;
    }

    const filesystem::path dest_path = pathFromUtf8(output_file_path);
    if (dest_path.has_parent_path())
    {
        error_code dir_ec;
        if (!filesystem::exists(dest_path.parent_path(), dir_ec) || !filesystem::is_directory(dest_path.parent_path(), dir_ec))
        {
            reportError(error_message, "Output directory does not exist for decompression: " + output_file_path);
            in_file.close();
            return false;
        }
    }

    // Check compressed file length before creating any output file
    in_file.seekg(0, ios::end);
    streampos file_length = in_file.tellg();
    in_file.seekg(0, ios::beg);

    auto writeEmptyOutput = [&]() -> bool {
        const filesystem::path tmp_path = makeUniqueSiblingPath(dest_path, "huff_tmp");
        ofstream out_file(tmp_path, ios::binary | ios::trunc);
        if (!out_file.is_open())
        {
            reportError(error_message, "Could not open output file for decompression: " + output_file_path);
            return false;
        }
        out_file.flush();
        out_file.close();
        if (!out_file)
        {
            removeQuietly(tmp_path);
            reportError(error_message, "Failed while writing decompressed data to: " + output_file_path);
            return false;
        }
        return publishTempFile(tmp_path, dest_path, error_message, output_file_path);
    };

    if (file_length == 0)
    {
        in_file.close();
        return writeEmptyOutput();
    }

    // Step 1: Read metadata (original file size - 8 bytes)
    uint64_t original_file_size = 0;
    in_file.read(reinterpret_cast<char*>(&original_file_size), sizeof(original_file_size));

    // Verify that all 8 bytes of metadata were successfully read
    if (in_file.gcount() < static_cast<streamsize>(sizeof(original_file_size)))
    {
        reportError(error_message, "Could not read header metadata from compressed file.");
        in_file.close();
        return false;
    }

    // If the original file was empty, nothing left to decode
    if (original_file_size == 0)
    {
        in_file.close();
        return writeEmptyOutput();
    }

    // Step 2: Reconstruct the Huffman tree from the serialized bytes before touching destination output
    Node* root_node = deserializeTree(in_file);
    if (root_node == nullptr)
    {
        reportError(error_message, "Failed to deserialize Huffman tree. The file may not be a valid Huffman-compressed file.");
        in_file.close();
        return false;
    }

    // Sanity check for multi-symbol trees: each encoded byte requires at least 1 bit in the payload
    if (root_node->left != nullptr || root_node->right != nullptr)
    {
        const streampos tree_end_pos = in_file.tellg();
        if (tree_end_pos != streampos(-1) && file_length >= tree_end_pos)
        {
            const uint64_t remaining_bytes = static_cast<uint64_t>(file_length - tree_end_pos);
            if (remaining_bytes <= (UINT64_MAX / 8) && original_file_size > remaining_bytes * 8)
            {
                reportError(error_message, "Compressed file header specifies an invalid or truncated data size.");
                deleteTree(root_node);
                in_file.close();
                return false;
            }
        }
    }

    if (out_codes != nullptr)
    {
        generateCodes(root_node, "", out_codes);
    }

    // Open a temporary file in the destination directory; publish only after verified completion
    const filesystem::path tmp_path = makeUniqueSiblingPath(dest_path, "huff_tmp");
    ofstream out_file(tmp_path, ios::binary | ios::trunc);
    if (!out_file.is_open())
    {
        reportError(error_message, "Could not open output file for decompression: " + output_file_path);
        deleteTree(root_node);
        in_file.close();
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
            if (!out_file)
            {
                break;
            }
        }

        // Clean up heap memory and close streams
        deleteTree(root_node);
        in_file.close();
        out_file.flush();
        out_file.close();
        if (!out_file)
        {
            removeQuietly(tmp_path);
            reportError(error_message, "Failed while writing decompressed data to: " + output_file_path);
            return false;
        }
        return publishTempFile(tmp_path, dest_path, error_message, output_file_path);
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
                reportError(error_message, "Corrupted bitstream encountered during tree traversal.");
                deleteTree(root_node);
                in_file.close();
                out_file.close();
                removeQuietly(tmp_path);
                return false;
            }

            // If we reached a leaf node, we found an original character!
            if (current_node->left == nullptr && current_node->right == nullptr)
            {
                out_file.put(current_node->character);
                decoded_bytes_count++;
                current_node = root_node; // Reset back to root for the next character

                // Padding protection: as soon as all original bytes are recovered,
                // stop decoding immediately so any trailing padding bits are ignored
                if (decoded_bytes_count == original_file_size)
                {
                    break;
                }
            }
        }
    }

    // Step 5: Clean up allocated tree memory to prevent memory leaks
    deleteTree(root_node);

    // Flush and close the file streams before checking write status
    in_file.close();
    out_file.flush();
    out_file.close();
    bool write_ok = static_cast<bool>(out_file);

    if (!write_ok)
    {
        removeQuietly(tmp_path);
        reportError(error_message, "Failed while writing decompressed data to: " + output_file_path);
        return false;
    }

    // Verify file integrity: ensure decoded byte count matches original file size
    if (decoded_bytes_count != original_file_size)
    {
        removeQuietly(tmp_path);
        reportError(
            error_message,
            "Decompressed size (" + to_string(decoded_bytes_count) +
            " bytes) does not match expected original size (" +
            to_string(original_file_size) + " bytes).");
        return false;
    }

    return publishTempFile(tmp_path, dest_path, error_message, output_file_path);
}
