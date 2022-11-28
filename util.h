//
//  util.h
//  Program 6: File Compression
//  Course: CS 251, Fall 2022. Thursday 12pm lab
//  System: VS Code on Microsoft Windows
//
// Author: Alexander Bernatowicz
//
// The objective of this program is to use a priority queue, binary search tree, and a map to compress a file to a smaller size.
// To complete the file compression we use the Huffman Algorithim to convert all characters into binary.

#pragma once

#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <vector>

#include "bitstream.h"
#include "hashmap.h"

typedef hashmap hashmapF;
typedef unordered_map <int, string> hashmapE;

struct HuffmanNode {
    int character;
    int count;
    HuffmanNode* zero;
    HuffmanNode* one;
};

struct compare
{
    bool operator()(const HuffmanNode *lhs,
        const HuffmanNode *rhs)
    {
        return lhs->count > rhs->count;
    }
};

//
// *This method frees the memory allocated for the Huffman tree.
//
void freeTree(HuffmanNode* node) {
    if(node == nullptr) { // base case to check if tree is empty
        return;
    }
    freeTree(node->zero); // recursive calls to delete all nodes
    freeTree(node->one);
    delete node;
}

//
// *This function build the frequency map.  If isFile is true, then it reads
// from filename.  If isFile is false, then it reads from a string filename.
//
void buildFrequencyMap(string filename, bool isFile, hashmapF &map) {
    if(isFile) { // if valid file
        ifstream inFS(filename);
        char c;
        // while loop that get all characters in the file and adds into the map
        while (inFS.get(c)) {
            if(map.containsKey(c)) {
                map.put(int(c), map.get(c) + 1);
            }
            else {
                map.put(int(c), 1);
            }
        }
    }
    else { // if not a valid file then treats filename as a string and parses through the string
        for(char c : filename) {
            if(map.containsKey(c)) {
                map.put(c, map.get(c) + 1);
            }
            else {
                map.put(c, 1);
            }
        }
    }
    map.put(256, 1); // adds EOF character to the the map
}

//
// *This function builds an encoding tree from the frequency map.
//
HuffmanNode* buildEncodingTree(hashmapF &map) {
    priority_queue<HuffmanNode*, vector<HuffmanNode*>, compare> pq;
    
    for(int key : map.keys()) { // loops through the map and creates a new node for the tree setting everything to defualt values
        HuffmanNode* newNode = new HuffmanNode;
        newNode->character = key;
        newNode->count = map.get(key);
        newNode->zero = nullptr;
        newNode->one = nullptr;
        pq.push(newNode);
    }

    while(pq.size() > 1) { // while loop that goes through the priority queue to get the root node
        HuffmanNode* firstPop = pq.top();
        pq.pop();
        HuffmanNode* secondPop = pq.top();
        pq.pop();
        HuffmanNode* newNode = new HuffmanNode;
        newNode->character = 257;
        newNode->count = firstPop->count + secondPop->count;
        newNode->zero = firstPop;
        newNode->one = secondPop;
        pq.push(newNode);
    }
    return pq.top(); // returns the root of the tree
}

//
// *Recursive helper function for building the encoding map.
//
void _buildEncodingMap(HuffmanNode* node, hashmapE &encodingMap, string str, HuffmanNode* prev) {
    if(node == nullptr) { // checks to see if tree is empty
        return;
    }
    prev = node; 
    if(node->character != 257) { // if current node is not a character add pair to encoding map
        encodingMap.insert({int(node->character), str});
        return;
    }
    _buildEncodingMap(node->zero, encodingMap, str += "0", prev); // recursive calls to go left
    str.pop_back(); // adds to string
    _buildEncodingMap(node->one, encodingMap, str += "1", prev); // recursive call to go right
}

//
// *This function builds the encoding map from an encoding tree.
//
hashmapE buildEncodingMap(HuffmanNode* tree) {
    // creates encoding map, node that is root of tree, and an empty string
    hashmapE encodingMap;
    HuffmanNode* node = tree;
    string str = "";

    _buildEncodingMap(node, encodingMap, str, node); // calls recursive helper function
    return encodingMap; // returns the encoding map
}

