#pragma once

#include <string>

using namespace std;

// Main compression function.
// Takes the path of the original input file and the target path for the compressed output file.
// Optional parameters return error details and the 256-entry frequency/code tables for UI callers.
// Returns true if compression succeeded, or false if an error occurred.
bool compressFile(
    const string& input_file_path,
    const string& output_file_path,
    string* error_message = nullptr,
    int* out_frequencies = nullptr,
    string* out_codes = nullptr);