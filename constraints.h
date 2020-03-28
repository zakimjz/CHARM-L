#ifndef __CONS_H
#define __CONS_H

#include <iostream>
#include "eclat.h"
#include "calcdb.h"
#include "eqclass.h"

#define INVALID -1

class Constraints{
public:
   Constraints(char *name);
   ~Constraints();
   
   const vector<vector<int> *> &rowConstraints()
   {
      return m_rowConstraints;
   }

   const vector<vector<int> *> &colConstraints()
   {
      return m_colConstraints;
   }
   
   bool checkRowConstraints(idlist& list, int iter);
   bool checkSubset(vector<int> &pat, idlist &list, bool reverse=false);
   bool checkColConstraints(Eqclass *eq, bool addnodes=false, int lval=INVALID);
   
private:
   ifstream m_file; //constraints file
   vector<vector<int> *> m_rowConstraints;
   vector<vector<int> *> m_colConstraints;
};

#endif