//
// *This function encodes the data in the input stream into the output stream
// using the encodingMap.  This function calculates the number of bits
// written to the output stream and sets result to the size parameter, which is
// passed by reference.  This function also returns a string representation of
// the output file, which is particularly useful for testing.
//
string encode(ifstream& input, hashmapE &encodingMap, ofbitstream& output, int &size, bool makeFile) {
    string result = "";
    char c;
    while(input.get(c)) { // while loops that gets each character from input and adds to result string, also updates size
        result += encodingMap[int(c)];
        size += encodingMap[int(c)].size();
    }
    result += encodingMap[256]; // adds last character and updates size
    size += encodingMap[256].size();

    if(makeFile) { // checks if valid file updates input to output
        for(char c : result) {
            output.writeBit(c == '0' ? 0 : 1);
        }
    }

    return result;  // returns encoded result string
}


//
// *This function decodes the input stream and writes the result to the output
// stream using the encodingTree.  This function also returns a string
// representation of the output file, which is particularly useful for testing.
//
string decode(ifbitstream &input, HuffmanNode* encodingTree, ofstream &output) {
    string result = "";
    HuffmanNode* node = encodingTree;

    while(!input.eof()) { // while loop that runs unitl end of file
        int bit = input.readBit();
        if(bit == 0) {  // if else statements that to 0 or 1 in tree based off of current bit in input
            node = node->zero;
        }
        else {
            node = node->one;
        }
        if(node->character != 257) { // if node is not EOF
            if(node->character == 256) { // if node not a character break out of loop
                break;
            }
            output.put(char(node->character)); // otherwise add character to output
            result += node->character; // add character to result string
            node = encodingTree; // reset node to root
        }
    }
    return result;  // returns decoded result string
}

//
// *This function completes the entire compression process.  Given a file,
// filename, this function (1) builds a frequency map; (2) builds an encoding
// tree; (3) builds an encoding map; (4) encodes the file (don't forget to
// include the frequency map in the header of the output file).  This function
// should create a compressed file named (filename + ".huf") and should also
// return a string version of the bit pattern.
//
string compress(string filename) {
    // creates empty string, size set to zero, and a hashmap
    string result = "";
    int size = 0;
    hashmapF fM;

    buildFrequencyMap(filename, true, fM); // calls the build frequency map function
    HuffmanNode* eT = buildEncodingTree(fM); // creates encoding tree and calling encoding tree function
    hashmapE eM = buildEncodingMap(eT); // creates encoding map and calling encoding map function

    ofbitstream output(filename + ".huf"); // creates outfput file
    output << fM;

    ifstream input(filename);
    
    result = encode(input, eM, output, size, true); // sets result string to the encode function
    // closes input and output and frees the tree
    input.close();
    output.close();
    freeTree(eT);

    return result;  // returns the result string
}

//
// *This function completes the entire decompression process.  Given the file,
// filename (which should end with ".huf"), (1) extract the header and build
// the frequency map; (2) build an encoding tree from the frequency map; (3)
// using the encoding tree to decode the file.  This function should create a
// compressed file using the following convention.
// If filename = "example.txt.huf", then the uncompressed file should be named
// "example_unc.txt".  The function should return a string version of the
// uncompressed file.  Note: this function should reverse what the compress
// function did.
//
string decompress(string filename) {
    string result = ""; // creates the empty result string

    hashmap fM;
    ifbitstream input(filename); // creates input
    ofstream output(filename.substr(0, filename.size() - 8) + "_unc.txt"); // creates output file
    
    input >> fM; // takes the input which is the encoded text
    HuffmanNode* eT = buildEncodingTree(fM); // builds a tree

    result = decode(input, eT, output); // sets result string to decoded text
    // closes input and output and frees the tree
    input.close();
    output.close();
    freeTree(eT);

    return result;  // returns the result string
}
