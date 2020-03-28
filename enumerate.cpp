#include<iostream>
#include <vector>
#include "eclat.h"
#include "timetrack.h"
#include "calcdb.h"
#include "eqclass.h"
#include "stats.h"
#include "maximal.h"
#include "chashtable.h"
#include "lattice.h"
#include "constraints.h"

typedef std::vector<bool, std::allocator<bool> > bit_vector;

//extern vars
extern Constraints *constraints;
extern ofstream outfile;
extern Dbase_Ctrl_Blk *DCB;
extern Stats stats;
extern MaximalTest maxtest; //for maximality & closed (with cmax) testing
cHashTable hashtest; //for closed (with chash) testing

//extern functions
extern void form_closed_f2_lists(Eqclass *eq);
extern void form_f2_lists(Eqclass *eq);
extern subset_vals get_intersect(idlist *l1, idlist *l2, 
                                 idlist *join, int minsup=0);
extern subset_vals get_diff (idlist *l1, idlist *l2, idlist *join);
extern subset_vals get_join(Eqnode *l1, Eqnode *l2, Eqnode *join, int iter);
extern void get_max_join(Eqnode *l1, Eqnode *l2, Eqnode *join, int iter);

void print_tabs(int depth)
{
   for (int i=0; i < depth; ++i)
      cout << "\t";
}

static bool notfrequent (Eqnode &n){
  //cout << "IN FREQ " << n.sup << endl;
  if (n.support() >= MINSUPPORT) return false;
  else return true;
}


void enumerate_closed_freq(Eqclass *eq, int iter, idlist &newmax, 
                           LatticeNode *lattice=NULL) 
{
   TimeTracker tt;
   Eqclass *neq;
   int nmaxpos;
   bool cflg;
   list<Eqnode *>::iterator ni, nj;
   Eqnode *join;
   subset_vals sval;

   bool addnodes = true;
   //if the eqclass as a whole does not contain the col constraints,
   //then no point proceeding
   if (check_constraints && !constraints->checkColConstraints(eq,addnodes))
      return;

   nmaxpos = newmax.size(); //initial newmax pos
   eq->sort_nodes();
   //print_tabs(iter-3);
   //cout << "F" << iter << " " << *eq;
   for (ni = eq->nlist().begin(); ni != eq->nlist().end(); ++ni){
      neq = new Eqclass;
      neq->set_prefix(eq->prefix(),*(*ni));

      //cout << "prefix " << neq->print_prefix() << endl;
      tt.Start();

      if (closed_type == cmax) 
         maxtest.update_maxset(ni, eq, newmax, nmaxpos, cflg);

      nmaxpos = newmax.size(); //initial newmax pos
      nj = ni;
      for (++nj; nj != eq->nlist().end(); ){
         join = new Eqnode ((*nj)->val);
//          if (check_constraints && 
//              !constraints->checkColConstraints(neq->prefix(), (*nj)->val)){
//             join->support() = 0;
//          }
//          else 
         sval = get_join(*ni, *nj, join, iter);
         
         stats.incrcand(iter-1);
         if (notfrequent(*join)){
            delete join;
            ++nj;
         } 
         else if (check_constraints && 
                  !constraints->checkRowConstraints(join->tidset,iter)){
            delete join;
            ++nj;
         }
         else{
            stats.incrlarge(iter-1);
            switch(sval){
            case subset:
               //add nj to all elements in eq by adding nj to prefix
               neq->prefix().push_back((*nj)->val);               
               //neq->closedsupport() = join->support();
               //cout << "SUSET " << *join << endl;
               delete join;
               ++nj;
               break;
            case notequal:
               if (closed_type == cmax) get_max_join(*ni, *nj, join, iter);
               if (neq->prefix().size()+1 <= max_closed_len){
                  neq->add_node(join);               
               }
               ++nj;
               break;
            case equals:
               //add nj to all elements in eq by adding nj to prefix
               neq->prefix().push_back((*nj)->val); 
               //neq->closedsupport() = join->support();
               delete *nj;
               nj = eq->nlist().erase(nj); //remove nj
               //cout << "EQUAL " << *join << endl;
               delete join;
               break;
            case superset:
               if (closed_type == cmax) get_max_join(*ni, *nj, join, iter);
               delete *nj;
               nj = eq->nlist().erase(nj); //remove nj
               //++nj;
               neq->add_node(join);            
               break;
            }
         }
      }
      
      LatticeNode *ln = NULL;
      cflg = true;
      if (closed_type == cmax){
         cflg = maxtest.check_closed(*ni);
         if (cflg){
            ln = lattice->add_child(neq->prefix(), (*ni)->sup);
            maxtest.adjustlattice(ln, (*ni)->maxset);
            maxtest.addnode(neq->prefix(), -1, ln, (*ni)->support());
            newmax.push_back(maxtest.maxcount-1);
         }
      }
      else if (closed_type == chash){
         cflg = hashtest.add(neq->prefix(), -1, (*ni)->support(), 
                             (*ni)->hval);
      }
         
      if (cflg){
         stats.incrmax(neq->prefix().size()-1);
         if (output){
            if (!check_constraints || 
                (check_constraints && constraints->checkColConstraints(neq))){
               neq->print_prefix(outfile);
               outfile << "- " << (*ni)->sup;
               if (output_idlist) (*ni)->print_node(outfile,false);            
               outfile << endl;
            }
         }
      }
      
      stats.incrtime(iter-1, tt.Stop());

      if (neq->nlist().size() > 1){
         if (!ln) ln = lattice;
         enumerate_closed_freq(neq, iter+1, newmax, ln);
      }
      else if (neq->nlist().size() == 1){
         //if the eqclass as a whole does not contain the col constraints,
         //then no point proceeding
         addnodes = true;
         cflg = true;
         if (closed_type == cmax){
            cflg = maxtest.check_closed(neq->nlist().front());
            if (cflg){
               LatticeNode *ln2;
               if (ln){
                  ln2 = ln->add_child(neq->prefix(), 
                                      neq->nlist().front()->val,
                                      neq->nlist().front()->sup);
               }
               else{
                  ln2 = lattice->add_child(neq->prefix(), 
                                           neq->nlist().front()->val,
                                           neq->nlist().front()->sup);
               }
               maxtest.adjustlattice(ln2, neq->nlist().front()->maxset);
               maxtest.addnode(neq->prefix(), neq->nlist().front()->val, 
                               ln2, neq->nlist().front()->sup);
               newmax.push_back(maxtest.maxcount-1);
            }
         }
         else if (closed_type == chash){
            cflg = hashtest.add(neq->prefix(), neq->nlist().front()->val, 
                                neq->nlist().front()->sup, 
                                neq->nlist().front()->hval);
         }
            
         if (cflg){
            if (output){
               if (!check_constraints || 
                   (check_constraints && 
                    constraints->checkColConstraints(neq,addnodes))){
                  outfile << *neq;
               }
               
               stats.incrmax(neq->prefix().size());
            }
         }
      }
      delete neq;
   }
}

