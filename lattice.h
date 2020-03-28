#ifndef __LATTICE__
#define __LATTICE__
#include <list>
#include <vector>
#include <algorithm>

using namespace std;

enum subset_res {SUBSET, SUPSET, EQUAL, NEQUAL};

//extern ostream & operator << (ostream & out, vector<int> &vint);


class LatticeNode: public vector<int>{
private:
   int _sup;
   long _flg;
   list<LatticeNode *> *_parent;
   list<LatticeNode *> *_child;
   list<vector<int> > *_mingen; //minimal generators
public:

   LatticeNode();
   LatticeNode(vector<int> &vec, int sup);
   LatticeNode(LatticeNode *fit);
   ~LatticeNode();
   
   list<LatticeNode *> *&parent(){ return _parent;}
   list<LatticeNode *> *&child(){ return _child;}
   list<vector<int> > *&mingen(){ return _mingen;}
   long &flag(){ return _flg; }
   int &sup(){ return _sup; }
   
   void add(int it);
   LatticeNode * add_child(vector<int> &vec, int sup);
   LatticeNode * add_child(vector<int> &vec, int litem, int sup);
   LatticeNode * add_child(LatticeNode *ln);
   void remove_child(LatticeNode *ln);
   void remove_parent(LatticeNode *ln);
   void compact();
   void reset(){ clear(); }
   void sort(bool childpar_sort=false);

   int compare(LatticeNode *fit);
   subset_res subset(LatticeNode *);
   void subdiff(LatticeNode *par, LatticeNode *diff);
   
   void print(int fend=1);
   void mark_parents(long marker);
   void mark_children(long marker);

   static bool cmpless (const LatticeNode *x, const LatticeNode *y){
     if (cmpfreqit(&x,&y) == -1) return true;
     else return false;
   }        
   bool operator == (const LatticeNode &x)
   {
      if (cmpfreqit(this, &x) == 0) return true;
      else return false;
   }
   
   static int cmpit(const void *f1, const void *f2);
   static int cmpfreqit(const void *f1, const void *f2);
   static void print_lattice(LatticeNode *node, long marker);
   void print_node(ostream &out, bool print_mingen=false);
   friend ostream& operator << (ostream& outputStream, LatticeNode& freq);
};

#endif //__LATTICE__




