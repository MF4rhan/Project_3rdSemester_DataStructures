#include <iostream>
#include <string>
#include "Compression.h"
#include "Decompression.h"

using namespace std;

int main() {

    int choice;
    string inputFile, outputFile;
    cout << "===== HUFFMAN FILE COMPRESSION =====\n";

    do {
        cout << "1. Compress a file\n";
        cout << "2. Decompress a file\n";
        cout << "3. Exit\n";
        cout << "Enter choice: ";
        cin >> choice;
		if (choice == 3) {
			cout << "Exiting program.\n";
			break;
		}
        
        cout << "Enter input file path: ";
        cin >> inputFile;

        cout << "Enter output file path: ";
        cin >> outputFile;

        bool success = false;

        if (choice == 1) {
            success = compressFile(inputFile, outputFile);
        }
        else if (choice == 2) {
            success = decompressFile(inputFile, outputFile);
        }
        else {
            cout << "Invalid choice!\n";
            return 1;
        }

        if (success)
            cout << "\nOperation completed successfully!\n\n";
        else
            cout << "\nOperation failed!\n\n";
	} while (true);
    return 0;
}