void enumerate_freq(Eqclass *eq, int iter)
{
   TimeTracker tt;
   Eqclass *neq;
   list<Eqnode *>::iterator ni, nj;
   Eqnode *join;

   for (ni = eq->nlist().begin(); ni != eq->nlist().end(); ++ni){
      neq = new Eqclass;
      neq->set_prefix(eq->prefix(),*(*ni));
      tt.Start();
      nj = ni;
      for (++nj; nj != eq->nlist().end(); ++nj){ 
         join = new Eqnode ((*nj)->val);
         get_join(*ni, *nj, join, iter);
         stats.incrcand(iter-1);
         if (notfrequent(*join)) delete join;
         else{
            neq->add_node(join);
            stats.incrlarge(iter-1);
         }
      }
      stats.incrtime(iter-1, tt.Stop());
      if (output) outfile << *neq;
      if (neq->nlist().size()> 1){
         enumerate_freq(neq, iter+1);
      }
      delete neq;
   }
}

LatticeNode *form_closed_f2_lists(Eqclass *eq, LatticeNode *lattice=NULL)
{
   bool addnodes = true;
   
   static bit_vector *bvec = NULL;
   if (bvec == NULL){
      bvec = new bit_vector(DCB->NumF1, true);
   }
   
   //if the eqclass as a whole does not contain the col constraints,
   //then no point proceeding
   addnodes = true;
   if (check_constraints && !constraints->checkColConstraints(eq,addnodes))
      return NULL;

   subset_vals sval;
   list<Eqnode *>::iterator ni;
   Eqnode *l1, *l2;
   int pit, nit;
   TimeTracker tt;
   bool extend = false;
   bool cflg;
   
   tt.Start();
   pit = eq->prefix()[0];
   l1 = DCB->ParentClass[pit];

   //check the rowconstraints
   if (check_constraints){
      int iter = 1;
      if (!constraints->checkRowConstraints(l1->tidset,iter)){
         eq->nlist().clear();
         return NULL;
      }
   }
   

   //bvec is set to false if the item has already been incorporated
   //into a "composite" item via the superset/equal case below
   //such an item can be skipped
   if (!(*bvec)[pit]){
      eq->nlist().clear();
     return NULL;
   }
   
   for (ni=eq->nlist().begin(); ni != eq->nlist().end(); ){
      nit = (*ni)->val;
      if (!(*bvec)[nit]){
         delete *ni;
         ni = eq->nlist().erase(ni); //remove ni
         //++ni;
         continue;
      }
      l2 = DCB->ParentClass[nit];

      sval = get_join(l1, l2, (*ni), 2);
      
      if (notfrequent(*(*ni))){
         delete *ni;
         ni = eq->nlist().erase(ni);
         continue;
      }
      else if (check_constraints){
         int iter = 2;
         if (!constraints->checkRowConstraints((*ni)->tidset,iter)){
            delete *ni;
            ni = eq->nlist().erase(ni);            
            continue;
         }
      }
      
      //cout << "SVAL " << (int)sval << endl;
      switch(sval){
      case subset:
         //add nj to all elements in eq by adding nj to prefix
         eq->prefix().push_back((*ni)->val);               
         extend = true;
         //eq->closedsupport() = (*ni)->support();
         delete *ni;
         ni = eq->nlist().erase(ni); //remove ni
         //cout << "CAME HERE " << eq->nlist().size() << endl;
         break;
      case notequal:
         if (alg_type == maxcharm || closed_type == cmax) 
            get_max_join(l1, l2, (*ni), 2);
         ++ni;
         break;
      case equals:
         //add nj to all elements in eq by adding nj to prefix
         eq->prefix().push_back((*ni)->val); 
         extend = true;
         //eq->closedsupport() = (*ni)->support();
         delete *ni;
         ni = eq->nlist().erase(ni); //remove ni
         (*bvec)[nit] = false;
         //cout << "CAME HERE " << eq->nlist().size() << endl;
         break;
      case superset:
         (*bvec)[nit] = false;
         if (alg_type == maxcharm || closed_type == cmax) 
            get_max_join(l1, l2, (*ni), 2);
         ++ni;
         break;
      }
   }
   
   
   LatticeNode *ln = NULL;
   if (alg_type == charm){

      cflg = true;
      if (closed_type == cmax){
         cflg = maxtest.check_closed(l1);
         if (cflg){
            ln = lattice->add_child(eq->prefix(), l1->sup);
            maxtest.adjustlattice(ln, l1->maxset);
            maxtest.addnode(eq->prefix(), -1, ln, l1->support());
         }
      }
      else if (closed_type == chash){
         cflg = hashtest.add(eq->prefix(), -1, l1->support(), 
                             l1->hval);
      }
      
      if (cflg){
         stats.incrmax(eq->prefix().size()-1);
         if (output){
            if (!check_constraints || 
                (check_constraints && constraints->checkColConstraints(eq))){
               eq->print_prefix(outfile);
               outfile << "- " << l1->sup;
               if (output_idlist){
                  l1->print_node(outfile,false);
               }
               outfile << endl;
            }
         }
      }
      
      
      if (eq->nlist().size() == 1){
         //if the eqclass as a whole does not contain the col constraints,
         //then no point proceeding
         addnodes = true;
         cflg = true;
         if (closed_type == cmax){
            cflg = maxtest.check_closed(eq->nlist().front());
            if (cflg){
               LatticeNode *ln2;
               if (ln){
                  ln2 = ln->add_child(eq->prefix(), 
                                      eq->nlist().front()->val, 
                                      eq->nlist().front()->sup);
                     
               }
               else{
                  ln2 = lattice->add_child(eq->prefix(), 
                                           eq->nlist().front()->val,
                                           eq->nlist().front()->sup);
               }
               maxtest.adjustlattice(ln2, eq->nlist().front()->maxset);
               maxtest.addnode(eq->prefix(), eq->nlist().front()->val, ln2, 
                               eq->nlist().front()->sup);
            }
         }
         else if (closed_type == chash){
            cflg = hashtest.add(eq->prefix(), eq->nlist().front()->val, 
                                eq->nlist().front()->sup, 
                                eq->nlist().front()->hval);
         }
            
         if (cflg){
            stats.incrmax(eq->prefix().size());
            if (output){
               if (!check_constraints || 
                   (check_constraints && 
                    constraints->checkColConstraints(eq,addnodes))){
                  outfile << *eq;
               }
            }
         }
         eq->nlist().clear();
      }
   }

   //cout << "F2 " << *eq << endl;
   stats.incrtime(1,tt.Stop());   
   //delete l1;
   return ln;
}



void form_f2_lists(Eqclass *eq)
{
   list<Eqnode *>::iterator ni;
   Eqnode *l1, *l2;
   int pit, nit;
   TimeTracker tt;
   
   tt.Start();
   //cout << "F2 " << *eq << endl;
   pit = eq->prefix()[0];
   l1 = DCB->ParentClass[pit];
   for (ni=eq->nlist().begin(); ni != eq->nlist().end(); ++ni){
      nit = (*ni)->val;
      l2 = DCB->ParentClass[nit];

      get_join(l1, l2, (*ni), 2);
      if (alg_type == basicmax) 
         get_max_join(l1, l2, (*ni), 2);
//        cout << "LISTS " << pit << " " << nit << " " 
//             <<DCB->FreqIdx[pit] << " " << DCB->FreqIdx[nit] 
//             << " " << l1->sup << " " << l2->sup << " " << (*ni)->sup <<endl;
      //cout << "f2prefix " << eq->prefix() << endl;
      //cout << "f2 " << *ins;
      //cout << "F2 " << l1->maxset << " xx  " << l2->maxset << endl;
   }
   //delete l1;
   stats.incrtime(1,tt.Stop());   
}


