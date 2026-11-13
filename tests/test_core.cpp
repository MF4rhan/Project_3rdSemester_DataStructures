#include "Compression.h"
#include "Decompression.h"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

#define CHECK(cond)                                                              \
    do                                                                           \
    {                                                                            \
        if (!(cond))                                                             \
        {                                                                        \
            std::cerr << "CHECK FAILED at " << __FILE__ << ":" << __LINE__       \
                      << " -> " << #cond << std::endl;                           \
            std::exit(1);                                                        \
        }                                                                        \
    } while (false)

namespace {
    fs::path pathFromUtf8(const std::string& s)
    {
        return fs::path(std::u8string_view(reinterpret_cast<const char8_t*>(s.data()), s.size()));
    }

    std::string utf8FromPath(const fs::path& p)
    {
        const std::u8string u8 = p.u8string();
        return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
    }

    std::vector<unsigned char> readBytes(const fs::path& p)
    {
        std::ifstream f(p, std::ios::binary);
        CHECK(f.is_open());
        return std::vector<unsigned char>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    }

    void writeBytes(const fs::path& p, const std::vector<unsigned char>& bytes)
    {
        std::ofstream f(p, std::ios::binary | std::ios::trunc);
        CHECK(f.is_open());
        if (!bytes.empty())
        {
            f.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        }
        f.close();
        CHECK(static_cast<bool>(f));
    }

    void writeText(const fs::path& p, const std::string& text)
    {
        writeBytes(p, std::vector<unsigned char>(text.begin(), text.end()));
    }

    int countFilesInDir(const fs::path& dir)
    {
        int count = 0;
        for (const auto& entry : fs::directory_iterator(dir))
        {
            (void)entry;
            ++count;
        }
        return count;
    }
}

