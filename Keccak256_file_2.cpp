#include <iostream>
#include <fstream>
#include <cassert>
#include <cstring>
#include <iomanip>

using namespace std;

const int HASH_LEN = 32;
const int BLOCK_SIZE = 200 - HASH_LEN * 2;
const int NUM_ROUNDS = 24;

const unsigned char ROTATION[5][5] = {
    {0, 36, 3, 41, 18},
    {1, 44, 10, 45, 2},
    {62, 6, 43, 15, 61},
    {28, 55, 25, 21, 56},
    {27, 20, 39, 8, 14},
};

void getHash(ifstream &file, uint8_t hashResult[HASH_LEN], ofstream &log);
void absorb(uint64_t state[5][5], ofstream &log);
uint64_t rotl64(uint64_t x, int i);
void printState(const uint64_t state[5][5], const string &stepName, int round, ofstream &log);
void printBlock(const uint8_t *block, size_t size, const string &desc, ofstream &log);

void getHash(ifstream &file, uint8_t hashResult[HASH_LEN], ofstream &log) {
    assert(file.is_open() && hashResult != nullptr);
    uint64_t state[5][5] = {};
    char buffer[BLOCK_SIZE] = {};
    size_t bytesRead = 0;

    while (file.read(buffer, BLOCK_SIZE) || (bytesRead = file.gcount()) > 0) {
        log << "Processing Block of Size: " << bytesRead << " bytes\n";
        printBlock(reinterpret_cast<uint8_t *>(buffer), bytesRead, "Block Data", log);

        int blockOff = 0;
        for (size_t i = 0; i < bytesRead; i++) {
            int j = blockOff >> 3;
            state[j % 5][j / 5] ^= static_cast<uint64_t>(buffer[i]) << ((blockOff & 7) << 3);
            blockOff++;
        }

        log << "State after XOR with block:\n";
        printState(state, "State After XOR", -1, log);

        if (blockOff == BLOCK_SIZE) {
            absorb(state, log);
        }
    }

    size_t blockOff = bytesRead;
    int i = blockOff >> 3;
    state[i % 5][i / 5] ^= UINT64_C(0x01) << ((blockOff & 7) << 3);
    blockOff = BLOCK_SIZE - 1;
    int j = blockOff >> 3;
    state[j % 5][j / 5] ^= UINT64_C(0x80) << ((blockOff & 7) << 3);
    log << "Final Block after Padding:\n";
    printState(state, "State Before Final Absorb", -1, log);
    absorb(state, log);

    log << "Extracting hash from final state:\n";
    printState(state, "Final State", -1, log);
    for (int i = 0; i < HASH_LEN; i++) {
        int j = i >> 3;
        hashResult[i] = static_cast<uint8_t>(state[j % 5][j / 5] >> ((i & 7) << 3));
    }
}

void absorb(uint64_t state[5][5], ofstream &log) {
    uint64_t(*a)[5] = state;
    uint8_t r = 1;
    log << "--- Applying Keccak-f ---\n";
    for (int i = 0; i < NUM_ROUNDS; i++) {
        log << "\n--- Round " << i + 1 << " ---\n";
        printState(a, "Initial State", i + 1, log);

        uint64_t c[5] = {};
        for (int x = 0; x < 5; x++) {
            for (int y = 0; y < 5; y++)
                c[x] ^= a[x][y];
        }
        for (int x = 0; x < 5; x++) {
            uint64_t d = c[(x + 4) % 5] ^ rotl64(c[(x + 1) % 5], 1);
            for (int y = 0; y < 5; y++)
                a[x][y] ^= d;
        }
        printState(a, "After Theta", i + 1, log);

        uint64_t b[5][5];
        for (int x = 0; x < 5; x++) {
            for (int y = 0; y < 5; y++)
                b[y][(x * 2 + y * 3) % 5] = rotl64(a[x][y], ROTATION[x][y]);
        }
        printState(b, "After Rho and Pi", i + 1, log);

        for (int x = 0; x < 5; x++) {
            for (int y = 0; y < 5; y++)
                a[x][y] = b[x][y] ^ (~b[(x + 1) % 5][y] & b[(x + 2) % 5][y]);
        }
        printState(a, "After Chi", i + 1, log);

        for (int j = 0; j < 7; j++) {
            a[0][0] ^= static_cast<uint64_t>(r & 1) << ((1 << j) - 1);
            r = static_cast<uint8_t>((r << 1) ^ ((r >> 7) * 0x171));
        }
        printState(a, "After Iota", i + 1, log);
    }
}

uint64_t rotl64(uint64_t x, int i) {
    return ((0U + x) << i) | (x >> ((64 - i) & 63));
}

void printState(const uint64_t state[5][5], const string &stepName, int round, ofstream &log) {
    log << stepName << (round >= 0 ? " (Round " + to_string(round) + ")" : "") << ":\n";
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            log << hex << setw(16) << setfill('0') << state[x][y] << " ";
        }
        log << "\n";
    }
    log << dec;
}

void printBlock(const uint8_t *block, size_t size, const string &desc, ofstream &log) {
    log << desc << ": ";
    for (size_t i = 0; i < size; i++) {
        log << hex << setw(2) << setfill('0') << static_cast<int>(block[i]) << " ";
    }
    log << "\n";
}

int main() {
    const char *filePath = "D:\\web\\ReactJs\\Flux.txt";
    const char *logPath = "D:\\web\\Keccak256 Hash Generator\\Keccak256_file_process.txt";

    ifstream file(filePath, ios::binary);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << filePath << endl;
        return 1;
    }

    ofstream log(logPath);
    if (!log.is_open()) {
        cerr << "Failed to open log file: " << logPath << endl;
        return 1;
    }

    uint8_t hashResult[HASH_LEN];
    getHash(file, hashResult, log);

    log << "Keccak-256 Hash: ";
    for (int i = 0; i < HASH_LEN; i++) {
        log << hex << setw(2) << setfill('0') << static_cast<int>(hashResult[i]);
    }
    log << "\n";

    file.close();
    log.close();

    cout << "Keccak-256 hashing process has been logged to " << logPath << endl;
    return 0;
}
