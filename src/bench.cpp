#include "crypto_utils.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>
#include <stdexcept>
#include <iomanip>
#include <cmath>


// simple POSIX helpers for path existence and directory creation
static bool pathExists(const std::string& p) {
    struct stat sb{};
    return ::stat(p.c_str(), &sb) == 0;
}

static void ensureDir(const std::string& dir) {
    if (!pathExists(dir)) {
        if (::mkdir(dir.c_str(), 0755) != 0 && errno != EEXIST) {
            throw std::runtime_error("Failed to create directory: " + dir);
        }
    }
}

// structure to hold benchmark results
struct BenchmarkResult {
    std::string cipher;
    std::string operation; // "encryption" or "decryption"
    std::string filename;
    size_t fileSize;
    double meanTimeMs; // mean time in milliseconds
    double stddevMs;   // standard deviation in milliseconds
    double throughputMBps; // throughput in MB/s (MB = 1e6 bytes)
    int runs; // number of timed runs
};

// function to create test files
void createTestFiles() {
    const std::string dataDir = "data";

    // create data dir if it doesnt exist
    if (!pathExists(dataDir)) {
        ensureDir(dataDir);
        std::cout << "Created data directory." << std::endl;
    }

    // File 1: 16B
    std::string file16B = dataDir + "/file_16B.txt";
    if (!pathExists(file16B)) {
        std::ofstream out(file16B);
        out << "0123456789abcdef"; // 16 bytes
        out.close();
        std::cout << "Created " << file16B << std::endl;
    }

    // create 20KB file (20 * 1024 bytes = 20480 bytes)
    std::string file20KB = dataDir + "/file_20KB.txt";
    if (!pathExists(file20KB)) {
        std::ofstream out(file20KB);
        // Write exactly 20 * 1024 bytes
        const size_t target = 20 * 1024;
        for (size_t i = 0; i < target; ++i) {
            out << static_cast<char>('A' + (i % 26));
        }
        out.close();
        std::cout << "Created " << file20KB << std::endl;
    }

    // create large (>2MB) file. I will create a 2.5MB file (2.5 * 1024 * 1024 = 2621440 bytes)
    std::string file2_5MB = dataDir + "/file_2_5MB.bin";
    if (!pathExists(file2_5MB)) {
        std::ofstream out(file2_5MB, std::ios::binary);
        // Write exactly 2.5 * 1024 * 1024 bytes
        const size_t target = static_cast<size_t>(2.5 * 1024 * 1024);
        for (size_t i = 0; i < target; ++i) {
            unsigned char b = static_cast<unsigned char>('a' + (i % 26));
            out.write(reinterpret_cast<const char*>(&b), 1);
        }
        out.close();
        std::cout << "Created " << file2_5MB << std::endl;
    }
}

// function to read file into a byte vector
std::vector<unsigned char> readFile(const std::string& filename) {
    // open file in binary mode
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    // get file size
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    // read into vector
    std::vector<unsigned char> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    file.close();
    return buffer;
}

// function to perform benchmark
double benchmarkEncryption(
    CipherType cipher,
    const std::vector<unsigned char>& plaintext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& iv
) {
    // Measure only EVP calls using steady_clock
    auto [ciphertext, timeMs] = encrypt_with_timing(cipher, plaintext, key, iv);
    (void)ciphertext; // ciphertext not used here; caller can perform separate decrypt
    return timeMs;
}

// function to benchmark decryption
double benchmarkDecryption(
    CipherType cipher,
    const std::vector<unsigned char>& ciphertext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& iv
) {
    auto [plaintext, timeMs] = decrypt_with_timing(cipher, ciphertext, key, iv);
    (void)plaintext;
    return timeMs;
}

// function to save results to CSV
void saveResultsToCSV(const std::vector<BenchmarkResult>& results) {
    const std::string resultsDir = "results";

    // create results dir if it doesnt exist
    if (!pathExists(resultsDir)) {
        ensureDir(resultsDir);
        std::cout << "Created results directory." << std::endl;
    }

    std::string csvFile = resultsDir + "/benchmark_results.csv";
    std::ofstream out(csvFile);

    if (!out) {
        throw std::runtime_error("Failed to open CSV file for writing: " + csvFile);
    }

    // write header
    out << "Cipher,Operation,Filename,FileSize(Bytes),Runs,MeanTime(ms),StdDev(ms),Throughput(MB/s)\n";

    // write results
    for (const auto& result : results) {
        out << result.cipher << ","
            << result.operation << ","
            << result.filename << ","
            << result.fileSize << ","
            << result.runs << ","
            << std::fixed << std::setprecision(6) << result.meanTimeMs << ","
            << std::fixed << std::setprecision(6) << result.stddevMs << ","
            << std::fixed << std::setprecision(2) << result.throughputMBps << "\n";
    }

    out.close();
    std::cout << "Saved benchmark results to: " << csvFile << std::endl;

}

