/*
 * FiniteField.h
 *
 *  Created on: 17 Nov 2010
 *      Author: shenghao
 */

#ifndef FINITEFIELD_H_
#define FINITEFIELD_H_

#include <fstream>
#include <iostream>
//#include <vector>
#include "Utilities.h"
#include "time.h"
using namespace std;

//template<typename int>

#include "ff.h"

class FiniteField {
    //	typedef unsigned short int;
public:
    int order;
    int size;
private:
    int createLogTables(int);
    int createMulintables(int);

    static const int prim_poly[9];
    static const int nw[9];
    static const int nwm1[9];

    int *divTable;
    int *mulTable;
    int *logTable;
    int *expTable;

    int *mulTable2;

    void init();
    void finalize();
    
public:
    FiniteField(int a) {
        order = a;
        size = 1 << a;
        init();
    }

    FiniteField() {
        order = 1;
        size = 2;
        init();
    }

    void setOrder(int a) {
        finalize();
        order = a;
        size = 1 << a;
        init();
    }

    ~FiniteField() {
        finalize();
    }

    // Scalar operations

    inline int add(int a, int b) {
	    //return a ^ b;
	    return ff_add(a, b);
    }

    inline int sub(int a, int b) {
	    //return a ^ b;
	    return ff_sub(a, b);
    }

    inline int mul(int a, int b) {
	    //return mulTable[(a << order) | b];
	    return ff_mul(a, b);
    }

    inline int div(int a, int b) {
	    //return divTable[(a << order) | b];
	    return ff_div(a, b);
    }

    inline int mulCMP(int a, int b) {
	    //return mulTable2[(a << order) | b];
	    return ff_mul(a, b);
    }

    // vector operations

//    inline void addvv(SymVector &r, const SymVector &a, const SymVector &b) {
//        int i;
//        int len = a.size();
//        r.clear();
//        for (i = 0; i < len; i++) {
//            r.push_back(add(a[i], b[i]));
//        }
//    }
//
//    inline void mulvc(SymVector &r, const int c) {
//        SymVector::iterator it;
//        for (it = r.begin(); it < r.end(); it++) {
//            *it = mul(*it, c);
//        }
//    }
    
    inline void incr(SymbolType& r, int c=1){
        r = add(r,c);
    }
    
    inline void mulvc(SymbolType* r, const int c, const int T) {
	    ff_mulv_local(r, c, T);
/*	    
        for (int i = 0; i < T; i++) {
            r[i] = mul(r[i], c);
        }
*/
    }
    
    inline void mulvc(SymbolType* o, const SymbolType* r, const int c, const int T) {
	    ff_mulv(o, r, c, T);
/*
        for (int i = 0; i < T; i++) {
            o[i] = mul(r[i], c);
        }
*/
    }

    inline void mulvcCMP(SymbolType* r, const int c, const int T) {
	    ff_mulv_local(r, c, T);
/*
        for (int i = 0; i < T; i++) {
            r[i] = mulCMP(r[i], c);
        }
*/
    }

    inline void mulvcCMP(SymbolType* o, const SymbolType* r, const int c, const int T) {
	    ff_mulv(o, r, c, T);
/*
        for (int i = 0; i < T; i++) {
            o[i] = mulCMP(r[i], c);
        }
*/
    }
    
    
//    inline void addvvc(SymVector &a, const SymVector &b, const int c) {
//        int i;
//        int len = a.size();
//        for (i = 0; i < len; i++) {
//            a[i] = add(a[i], mul(b[i], c));
//            //            a[i] = a[i] ^ mulTable[(b[i]<<order)|c];
//        }
//    }

    inline void addvv(SymbolType *a, const SymbolType *b, const int len) {
	    ff_addv_local(a, b, len);
/*
        int i;
        for (i = 0; i < len; i++) {
            a[i] = add(a[i], b[i]);
            //            a[i] = a[i] ^ mulTable[(b[i]<<order)|c];
        }
*/
    }

    inline void addvvc(SymbolType *a, const SymbolType *b, const SymbolType c, const int len) {
	    ff_add_mulv_local(a, b, c, len);
/*
        int i;
        for (i = 0; i < len; i++) {
            a[i] = add(a[i], mul(b[i], c));
            //            a[i] = a[i] ^ mulTable[(b[i]<<order)|c];
        }
*/
    }

    inline void addvvcCMP(SymbolType *a, const SymbolType *b, const SymbolType c, const int len) {
	    ff_add_mulv_local(a, b, c, len);
/*
        int i;
        for (i = 0; i < len; i++) {
            a[i] = add(a[i], mulCMP(b[i], c));
            //            a[i] = a[i] ^ mulTable[(b[i]<<order)|c];
        }
*/
    }

//    inline void addvvcCMP(SymbolType *o, const SymbolType *a, const SymbolType *b, const SymbolType c, const int len) {
//        int i;
//        for (i = 0; i < len; i++) {
//            o[i] = add(a[i], mulCMP(b[i], c));
//            //            a[i] = a[i] ^ mulTable[(b[i]<<order)|c];
//        }
//    }

    inline int innerprod(const SymbolType* a, const SymbolType* b, const int len) {
	    return ff_innerprod(a, b, len);
/*
        int sum = 0;
        for (int i = 0; i < len; i++) {
            sum = add(sum, mul(a[i], b[i]));
        }
        return sum;
*/
    }
    
    inline void mulmcvCMP(SymbolType* o, SymbolType** Y, const SymbolType* v, const int N, const int T){
        mulvcCMP(o, Y[0], v[0], T);
        for (int i = 1; i < N; i++) {
            addvvcCMP(o, Y[i], v[i], T);
        }
    }
    
    inline void addvmcvCMP(SymbolType* o, SymbolType** Y, const SymbolType* v, const int N, const int T){
        for (int i = 0; i < N; i++) {
            addvvcCMP(o, Y[i], v[i], T);
        }
    }
    
    inline void mulmcv(SymbolType* o, SymbolType** Y, const SymbolType* v, const int N, const int T){
        mulvc(o, Y[0], v[0], T);
        for (int i = 1; i < N; i++) {
            addvvc(o, Y[i], v[i], T);
        }
    }

//    inline int innerprod(const SymVector &a, const SymVector &b) {
//        int sum = 0;
//        int len = a.size();
//        int i;
//        for (i = 0; i < len; i++) {
//            sum = add(sum, mul(a[i], b[i]));
//        }
//        return sum;
//    }
//
//    // row matrix mul colum matrix = row matrix
//    vector<SymVector> mulMrMc(vector<SymVector> &, vector<SymVector> &);
//
//    // col matrix mul colum matrix = col matrix
//    vector<SymVector> mulMcMc(vector<SymVector> &, vector<SymVector> &);

    // rank of a column matrix
    int rankM(SymbolType**, int, int);

//    // linear equations
//    int GaussianElimination(vector<SymVector> &, vector<SymVector> &);
        
    int GaussianElimination(SymbolType**, SymbolType**, int, int, int);
//    int GaussianEliminationCMP(SymbolType**, SymbolType**, int, int, int);
    
    // Solve x A = Y, to x [I 0] = X
    // A is a row x col matrix
    // Y is a rowY x col matrix
    // x is a rowY x row matrix
    int GaussianSolve(SymbolType** X, SymbolType** A, int row, int col, SymbolType** Y, int rowY, bool fu=false);
    
};


extern FiniteField FF;

#endif /* FINITEFIELD_H_ */