int main()
{
    const fs::path tempRoot = fs::temp_directory_path() / "huffman_core_tests";
    fs::remove_all(tempRoot);
    fs::create_directories(tempRoot);

    // 1. Existing sample fixture round-trip
    {
        const fs::path fixture = fs::path(HUFFMAN_FIXTURES_DIR) / "input.txt.txt";
        const fs::path huff = tempRoot / "fixture.huff";
        const fs::path out = tempRoot / "fixture_out.txt";
        std::string err;
        CHECK(compressFile(utf8FromPath(fixture), utf8FromPath(huff), &err));
        CHECK(decompressFile(utf8FromPath(huff), utf8FromPath(out), &err));
        CHECK(readBytes(fixture) == readBytes(out));
        std::cout << "[PASS] Fixture round-trip\n";
    }

    // 2. Binary 256-byte range & spaces in path
    {
        std::vector<unsigned char> data;
        for (int rep = 0; rep < 15; ++rep)
        {
            for (int b = 0; b < 256; ++b)
            {
                data.push_back(static_cast<unsigned char>(b));
            }
        }
        const fs::path in = tempRoot / "binary with spaces.bin";
        const fs::path huff = tempRoot / "binary with spaces.bin.huff";
        const fs::path out = tempRoot / "binary restored.bin";
        writeBytes(in, data);

        int freq[256] = {};
        std::string compCodes[256];
        std::string decompCodes[256];
        std::string err;

        CHECK(compressFile(utf8FromPath(in), utf8FromPath(huff), &err, freq, compCodes));
        CHECK(decompressFile(utf8FromPath(huff), utf8FromPath(out), &err, decompCodes));
        CHECK(readBytes(out) == data);
        for (int b = 0; b < 256; ++b)
        {
            CHECK(freq[b] == 15);
            CHECK(!compCodes[b].empty());
            CHECK(compCodes[b] == decompCodes[b]);
        }
        std::cout << "[PASS] Binary 256-byte, spaces in path, and code table match\n";
    }

    // 3. Unicode path round-trip
    {
        const fs::path in = pathFromUtf8(utf8FromPath(tempRoot) + "/café_sample.txt");
        const fs::path huff = pathFromUtf8(utf8FromPath(tempRoot) + "/café_sample.txt.huff");
        const fs::path out = pathFromUtf8(utf8FromPath(tempRoot) + "/café_restored.txt");
        const std::string text = "Huffman coding lossless test with Unicode path.\nLine 2!";
        writeText(in, text);

        std::string err;
        CHECK(compressFile(utf8FromPath(in), utf8FromPath(huff), &err));
        CHECK(decompressFile(utf8FromPath(huff), utf8FromPath(out), &err));
        CHECK(readBytes(out) == readBytes(in));
        std::cout << "[PASS] Unicode path round-trip\n";
    }

    // 4. Empty file and single repeated character
    {
        const fs::path emptyIn = tempRoot / "empty.dat";
        const fs::path emptyHuff = tempRoot / "empty.dat.huff";
        const fs::path emptyOut = tempRoot / "empty_out.dat";
        writeText(emptyIn, "");
        std::string err;
        CHECK(compressFile(utf8FromPath(emptyIn), utf8FromPath(emptyHuff), &err));
        CHECK(decompressFile(utf8FromPath(emptyHuff), utf8FromPath(emptyOut), &err));
        CHECK(readBytes(emptyOut).empty());

        const fs::path singleIn = tempRoot / "single.txt";
        const fs::path singleHuff = tempRoot / "single.txt.huff";
        const fs::path singleOut = tempRoot / "single_out.txt";
        writeText(singleIn, std::string(400, 'Q'));
        CHECK(compressFile(utf8FromPath(singleIn), utf8FromPath(singleHuff), &err));
        CHECK(decompressFile(utf8FromPath(singleHuff), utf8FromPath(singleOut), &err));
        CHECK(readBytes(singleOut) == readBytes(singleIn));
        std::cout << "[PASS] Empty file and single-byte file round-trip\n";
    }

    // 5. Failed decompression safety: invalid & truncated files leave NO output and preserve existing files
    {
        const fs::path safeDir = tempRoot / "decompression_safety";
        fs::create_directories(safeDir);

        // 5a. Invalid compressed file -> non-existent destination must NOT be created
        const fs::path invalidFile = safeDir / "invalid.huff";
        const fs::path missingDest = safeDir / "should_not_exist.txt";
        writeText(invalidFile, "Not a valid Huffman archive at all!");
        std::string err;
        CHECK(!decompressFile(utf8FromPath(invalidFile), utf8FromPath(missingDest), &err));
        CHECK(!err.empty());
        CHECK(!fs::exists(missingDest));
        CHECK(countFilesInDir(safeDir) == 1);

        // 5b. Invalid compressed file -> pre-existing destination must NOT be truncated or altered
        const fs::path existingDest = safeDir / "important.txt";
        const std::string importantContent = "IMPORTANT_USER_DATA_MUST_STAY_INTACT";
        writeText(existingDest, importantContent);
        err.clear();
        CHECK(!decompressFile(utf8FromPath(invalidFile), utf8FromPath(existingDest), &err));
        CHECK(!err.empty());
        CHECK(readBytes(existingDest) == std::vector<unsigned char>(importantContent.begin(), importantContent.end()));
        CHECK(countFilesInDir(safeDir) == 2);

        // 5c. Truncated valid compressed file -> pre-existing destination preserved & no temp files left
        const fs::path src = safeDir / "source.txt";
        const fs::path fullHuff = safeDir / "source.huff";
        const fs::path truncHuff = safeDir / "truncated.huff";
        writeText(src, "The quick brown fox jumps over the lazy dog repeatedly! 1234567890.");
        CHECK(compressFile(utf8FromPath(src), utf8FromPath(fullHuff), &err));
        std::vector<unsigned char> huffBytes = readBytes(fullHuff);
        CHECK(huffBytes.size() > 12);
        huffBytes.resize(huffBytes.size() - 3);
        writeBytes(truncHuff, huffBytes);

        err.clear();
        CHECK(!decompressFile(utf8FromPath(truncHuff), utf8FromPath(existingDest), &err));
        CHECK(!err.empty());
        CHECK(readBytes(existingDest) == std::vector<unsigned char>(importantContent.begin(), importantContent.end()));
        CHECK(!fs::exists(missingDest));
        CHECK(countFilesInDir(safeDir) == 5);

        // 5d. Valid compressed file replacing an existing destination succeeds cleanly with no temp leftovers
        err.clear();
        CHECK(decompressFile(utf8FromPath(fullHuff), utf8FromPath(existingDest), &err));
        CHECK(readBytes(existingDest) == readBytes(src));
        CHECK(countFilesInDir(safeDir) == 5);
        std::cout << "[PASS] Failed/truncated decompression safety and existing-destination preservation\n";
    }

    // 6. Missing input file
    {
        std::string err;
        CHECK(!compressFile(utf8FromPath(tempRoot / "nonexistent.bin"), utf8FromPath(tempRoot / "out.huff"), &err));
        CHECK(!err.empty());
        CHECK(!decompressFile(utf8FromPath(tempRoot / "nonexistent.huff"), utf8FromPath(tempRoot / "out.txt"), &err));
        CHECK(!err.empty());
        std::cout << "[PASS] Missing input file handling\n";
    }

    fs::remove_all(tempRoot);
    std::cout << "ALL CORE TESTS PASSED\n";
    return 0;
}
