#include "model/Encryption/DataStandardEncryption.h"
#include <model/helperFunctions.h>

#include <sstream>
#include <vector>
#include <algorithm>
#include <iostream>

using std::stringstream;
using std::fill_n;
using std::reverse;
using std::cout;

string DataStandardEncryption::encrypt(string message, Key &key) {
    stringstream is, os;
    is << message;

    vector<bitset<48>> roundKey = initializeRoundKey(key);
    ElectronicCodeBook(is, os, roundKey);

    return os.str();
}

string DataStandardEncryption::decrypt(string message, Key &key) {
    stringstream is, os;
    is << message;

    vector<bitset<48>> roundKey = initializeRoundKey(key);
    reverse(roundKey.begin(), roundKey.end());
    ElectronicCodeBook(is, os, roundKey);

    string outString = os.str();

    outString = helperFunctions::binaryStringToString(outString);
    outString.erase(std::find(outString.begin(), outString.end(), '\0'), outString.end());

    return outString;
}

void DataStandardEncryption::encrypt(string fileIn, string fileOut, Key &key) {

}

void DataStandardEncryption::decrypt(string fileIn, string fileOut, Key &key) {

}

vector<bitset<48>> DataStandardEncryption::initializeRoundKey(Key &key) {
    bitset<64> bitKey(helperFunctions::stringToBinaryString(key.getKey()));

    bitset<56> noParityKey = permutation(bitKey, keyParityDrop);

    bitset<28> leftKeyPart = getKeyPart(noParityKey, 0);
    bitset<28> rightKeyPart = getKeyPart(noParityKey, 28);

    vector<bitset<48>> roundKey;
    for(size_t i=0; i<16; i++) {
        shiftLeft(leftKeyPart, shiftsTable[i]);
        shiftLeft(rightKeyPart, shiftsTable[i]);

        bitset<56> combKey = combineKey(leftKeyPart, rightKeyPart);
        bitset<48> pCombinedKey = permutation(combKey, keyCompression);

        roundKey.push_back(pCombinedKey);
    }

    return roundKey;
}

bitset<56> DataStandardEncryption::permutation(const bitset<64> &oldSet, const byte* array) {
    bitset<56> newSet;
    for(size_t i=0; i<newSet.size(); i++) {
        newSet[i] = oldSet[array[i]-1];
    }
    return newSet;
}

bitset<48> DataStandardEncryption::permutation(const bitset<56> &oldSet, const byte* array) {
    bitset<48> newSet;
    for(size_t i=0; i<newSet.size(); i++) {
        newSet[i] = oldSet[array[i]-1];
    }
    return newSet;
}

bitset<48> DataStandardEncryption::permutation(const bitset<32> &oldSet, const byte* array) {
    bitset<48> newSet;
    for(size_t i=0; i<newSet.size(); i++) {
        newSet[i] = oldSet[array[i]-1];
    }
    return newSet;
}

bitset<64> DataStandardEncryption::permutation64s(const bitset<64> &oldSet, const byte *array) {
    bitset<64> newSet;
    for(size_t i=0; i<newSet.size(); i++) {
        newSet[i] = oldSet[array[i]-1];
    }
    return newSet;
}

bitset<32> DataStandardEncryption::permutation32s(const bitset<32> &oldSet, const byte* array) {
    bitset<32> newSet;
    for(size_t i=0; i<newSet.size(); i++) {
        newSet[i] = oldSet[array[i]-1];
    }
    return newSet;
}

bitset<28> DataStandardEncryption::getKeyPart(const bitset<56> &oldSet, const size_t &from) {
    bitset<28> nSet;
    for(size_t i=0; i<nSet.size(); i++) {
        nSet[i] = oldSet[from+i];
    }
    return nSet;
}

bitset<32> DataStandardEncryption::getKeyPart(const bitset<64> &oldSet, const size_t &from) {
    bitset<32> nSet;
    for(size_t i=0; i<nSet.size(); i++) {
        nSet[i] = oldSet[from+i];
    }
    return nSet;
}

void DataStandardEncryption::shiftLeft(bitset<28> &bset, const byte shiftBy) {
    string shifted;
    for(size_t i=0; i<shiftBy; i++) {
        shifted += bset[i];
    }
    bset >>= shiftBy;
    for(byte i=0; i<shiftBy; i++) {
        bset[bset.size()-shiftBy+i] = shifted[i];
    }
}

bitset<56> DataStandardEncryption::combineKey(const bitset<28> &bs1, const bitset<28> &bs2) {
    bitset<56> nBitset;
    for(size_t i=0; i<bs1.size(); i++) {
        nBitset[i] = bs1[i];
        nBitset[bs1.size()+i] = bs2[i];
    }
    return nBitset;
}

bitset<64> DataStandardEncryption::combineKey(const bitset<32> &bs1, const bitset<32> &bs2) {
    bitset<64> nBitset;
    for(size_t i=0; i<bs1.size(); i++) {
        nBitset[i] = bs1[i];
        nBitset[bs1.size()+i] = bs2[i];
    }
    return nBitset;
}

void DataStandardEncryption::ElectronicCodeBook(istream &is, ostream &os, vector<bitset<48>> roundKey) {
    //One extra free space for null byte due to string conversion
    char plainText[9];
    while(is.get(&plainText[0], 9)) {
        string fixedPlainText(plainText);

        for(size_t i = fixedPlainText.size(); i<8; i++) {
            fixedPlainText += '\0';
        }

        string text_cypherText = blockPartial(fixedPlainText, roundKey);

        if(!(os << text_cypherText)) {
            std::cerr << "Blad zapisu do sstring" << "\n";
        }

        fill_n(plainText, 9, 0);
    }
}

string DataStandardEncryption::blockPartial(string plainText, const vector<bitset<48>> &roundKey) {
    bitset<64> bitText(helperFunctions::stringToBinaryString(plainText));
    bitText = permutation64s(bitText, initialPermutation);

    bitset<32> leftBitText = getKeyPart(bitText, 0);
    bitset<32> rightBitText = getKeyPart(bitText, 32);

    for(byte i=0; i<16; i++) {
        bitset<48> extRightBitText = permutation(rightBitText, expansionFunction);
        bitset<48> xerb = xorBitset(extRightBitText, roundKey[i]);

        bitset<32> sBoxOp;
        for(byte j=0; j<8; j++) {
            int j6 = 6*j;
            int r = 2 * xerb[j6] + xerb[j6+5];
            int c = 8*xerb[j6+1] + 4*xerb[j6+2] + 2*xerb[j6+3] + xerb[j6+4];
            byte val = sBox[j][r][c];

            for(byte k=8, l=0; k>=1; k/=2, l++) {
                sBoxOp[j*4+l] = val/k;
                val %= k;
            }
        }

        sBoxOp = permutation32s(sBoxOp, P);

        leftBitText = xorBitset(sBoxOp, leftBitText);

        swap(leftBitText, rightBitText);
    }

    bitset<64> combine = combineKey(rightBitText, leftBitText);
    combine = permutation64s(combine, finalPermutation);

    return helperFunctions::reverseString(combine.to_string());
}

bitset<48> DataStandardEncryption::xorBitset(const bitset<48> &bs1, const bitset<48> &bs2) {
    bitset<48> nbitset = bs1 ^ bs2;
    return nbitset;
}

bitset<32> DataStandardEncryption::xorBitset(bitset<32> &bs1, bitset<32> &bs2) {
    bitset<32> nbitset = bs1 ^ bs2;
    return nbitset;
}