#ifndef SIGNATURE_H
#define SIGNATURE_H

#include <iostream>
#include <string>
#include <vector>
#include <cmath>

using namespace std;

struct RSAKeys {
    long long n; // modul
    long long e; // Public
    long long d; // Private
};

class DigitalSignature {
public:
    static long long gcd(long long a, long long b);
    static long long power(long long base, long long exp, long long mod);
    static vector<long long> getPrimesList(long long max_n);
    static long long selectRandomPrime(const vector<long long>& primes);
    
    static RSAKeys generateKeys();
    static vector<long long> encrypt(string data, long long e, long long n);
    static string decrypt(vector<long long> cipher, long long d, long long n);
};

#endif