#include "signature.h"
#include <random>

using namespace std;

long long DigitalSignature::gcd(long long a, long long b) {
    while (b != 0) {
        a %= b;
        swap(a, b);
    }
    return a;
}

long long DigitalSignature::power(long long base, long long exp, long long mod) {
    long long res = 1;
    base %= mod;
    while (exp > 0) {
        if (exp % 2 == 1) res = (__int128)res * base % mod;
        base = (__int128)base * base % mod;
        exp /= 2;
    }
    return res;
}

// --- Sieve of Eratosthenes ---
vector<long long> DigitalSignature::getPrimesList(long long max_n) {
    vector<bool> is_prime(max_n + 1, true);
    is_prime[0] = is_prime[1] = false;
    for (long long p = 2; p * p <= max_n; p++) {
        if (is_prime[p]) {
            for (long long i = p * p; i <= max_n; i += p)
                is_prime[i] = false;
        }
    }
    vector<long long> primes;
    for (long long p = 2; p <= max_n; p++) {
        if (is_prime[p]) primes.push_back(p);
    }
    return primes;
}

long long DigitalSignature::selectRandomPrime(const vector<long long>& primes) {
    if (primes.empty()) return 0;
    static mt19937 rng(random_device{}());
    uniform_int_distribution<int> dist(0, primes.size() - 1);
    return primes[dist(rng)];
}

RSAKeys DigitalSignature::generateKeys() {
    vector<long long> primes = getPrimesList(500);

    long long p = 0, q = 0;
    while (p < 50) p = selectRandomPrime(primes);
    while (q < 50 || q == p) q = selectRandomPrime(primes);

    long long n = p * q;
    long long phi = (p - 1) * (q - 1);

    long long e = 3;
    while (gcd(e, phi) != 1) e += 2;

    long long d = 0;
    for (long long i = 1; i < phi; i++) {
        if ((i * e) % phi == 1) { d = i; break; }
    }
    return {n, e, d};
}

vector<long long> DigitalSignature::encrypt(const string &data, long long d, long long n) {
    vector<long long> cipher;
    cipher.reserve(data.size());
    for (char c : data) {
        cipher.push_back(power(c, d, n));
    }
    return cipher;
}

string DigitalSignature::decrypt(const vector<long long> &cipher, long long e, long long n) {
    string msg;
    msg.reserve(cipher.size());
    for (long long c : cipher) {
        msg += static_cast<char>(power(c, e, n));
    }
    return msg;
}

void DigitalSignature::saveKeys(const RSAKeys& keys, const string& filename) {
    ofstream file(filename);
    if (file.is_open()) {
        file << keys.e << " " << keys.d << " " << keys.n;
        file.close();
    }
}

bool DigitalSignature::loadKeys(RSAKeys& keys, const string& filename) {
    ifstream file(filename);
    if (file.is_open()) {
        if (file >> keys.e >> keys.d >> keys.n) {
            file.close();
            return true;
        }
        file.close();
    }
    return false;
}

RSAKeys DigitalSignature::getKeys(const string& filename) {
    RSAKeys currentKeys;
    if (loadKeys(currentKeys, filename)) {
        cout << "[INFO]: Keys loaded from file: " << filename << endl;
        return currentKeys;
    } else {
        cout << "[WARNING]: File not found. Generating new RSA keys..." << endl;
        RSAKeys newKeys = generateKeys();
        saveKeys(newKeys, filename);
        cout << "[SUCCESS]: New keys generated and saved: " << filename << endl;
        return newKeys;
    }
}