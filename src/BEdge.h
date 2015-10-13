#ifndef BEDGE_H
#define BEDGE_H

#include "Utilities.h"
#include "FiniteField.h"

//Forward declaration needed due to circular dependency
struct VariableNode;
struct CheckNode;

class BEdge{
public:
    // variable node
    VariableNode* vnode;
    // check node
    CheckNode *cnode;
    int seqInCheck;
    // g is a vector of size batchSize
    // it is the row of the generator matrix of cnode corresponding vnode;
    SymbolType* g;
    // gh is a vector of the coefficients of the received packets
    // the size of the vector is equal to numRec in BatsDecoder
    // a component of gh is given by inner product of g and coding vector of a packet
    SymbolType* gh;
     
    BEdge *nextInVar;
    
    BEdge(CheckNode * c, VariableNode * v, int size): cnode(c), vnode(v){
        g = new SymbolType[size];
        gh = new SymbolType[size];
        nextInVar = NULL;
    }
    ~BEdge(){
        delete[] g;
        delete[] gh;
    }
    
    // set gh for the index idx
    inline void addCoef(int idx, int size, SymbolType* cv){
        gh[idx] = FF.innerprod(g,cv,size);
    }
};

#endif /* BEDGE_H */

