#include <cstring>
#include <algorithm>
#include "constraints.h"

using namespace std;

Constraints::Constraints(char *name){
   m_file.open(name);
   if (!m_file.is_open()){
      cerr << "cannot open constraint file " << name << endl;
      exit(1);
   }

   //read the constraints, one per line
   //if first field is C then it is an col/attribute constraint
   //if first field is R then it is a row constraint
   //items on each line are considered to be conjunctions
   // different lines are considered as disjunctions
   //each line is assumed to be sorted in increasing order

   const int linesz = 16834;
   const int wordsz = 1024;
   char line[linesz];
   char word[wordsz];
   char constraintType[1];
      
   while(!m_file.eof()){
      m_file.getline(line, linesz);
      int line_len = m_file.gcount(); //how many chars read
         
      if (line_len <= 0) break;
         
      parseline pl(line, line_len);
      pl.next(constraintType);
         
      vector<int> *cons = new vector<int>;
      while (pl.next(word)){
         int w = atoi(word);
         cons->push_back(w);
      }
         
      if (strcmp(constraintType,"C") == 0){
         m_colConstraints.push_back(cons);
      }
      else if (strcmp(constraintType, "R") == 0){
         m_rowConstraints.push_back(cons);
         //cout << "ADD " << m_rowConstraints.size() << endl;
      }
      else{
         cout << constraintType << " option NOT IMPLEMENTED" << endl;
         exit(-1);
      }
         
      cout << "READ " << constraintType << " " << *cons << endl;
   }        
}
   
   
Constraints::~Constraints()
{
   vector<vector<int> *>::iterator i;
   for (i=m_rowConstraints.begin(); i != m_rowConstraints.end(); ++i){
      delete *i;
   }
   for (i=m_colConstraints.begin(); i != m_colConstraints.end(); ++i){
      delete *i;
   }
      
   m_file.close();
}
   
bool Constraints::checkRowConstraints(idlist& list, int iter)
{
   //if there are no row constraints, then by default they are satisfied
   if (m_rowConstraints.size() == 0){
      return true;
   }
   
   bool res = false;
   //cout << "CAME HEREa " << m_rowConstraints.size() << " == " <<
   //  iter << " " << diff_type << " -- " << list << endl;

   //we rely on the fact that for tidsets, we can simply check if the
   //constraints is included in the idlist given by list
   //for diffsets, we use the observation that if all the items were
   //checked to be present in the previous step, then after extension,
   //we have only to monitor if an item from the constraint is now
   //part of the diffset (i.e., if the constraint - list is not equal
   //to constraint), in which case the constraint is not satisfied 
   if (diff_type == nodiff || (diff_type == diff && iter <= 2) ||
       (diff_type == diff2 && iter <= 1)){
      const bool reverse = false;
      vector<vector<int> *>::iterator i;
      for (i = m_rowConstraints.begin(); 
           i != m_rowConstraints.end(); ++i){
         //cout << "CAME HERE " << *(*i) << endl;
         res = res || checkSubset(*(*i),list,reverse);
      }
   }
   else{
      const bool reverse = true;
      vector<vector<int> *>::iterator i;
      for (i = m_rowConstraints.begin(); 
           i != m_rowConstraints.end(); ++i){
         res = res || checkSubset(*(*i),list,reverse);
      }
   }
   return res;
}

//we rely on the fact that for tidsets, we can simply check if the
//constraints is included in the idlist given by list
//for diffsets, we use the observation that if all the items were
//checked to be present in the previous step, then after extension,
//we have only to monitor if an item from the constraint is now
//part of the diffset (i.e., if the constraint - list is not equal
//to constraint), in which case the constraint is not satisfied 
bool Constraints::checkSubset(vector<int> &pat, idlist &list, bool reverse)
{
   if (reverse){
      vector<int> tmp;
      insert_iterator< vector<int> > tadd(tmp, tmp.begin());
      set_difference(pat.begin(),pat.end(),
                     list.begin(), list.end(),tadd);
      if (tmp.size() == pat.size()) return true;
      else return false;
   }
   else{
      //cout << "CHECK " << pat << " -- " << list << endl;
      return includes(list.begin(), list.end(), pat.begin(), pat.end());
   }
}


//add eq->prefix to iset
//if addnodes flag is true, add each element in the class to eq
// if lval != INVALID then add lval to the iset
bool Constraints::checkColConstraints(Eqclass *eq, bool addnodes, int lval)
{
   //if there are no col constraints, then by default they are satisfied
   if (m_colConstraints.size() == 0){
      return true;
   }   

   //construct the set of items that is the union of prefix and all
   //other elements in eq (if addnodes), and add lval (if not
   //invalid)
   //have to convert each item to its original value
   //sort iset as well
   vector<int> iset;

   for (int j=0; j < eq->prefix().size(); ++j){
      iset.push_back(Dbase_Ctrl_Blk::FreqIdx[(eq->prefix())[j]]);
   }
   
   if (addnodes){
      list<Eqnode *>::iterator ni = eq->nlist().begin();
      for (; ni != eq->nlist().end(); ++ni){
         iset.push_back(Dbase_Ctrl_Blk::FreqIdx[(*ni)->val]);
      }
   }
   
   if (lval != INVALID) iset.push_back(Dbase_Ctrl_Blk::FreqIdx[lval]);
   
   sort(iset.begin(),iset.end());
   
   //now see if each constraint is subset of iset
   const bool reverse = false;
   bool res = false;
   vector<vector<int> *>::iterator i;
   for (i = m_colConstraints.begin(); 
        i != m_colConstraints.end(); ++i){
      res = res || checkSubset(*(*i),iset,reverse);
   }
   return res;   
}
