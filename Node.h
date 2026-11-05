#pragma once

class Node {
public:
    unsigned char character;
    int freq;
    Node * left;
    Node * right;
       
    Node(unsigned char c = 0, int f = 0);
	  //unsigned_char prevents negative byte values
};
