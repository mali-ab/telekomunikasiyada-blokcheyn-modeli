#include "signature.h"

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
        if (is_prime[p]) {
            primes.push_back(p);
        }
    }
    return primes;
}

long long DigitalSignature::selectRandomPrime(const vector<long long>& primes) {
    if (primes.empty()) return 0;
    int randomIndex = rand() % primes.size();
    return primes[randomIndex];
}

RSAKeys DigitalSignature::generateKeys() {
    srand(time(0));

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
        if ((i * e) % phi == 1) {
            d = i;
            break;
        }
    }
    return {n, e, d};
}

// RSA Şifrlemek: C = M^e mod N
vector<long long> DigitalSignature::encrypt(string data, long long e, long long n) {
    vector<long long> cipher;
    for (char c : data) {
        cipher.push_back(power(c, e, n));
    }
    return cipher;
}

// RSA Şifrini açmak: M = C^d mod N
string DigitalSignature::decrypt(vector<long long> cipher, long long d, long long n) {
    string msg = "";
    for (long long c : cipher) {
        msg += (char)power(c, d, n);
    }
    return msg;
}