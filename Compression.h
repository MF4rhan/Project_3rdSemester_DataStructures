#pragma once

#include <string>

using namespace std;

// Main compression function.
// Takes the path of the original input file and the target path for the compressed output file.
// Returns true if compression succeeded, or false if an error occurred.
bool compressFile(const string& input_file_path, const string& output_file_path);