#include <iostream>

#include "lattice.h"
#include "calcdb.h"

LatticeNode::LatticeNode(){
   _parent = new list<LatticeNode *>;
   _child = new list<LatticeNode *>;
   _mingen = new list<vector<int> >;
   _sup=0;
   _flg = 0;
}

LatticeNode::~LatticeNode()
{
   if (_parent) delete _parent;
   if (_child) delete _child;
   if (_mingen) delete _mingen;
}

LatticeNode::LatticeNode(LatticeNode *fit)
{
   (*(vector<int> *)this) = *((vector<int> *)fit);
   //this(*(vector<int> *)fit);
   _sup = fit->_sup;
   //cout << "COPY " << fit->seqcnt << " " << fit->seqsz << " " <<
   //   seqcnt << " " << seqsz << endl << flush;
}

LatticeNode::LatticeNode(vector<int> &vec, int sup)
{
   _parent = new list<LatticeNode *>;
   _child = new list<LatticeNode *>;
   _mingen = new list<vector<int> >;
   _sup = sup;
   
   (*(vector<int> *)this) = vec;
}
void LatticeNode::add(int it)
{
   push_back(it);
}

void LatticeNode::compact()
{
   //_seqsz = reAlloc(_seqcnt, sizeof(int), _seq);
}


void LatticeNode::sort(bool childpar_sort)
{
   std::sort(begin(), end());
   if (childpar_sort){
      _child->sort(cmpless);
      _parent->sort(cmpless);
   }
}

// ostream & operator << (ostream & out, vector<int> &vint)
// {
//    for (int i=0; i < vint.size(); ++i)
//       out << vint[i] << " ";
//    return out;
// }

void LatticeNode::print(int fend)
{
   cout << *this;
   //cout << "( " << _sup << " )";
   if (fend) cout << endl;
}

ostream& operator << (ostream& outputStream, LatticeNode& freq)
{
   //outputStream << "FREQ : ";
   //   outputStream << static_cast<vector<int>&> (freq);
   for (int i=0; i < freq.size(); ++i)
      outputStream << freq[i] << " ";   
   //outputStream << Dbase_Ctrl_Blk::FreqIdx[freq[i]] << " ";   
   outputStream << "( " << freq._sup << " )";
   return outputStream;
}

int LatticeNode::compare(LatticeNode *fit){
   LatticeNode *tmp = this;
   return LatticeNode::cmpfreqit(&tmp, &fit);
}


//this is a subset of par; return the difference par-this
void LatticeNode::subdiff(LatticeNode *par, LatticeNode *diff){
  diff->reset();
  int i, j;

  for (i=0, j=0; i < size() && j < par->size() ;){
    if ((*this)[i] > (*par)[j]){
      diff->add((*par)[j]);
      j++; 
    } 
    else if ((*this)[i] < (*par)[j]){
      i++;
    }
    else{
      i++;
      j++;
    }
  }
  for (; j < par->size(); j++)
    diff->add((*par)[j]);
}

subset_res LatticeNode::subset(LatticeNode *set)
{
   int i,j;
   int di=0, dj=0;
   
//    if (sortitemsets){
      for (i=0, j=0; i < size() && j < set->size() ;){
         if ((*this)[i] > (*set)[j]){
            j++;
            dj++;
         }
         else if ((*this)[i] < (*set)[j]){
            i++;
            di++;
         }
         else{
            i++;
            j++;
         }
      }
   
      
      if (i < size()) di++;
      if (j < set->size()) dj++;
      
      if (di == 0 && dj == 0) return EQUAL;
      else if (di == 0 && dj > 0) return SUBSET;
      else if (di > 0 && dj == 0) return SUPSET;
      else return NEQUAL;
//    }
//    else{
//       //freq itemsest are not sorted
//       int fcnt = 0, nfcnt = 0;
//       for (i=0; i < size(); ++i){
//          bool found=false;
//          for (j=0; j < set->size(); ++j){
//             if ((*this)[i] == (*set)[j]){
//                found = true;
//             }
//          }
//          if (found) fcnt++;
//          else nfcnt++;
//       }
//       if (fcnt == size() && fcnt == set->size()) return EQUAL;
//       else if (fcnt == size() && fcnt < set->size()) return SUBSET;
//       else if (fcnt < size() && fcnt == set->size()) return SUPSET;
//       else return NEQUAL;
//    }
   
}

