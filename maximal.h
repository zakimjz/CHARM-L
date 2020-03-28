#ifndef __maximal_H
#define __maximal_H

#include <vector>
#include "calcdb.h"
#include "eqclass.h"
#include "lattice.h"

class MaximalTest{
private:
   idlist tmp1, tmp2; //temp results to be stored
   vector<int> csuplist; //to store supports for closed sets
   vector<LatticeNode *> lattice_nodes;
public:
   int maxcount;
   MaximalTest():maxcount(0){}
   
   vector<LatticeNode *> &lattice(){ return lattice_nodes; }
   
   void add(vector<int> &prefix, int lit, int sup=0);
   void addnode(vector<int> &prefix, int lit, LatticeNode *ln, int sup=0);
   bool max_intersect(idlist *l1, idlist *inl2=NULL);
   bool max_union(int sup, idlist *l1, idlist *&res);
   int max_diff(idlist *l1, idlist *l2);
   bool update_maxset(list<Eqnode *>::iterator ni, Eqclass *eq, 
                      idlist &newmax, unsigned int nmaxpos, bool &subset);
   bool subset_test(list<Eqnode *>::iterator ni, Eqclass *eq);
   bool subset_test_f2(Eqclass *eq);
   bool check_closed(Eqnode *eqn);
   void adjustlattice(LatticeNode *current,
                      idlist &supersets);
};

#endif
