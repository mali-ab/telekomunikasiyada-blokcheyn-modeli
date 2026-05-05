#ifndef SIGNATURE_H
#define SIGNATURE_H

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <fstream>

struct RSAKeys {
    long long n; // modulus
    long long e; // public exponent
    long long d; // private exponent
};

class DigitalSignature {
public:
    static long long gcd(long long a, long long b);
    static long long power(long long base, long long exp, long long mod);
    static std::vector<long long> getPrimesList(long long max_n);
    static long long selectRandomPrime(const std::vector<long long> &primes);

    static RSAKeys generateKeys();
    static std::vector<long long> encrypt(const std::string &data, long long d, long long n);
    static std::string decrypt(const std::vector<long long> &cipher, long long e, long long n);

    static void saveKeys(const RSAKeys &keys, const std::string &filename);
    static bool loadKeys(RSAKeys &keys, const std::string &filename);
    static RSAKeys getKeys(const std::string &filename = "keys.txt");
};

#endif