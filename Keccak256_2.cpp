#include <iostream>
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

static void getHash(const std::uint8_t msg[], std::size_t len, std::uint8_t hashResult[HASH_LEN]);
static void absorb(std::uint64_t state[5][5], const uint8_t *data, size_t dataSize);
static void keccakF(uint64_t state[5][5]);
static std::uint64_t rotl64(std::uint64_t x, int i);
void printState(const std::uint64_t state[5][5], const string &stepName, int round = -1);
void printBlock(const uint8_t *block, size_t size, const string &desc);

void getHash(const uint8_t msg[], size_t len, uint8_t hashResult[HASH_LEN])
{
    assert((msg != nullptr || len == 0) && hashResult != nullptr);
    uint64_t state[5][5] = {};

    // Add padding and prepare data
    size_t paddedLen = ((len + 1 + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE; // Padding length
    uint8_t *paddedMsg = new uint8_t[paddedLen]();
    memcpy(paddedMsg, msg, len);
    paddedMsg[len] = 0x01; // Padding start
    paddedMsg[paddedLen - 1] |= 0x80; // Padding end

    cout << "Message after padding:" << endl;
    printBlock(paddedMsg, paddedLen, "Padded Message");

    // Divide into blocks and absorb
    size_t numBlocks = paddedLen / BLOCK_SIZE;
    for (size_t i = 0; i < numBlocks; i++) {
        cout << "\nProcessing Block " << i + 1 << " of " << numBlocks << ":" << endl;
        printBlock(&paddedMsg[i * BLOCK_SIZE], BLOCK_SIZE, "Block Pi");
        absorb(state, &paddedMsg[i * BLOCK_SIZE], BLOCK_SIZE);
    }

    delete[] paddedMsg;

    // Extract the hash
    cout << "Extracting hash from final state:" << endl;
    printState(state, "Final State");
    for (int i = 0; i < HASH_LEN; i++) {
        int j = i >> 3;
        hashResult[i] = static_cast<uint8_t>(state[j % 5][j / 5] >> ((i & 7) << 3));
    }
}

void absorb(uint64_t state[5][5], const uint8_t *data, size_t dataSize)
{
    // XOR block data with state
    for (size_t i = 0; i < dataSize; i++) {
        int j = i >> 3;
        state[j % 5][j / 5] ^= static_cast<uint64_t>(data[i]) << ((i & 7) << 3);
    }

    cout << "State after XOR with block:" << endl;
    printState(state, "State After XOR");

    // Apply the Keccak-f permutation
    keccakF(state);
}

void keccakF(uint64_t state[5][5])
{
    uint64_t(*a)[5] = state;
    uint8_t r = 1; // LFSR
    cout << "--- Applying Keccak-f ---" << endl;
    for (int i = 0; i < NUM_ROUNDS; i++) {
        cout << "\n--- Round " << i + 1 << " ---" << endl;
        printState(a, "Initial State", i + 1);

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
        printState(a, "After Theta", i + 1);

        uint64_t b[5][5];
        for (int x = 0; x < 5; x++) {
            for (int y = 0; y < 5; y++)
                b[y][(x * 2 + y * 3) % 5] = rotl64(a[x][y], ROTATION[x][y]);
        }
        printState(b, "After Rho and Pi", i + 1);

        for (int x = 0; x < 5; x++) {
            for (int y = 0; y < 5; y++)
                a[x][y] = b[x][y] ^ (~b[(x + 1) % 5][y] & b[(x + 2) % 5][y]);
        }
        printState(a, "After Chi", i + 1);

        for (int j = 0; j < 7; j++) {
            a[0][0] ^= static_cast<uint64_t>(r & 1) << ((1 << j) - 1);
            r = static_cast<uint8_t>((r << 1) ^ ((r >> 7) * 0x171));
        }
        printState(a, "After Iota", i + 1);
    }
}

uint64_t rotl64(uint64_t x, int i)
{
    return ((0U + x) << i) | (x >> ((64 - i) & 63));
}

void printState(const std::uint64_t state[5][5], const string &stepName, int round)
{
    cout << stepName << (round >= 0 ? " (Round " + to_string(round) + ")" : "") << ":\n";
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            cout << hex << setw(16) << setfill('0') << state[x][y] << " ";
        }
        cout << endl;
    }
    cout << dec; // Reset formatting
}

void printBlock(const uint8_t *block, size_t size, const string &desc)
{
    cout << desc << ": ";
    for (size_t i = 0; i < size; i++) {
        cout << hex << setw(2) << setfill('0') << static_cast<int>(block[i]) << " ";
    }
    cout << endl;
}

int main()
{
    const char *message = "Hello, World!";
    std::uint8_t hashResult[HASH_LEN];

    getHash(reinterpret_cast<const std::uint8_t *>(message), strlen(message), hashResult);

    std::cout << "Keccak-256 Hash: ";
    for (int i = 0; i < HASH_LEN; i++) {
        std::cout << hex << setw(2) << setfill('0') << static_cast<int>(hashResult[i]);
    }
    std::cout << std::endl;

    return 0;
}
