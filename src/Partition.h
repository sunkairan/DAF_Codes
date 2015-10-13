#ifndef PARTITION_H
#define PARTITION_H

#include <vector>
#include <utility>
#include <iterator>

using namespace std;

template<typename TypeT> class ClassIterator;
template<typename TypeT> class RangeIterator;

template<typename TypeT>
class Partition {
public:
	//Default ctor
	Partition(int numClasses) : classMember(numClasses), m_Size(0), m_nClass(numClasses) { }
	
	void insert(TypeT x, int cls) {
		actualBuf.push_back(x);
		classIDandLoc.push_back(make_pair(cls, classMember[cls].size()));
		m_Size++;
		
		classMember[cls].push_back(m_Size - 1);
	}
	void changeClass(int id, int newcls) {
		int oldcls = classIDandLoc[id].first;
		if (oldcls == newcls)
			return;
		//Remove in old class
		int k = classIDandLoc[id].second;
		int len = classMember[oldcls].size();
		if (k != len - 1) {
			//Swap to the end
			int tmp;
			tmp = classMember[oldcls][k];
			classMember[oldcls][k] = classMember[oldcls][len-1];
			classMember[oldcls][len-1] = tmp;
			//Handle affected member
			classIDandLoc[classMember[oldcls][k]].second = k;
		}
		classMember[oldcls].pop_back();
		//Add in new class
		classMember[newcls].push_back(id);
		//Update classIDandLoc
		classIDandLoc[id].first = newcls;
		classIDandLoc[id].second = classMember[newcls].size() - 1;
	}
	int getClass(int id) { return classIDandLoc[id].first; }
	int size() { return m_Size; }
	int sizeCls(int cls) { return classMember[cls].size(); }
	
	TypeT get(int id) { return actualBuf[id]; }//TODO
	
	ClassIterator<TypeT> beginCls(int cls) {
		return ClassIterator<TypeT>(*this, cls);
	}
	ClassIterator<TypeT> endCls(int cls) {
		return ClassIterator<TypeT>(*this, cls, classMember[cls].size());
	}
	RangeIterator<TypeT> begin(int start, int end) {
		return RangeIterator<TypeT>(*this, start, end, start);
	}
	RangeIterator<TypeT> end(int start, int end) {
		return RangeIterator<TypeT>(*this, start, end, end+1);
	}
	friend class ClassIterator<TypeT>;
	friend class RangeIterator<TypeT>;
private:
	int m_Size;
	const int m_nClass;//cannot change in this version
	vector<TypeT> actualBuf;
	vector<pair<int,int> > classIDandLoc;//classID[i] == which class does actualBuf[i] belongs to and its index in classMember
	vector<vector<int> > classMember;//classMember[i] is list of all members belonging to class i, by their indices in actualBuf
};

template<typename TypeT>
class RangeIterator : public iterator<input_iterator_tag, TypeT> {
public:
	//Standard ctor
	RangeIterator(Partition<TypeT>& src, int start, int end, int i) : m_part(src), m_start(start), m_end(end), m_index(i) {}
	//copy-ctor
	RangeIterator(const RangeIterator<TypeT>& iter) : m_part(iter.m_part), m_start(iter.m_start), m_end(iter.m_end), m_index(iter.m_index) {}
	
	//copy-assignment
	RangeIterator<TypeT>& operator=(const RangeIterator<TypeT>& iter) {
		m_part = iter.m_part;
		m_start = iter.m_start;
		m_end = iter.m_end;
		m_index = iter.m_index;
		return *this;
	}
	
	//comparison
	bool operator==(const RangeIterator<TypeT>& iter) const {
		return (&m_part == &(iter.m_part)) && (m_start == iter.m_start) && (m_end == iter.m_end) && (m_index == iter.m_index);//address check
	}
	bool operator!=(const RangeIterator<TypeT>& iter) const {
		return !operator==(iter);
	}
	
	//dereference
	TypeT& operator*() {
		return m_part.actualBuf[m_index];
	}
	TypeT* operator->() {
		return &(m_part.actualBuf[m_index]);
	}
	
	//increment
	RangeIterator<TypeT>& operator++() { //prefix
		m_index++;
		return *this;
	}
	RangeIterator<TypeT> operator++(int) { //postfix
		RangeIterator<TypeT> clone(*this);
		m_index++;
		return clone;
	}
private:
	Partition<TypeT>& m_part;
	const int m_start, m_end;
	int m_index;
};

template<typename TypeT>
class ClassIterator : public iterator<input_iterator_tag, TypeT> {
public:
	//Standard ctor
	ClassIterator(Partition<TypeT>& src, int cls) : m_part(src), m_classID(cls), m_index(0) {}
	ClassIterator(Partition<TypeT>& src, int cls, int i) : m_part(src), m_classID(cls), m_index(i) {}
	//copy-ctor
	ClassIterator(const ClassIterator<TypeT>& iter) : m_part(iter.m_part), m_classID(iter.m_classID), m_index(iter.m_index) {}
	
	//copy-assignment
	ClassIterator<TypeT>& operator=(const ClassIterator<TypeT>& iter) {
		m_part = iter.m_part;
		m_classID = iter.m_classID;
		m_index = iter.m_index;
		return *this;
	}
	
	//comparison
	bool operator==(const ClassIterator<TypeT>& iter) const {
		return (&m_part == &(iter.m_part)) && (m_classID == iter.m_classID) && (m_index == iter.m_index);//address check
	}
	bool operator!=(const ClassIterator<TypeT>& iter) const {
		return !operator==(iter);
	}
	
	//dereference
	TypeT& operator*() {
		int actualID = m_part.classMember[m_classID][m_index];
		return m_part.actualBuf[actualID];
	}
	TypeT* operator->() {
		int actualID = m_part.classMember[m_classID][m_index];
		return &(m_part.actualBuf[actualID]);
	}
	
	//Increment
	ClassIterator<TypeT>& operator++() { //prefix
		m_index++;
		return *this;
	}
	ClassIterator<TypeT> operator++(int) { //postfix
		ClassIterator<TypeT> clone(*this);
		m_index++;
		return clone;
	}
private:
	Partition<TypeT>& m_part;
	const int m_classID;
	int m_index;
};

#endif /* PARTITION_H */