int main() {
    std::cout << "======================================" << std::endl;
    std::cout << "      OpenSSL Cipher Benchmark      " << std::endl;
    std::cout << "======================================" << std::endl;

    try {
        // step 1: create test files
        std::cout << "Creating test files..." << std::endl;
        createTestFiles();

        // step 2: generate key & IV (generated once, reused for all tests)
        std::cout << "Generating random key and IV..." << std::endl;
        auto key = generateRandomBytes(16); // 128-bit key
        auto iv = generateRandomBytes(16);  // 128-bit IV
        // step 3: define test files
        std::vector<std::string> testFiles = {
            "data/file_16B.txt",
            "data/file_20KB.txt",
            "data/file_2_5MB.bin"
        };

        // step 4: define ciphers to test
        std::vector<CipherType> ciphers = {
            CipherType::AES,
            CipherType::CAMELLIA,
            CipherType::SM4
        };
        // step 5: perform benchmarks
        std::vector<BenchmarkResult> results;

        // repeat configuration
        const int warmupIters = 1; // one warm-up per (cipher, file)
        const int timedIters = 5; // number of timed runs

        for (const auto& cipher : ciphers) {
            std::string cipherName = cipherTypeToString(cipher);
            std::cout << "\n--- Testing " << cipherName << " ---" << std::endl;

            for (const auto& filename : testFiles) {
                std::cout << "\nFile: " << filename << std::endl;

                // read file
                auto plaintext = readFile(filename);
                size_t fileSize = plaintext.size();
                std::cout << "File size: " << fileSize << " bytes" << std::endl;

                // warm-up encryption/decryption (not timed)
                {
                    auto [ct_warm, _] = encrypt_with_timing(cipher, plaintext, key, iv);
                    auto [pt_warm, __] = decrypt_with_timing(cipher, ct_warm, key, iv);
                    if (pt_warm != plaintext) {
                        throw std::runtime_error("Warm-up decrypt mismatch: plaintext mismatch for " + filename + " with cipher " + cipherName);
                    }
                }

                // timed runs: encryption
                std::vector<double> encTimes;
                encTimes.reserve(timedIters);
                std::vector<unsigned char> ciphertext_last;
                for (int i = 0; i < timedIters; ++i) {
                    auto [ct, t] = encrypt_with_timing(cipher, plaintext, key, iv);
                    encTimes.push_back(t);
                    ciphertext_last = std::move(ct);
                }
                // compute mean and stddev for encryption
                double encSum = 0.0; for (double v : encTimes) encSum += v;
                double encMean = encSum / encTimes.size();
                double encVar = 0.0; for (double v : encTimes) encVar += (v - encMean) * (v - encMean); encVar /= encTimes.size();
                double encStd = std::sqrt(encVar);
                double encThroughputMBs = (fileSize / 1.0e6) / (encMean / 1000.0); // MB/s using MB=1e6 bytes
                std::cout << "Encrypt: mean=" << std::fixed << std::setprecision(6) << encMean << " ms, stddev=" << encStd
                          << " ms, throughput=" << std::setprecision(2) << encThroughputMBs << " MB/s" << std::endl;
                results.push_back({cipherName, "encrypt", filename, fileSize, encMean, encStd, encThroughputMBs, timedIters});

                // timed runs: decryption on last ciphertext
                std::vector<double> decTimes;
                decTimes.reserve(timedIters);
                std::vector<unsigned char> plaintext_last;
                for (int i = 0; i < timedIters; ++i) {
                    auto [pt, t] = decrypt_with_timing(cipher, ciphertext_last, key, iv);
                    decTimes.push_back(t);
                    plaintext_last = std::move(pt);
                }
                if (plaintext_last != plaintext) {
                    throw std::runtime_error("Decryption mismatch: recovered plaintext differs for " + filename + " with cipher " + cipherName);
                }
                double decSum = 0.0; for (double v : decTimes) decSum += v;
                double decMean = decSum / decTimes.size();
                double decVar = 0.0; for (double v : decTimes) decVar += (v - decMean) * (v - decMean); decVar /= decTimes.size();
                double decStd = std::sqrt(decVar);
                double decThroughputMBs = (fileSize / 1.0e6) / (decMean / 1000.0);
                std::cout << "Decrypt: mean=" << std::fixed << std::setprecision(6) << decMean << " ms, stddev=" << decStd
                          << " ms, throughput=" << std::setprecision(2) << decThroughputMBs << " MB/s" << std::endl;
                results.push_back({cipherName, "decrypt", filename, fileSize, decMean, decStd, decThroughputMBs, timedIters});
            }
        }
        // step 6: save results to CSV
        saveResultsToCSV(results);

        std::cout << "\n======================================" << std::endl;
        std::cout << "        Benchmark Completed         " << std::endl;
        std::cout << "======================================" << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}