int LatticeNode::cmpit(const void *f1, const void *f2)
{
   int i1 = *(int *)f1;
   int i2 = *(int *)f2;
   if (i1 > i2) return 1;
   else if (i1 < i2) return -1;
   else return 0;
}


int LatticeNode::cmpfreqit(const void *f1, const void *f2)
{
   int i, res=0;
   vector<int> *fit1 = *(vector<int> **)f1;
   vector<int> *fit2 = *(vector<int> **)f2;
   //compare size
   if (fit1->size() > fit2->size()) res = 1;
   else if (fit1->size() < fit2->size()) res = -1;


   if (res == 0){
      //compare items
      for (i=0; i < fit1->size(); i++){
         
         if ((*fit1)[i] < (*fit2)[i]) res = -1;
         else if ((*fit1)[i] > (*fit2)[i]) res = 1;
         if (res != 0) break;
      }
   }

   return res;
}


void LatticeNode::print_node(ostream &out, bool print_mingen){
   list<LatticeNode *>::iterator i;
   list<vector<int> >::iterator j;

   out << "NODE : " << *this << endl;
   
   out << "PARENTS : ";
   //parent()->sort(LatticeNode::cmpless);
   for (i = parent()->begin(); i!= parent()->end(); i++){
      out << *(*i) << " , ";
   }
   out << endl;
   
   out << "CHILDREN : ";
   //child()->sort(LatticeNode::cmpless);
   for (i = child()->begin(); i!= child()->end(); i++){
      out << *(*i) << " , ";
   }
   out << endl;

   if (print_mingen){
      out << "MINGENS : ";
      for (j = mingen()->begin(); j!= mingen()->end(); ++j){
         out << (*j) << " , ";
      }
      out << endl;
   }
   
}

void LatticeNode::print_lattice(LatticeNode *node, long marker)
{
   list<LatticeNode *>::iterator i;
   if (node->flag() == marker) return;
   
   node->flag() = marker;
   
   node->print_node(cout);
   
   for (i = node->child()->begin(); i!= node->child()->end(); i++){
      print_lattice(*i, marker);
   }
}

LatticeNode * LatticeNode::add_child(vector<int> &vec, int sup)
{
   LatticeNode *ln = new LatticeNode(vec, sup);
   _child->push_back(ln);
   ln->_parent->push_back(this);

   std::sort(ln->begin(), ln->end(), less<int>());
   return ln;
}

LatticeNode * LatticeNode::add_child(LatticeNode *ln)
{
   _child->push_back(ln);
   ln->_parent->push_back(this);
   return ln;
}

LatticeNode * LatticeNode::add_child(vector<int> &vec, int litem, int sup)
{
   LatticeNode *ln = new LatticeNode(vec, sup);
   ln->push_back(litem);
   _child->push_back(ln);
   ln->_parent->push_back(this);
   std::sort(ln->begin(), ln->end(), less<int>());
   return ln;
}

void LatticeNode::remove_child(LatticeNode *ln)
{
   list<LatticeNode *>::iterator i;
   for (i = _child->begin(); i!= _child->end(); ++i){
      if (*i == ln){
         i = _child->erase(i);
         break;
      }
   }
   //cout << "AFTERC :";
   //print_node();
}

void LatticeNode::remove_parent(LatticeNode *ln)
{
   list<LatticeNode *>::iterator i;
   for (i = _parent->begin(); i!= _parent->end(); ++i){
      if (*i == ln){
         i = _parent->erase(i);
         break;
      }
   }
   //cout << "AFTERP :";
   //print_node();
}

void  LatticeNode::mark_parents(long marker)
{
   if (_flg == marker) return;
   list<LatticeNode *>::iterator i;
   for (i = _parent->begin(); i!= _parent->end(); ++i){
      (*i)->mark_parents(marker);
      (*i)->flag() = marker;
   }
   
}

void  LatticeNode::mark_children(long marker)
{
   if (_flg == marker) return;
   list<LatticeNode *>::iterator i;
   for (i = _child->begin(); i!= _child->end(); ++i){
       (*i)->mark_children(marker);
       (*i)->flag() = marker;
   }
   
}
