#include <iterator>
#include <iostream>
#include <vector>
#include <algorithm>
#include <ext/hash_map>

#include "eclat.h"
#include "lattice.h"
#include "calcdb.h"
#include "timetrack.h"
#include "chashtable.h"
#include "constraints.h"

extern double rulegen_time, mingen_time;
extern bool print_rules;
extern Constraints *constraints;
extern double MINCONF;

using namespace __gnu_cxx;
using namespace std;

#define HASHNS __gnu_cxx

int NUMCLOSEDRULES=0;
int NUMCLOSEDEXAMINED=0;

//apriori-style mingen program
void apriori_mingen(vector<int> &U, LatticeNode *node)
{
   list<vector<int> >::iterator li, li2, li3, lp;
   list<LatticeNode *>::iterator ci;
   list<vector<int> > cands, cands2;
   vector<int> newcand;
   bool found;
   //cout << "CANDS" << endl;
   for (int i=0; i < U.size(); ++i){
      newcand.clear();
      newcand.push_back(U[i]);
      cands.push_back(newcand);
      //cout << newcand << endl;
   }
   
   lp = --(node->mingen()->end());
   while (!cands.empty()){
      cands2.clear();
      for (li=cands.begin(); li != cands.end();){
         found = false;
         for (ci=node->child()->begin(); 
              ci != node->child()->end() && !found; ++ci){
            found = includes((*ci)->begin(), (*ci)->end(), 
                             (*li).begin(), (*li).end());
         }
         if (!found){
            node->mingen()->push_back(*li);
            cands.erase(li++);
         }
         else{
            cands2.push_back(*li);
            //cout << "CANDS2 " << (*li) << endl;
            ++li;
         }
      }
      cands.clear();
      for (li=cands2.begin(); li != cands2.end(); ++li){
         for (li2 = li, ++li2; li2 != cands2.end(); ++li2){
            if (equal((*li).begin(), --((*li).end()),(*li2).begin())){
               newcand.clear();
               newcand = (*li);
               newcand.push_back((*li2).back());
               //cout << "NEW CAND " << newcand << endl;
               found = false;
               for (li3= node->mingen()->begin();
                    li3 != node->mingen()->end() && !found; ++li3){
                  found = includes(newcand.begin(), newcand.end(),
                                   (*li3).begin(), (*li3).end());
               }
               if (!found){
                  cands.push_back(newcand);
               }
            }
            //else cout << "NOt REQUSAL\n";
         }
      }
   }
   
}


bool operator < (vector<int> &v1, vector<int> &v2)
{
//    if (v1.size() < v2.size()) return true;
//    else if (v1.size() > v2.size()) return false;
//    else{
   return lexicographical_compare(v1.begin(),v1.end(),v2.begin(),v2.end());
   //   }
}

bool operator == (vector<int> &v1, vector<int> &v2)
{
   if (v1.size() == v2.size()){
      return equal(v1.begin(),v1.end(),v2.begin());
   }
   return false;
}
//computes the minimal generators for each lattice node
//makrer is a unique val to mark visited nodes
void compute_mingenerators(LatticeNode *node, long marker)
{
   if (node->flag() == marker) return; 
   int i;
   
   node->flag() = marker;
   list<LatticeNode *>::iterator l,k;
   typedef HASHNS::hash_map <int, bool, HASHNS::hash<int>, equal_to<int> > myhmap;
   myhmap hmap;
   
   //cout << "PROCESS NODE " << *node << endl;
   LatticeNode *child;
   vector<int> mingen, rem;
   for (l = node->child()->begin(); l!= node->child()->end(); ++l){
      child = *l;

      //first compute child mingens recursively
      compute_mingenerators(child, marker);
      
      //mark items in the union of all children
      for (i=0; i < child->size(); ++i){
         hmap[(*child)[i]] = true;
      }
   }   

   rem.clear();
   //each unmarked item is a mingen, the remaining items are in rem
   for (i=0; i < node->size(); ++i){
      myhmap::iterator hi = hmap.find((*node)[i]);
      if (hi == hmap.end()){
         mingen.clear();
         mingen.push_back((*node)[i]);
         node->mingen()->push_back(mingen);
      }
      else rem.push_back((*node)[i]);
   }

   //cout << "remmm " << rem << endl;

   apriori_mingen(rem, node);
   node->mingen()->sort();
}


bool verify_mingen(vector<int> &mingen, IVhash &ivhash)
{
   for (int i=0; i < mingen.size(); ++i){
      vector<int> u;
      //cout << "PROCESS " << mingen[i] << "-- "<<ivhash[mingen[i]] << endl;
      for (int j=0; j < mingen.size(); ++j){
         if (j != i){
            //cout << "ADD " << mingen[j] << "-- "<<ivhash[mingen[j]] << endl;
            insert_iterator< vector<int> > ii(u,u.end());
            copy(ivhash[mingen[j]].begin(),ivhash[mingen[j]].end(),ii);
         }
      }
      
      sort(u.begin(),u.end());
      int tt;
      //cout << "COMPARE " << ivhash[mingen[i]] << " xx " << u << endl;
      subset_vals res = get_intersect(&ivhash[mingen[i]], &u,NULL,tt);
      if (res == subset) return false;
   }
   
   return true;
}

void compute_mhs(LatticeNode *node, list <vector<int> > &diffs, 
                 list <vector<int> >::iterator curr, 
                 vector<int> &mingen, IBhash &ibhash,
                 IVhash &cover, IVhash &ivhash)
{
   int i;
   bool found = false;
   //cout << "MHS " << (*curr).size() << " --  " << *curr << endl;

   //search if any item from curr has already been added to mingen so
   //far, if so we need to skip curr totally
   for (i=0; i < (*curr).size(); ++i){
      IBhash::iterator res = ibhash.find((*curr)[i]);
      if (res != ibhash.end() && res->second == true){
         found = true;
         break;
      }
   }
      
   if (found){
      //cout << "FOUND " << mingen << endl;
      list <vector<int> >::iterator next = curr;
      ++next;
      if (next == diffs.end()){
         //cout << "MINGENx " << mingen << endl;

         //verify that no item is unnecessary in each mingen
         if (verify_mingen(mingen,ivhash)){
            //add to mingen and sort
            node->mingen()->push_back(mingen);
            sort(node->mingen()->back().begin(),node->mingen()->back().end());
         }
         
      }
      else{
         compute_mhs(node,diffs,next,mingen,ibhash, cover, ivhash);
      }      
   }
   else{

      //now process the remaining items
      for (i=0; i < (*curr).size(); ++i){
         //cout << "PITEM " << (*curr)[i] << endl;

         //if (*curr)[i] is found in any cover of previous items in
         //mingen, skip to next item
         bool skip = false;
         for (int j=0; j < mingen.size(); ++j){
            if (binary_search(cover[mingen[j]].begin(),
                              cover[mingen[j]].end(), (*curr)[i])){
               skip = true;
               break;
            }
         }
         
         if (skip) continue;
         
         mingen.push_back((*curr)[i]);
         ibhash[(*curr)[i]] = true;
         list <vector<int> >::iterator next = curr;
         ++next;
         if (next == diffs.end()){
            //cout << "MINGENy " << mingen << endl;
            //add to mingen and sort
            //verify that no item is unnecessary in each mingen
            if (verify_mingen(mingen,ivhash)){
               node->mingen()->push_back(mingen);
               sort(node->mingen()->back().begin(),
                    node->mingen()->back().end());
            }
         }
         else{
            compute_mhs(node,diffs,next,mingen,ibhash, cover, ivhash);
         }
         mingen.pop_back();
         ibhash[(*curr)[i]] = false;
      }
   }
}


//computes the minimal generators for each lattice node
// via the concept of hitting sets
// Given a node N and its children C_i, compute D_i = N-C_i for all i
//then the minimal hitting sets of all D_i are mingens of N 
void compute_hs_mingenerators(LatticeNode *node, long marker)
{
   if (node->flag() == marker) return; 
   int i;
   
   node->flag() = marker;
   list<LatticeNode *>::iterator l,k;
   list <vector<int> > diffs;

   //cout << "PROCESS NODE " << *node << endl;
   LatticeNode *child;
   vector<int> mingen, diff;
   diff.resize(node->size());
   if (node->child()->empty()){
      diff = *node;
      diffs.push_back(diff);
      //cout << "DIFFx " << *node << " xx " << diff <<  endl;
   }
   else{
      for (l = node->child()->begin(); l!= node->child()->end(); ++l){
         child = *l;
         
         //first compute child mingens recursively
         compute_hs_mingenerators(child, marker);
      }
      
      for (l = node->child()->begin(); l!= node->child()->end(); ++l){
         child = *l;
         
         //compute diff of node and its child
         diff.clear();
         insert_iterator< vector<int> > inserter(diff, diff.begin());
         set_difference(node->begin(),node->end(),
                        child->begin(),child->end(),inserter);
         //cout << "DIFFy " << *node << " -- " << *child 
         //    << " xx " << diff <<  endl; 
         diffs.push_back(diff);
      }   
   }

   //get common items to all diffs, if any
   vector<int> common, tmp;
   list <vector<int> >::iterator lic = diffs.begin();
   
   common = diffs.front();
   
   for (++lic; lic != diffs.end(); ++lic){
      tmp.clear();
      insert_iterator< vector<int> > tadd(tmp, tmp.begin());
      set_intersection((*lic).begin(),(*lic).end(),
                       common.begin(),common.end(),tadd);
      common = tmp;
   }
   
   //cout << "COMMON " << common << endl;
   //remove common from all diff sets
   lic = diffs.begin();
   for (; lic != diffs.end(); ++lic){
      tmp.clear();
      insert_iterator< vector<int> > tadd(tmp, tmp.begin());
      set_difference((*lic).begin(),(*lic).end(),
                     common.begin(),common.end(),tadd);
      (*lic) = tmp;
   }

   //add each item in common as mingen
   for (i=0; i < common.size(); ++i){
      mingen.clear();
      mingen.push_back(common[i]);
      node->mingen()->push_back(mingen);
   }   
   

   //for each item idenfiy the covering items (i.e., Y is
   //covering for X iff the sets in which X occurs are a subset of
   //those diff sets in which Y occurs)
   //these items will be skipped for the mingen construction
   lic = diffs.begin();
   int id=0;
   IVhash ivhash;
   for (; lic != diffs.end(); ++lic, ++id){
      for (int i=0; i < (*lic).size(); ++i){
         ivhash[(*lic)[i]].push_back(id);
      }
   }
   
   IVhash cover;
   
   IVhash::iterator ivi = ivhash.begin();
   for (; ivi != ivhash.end(); ++ivi){
      //cout << "IV " << (*ivi).first << " -- " << (*ivi).second << endl;
      IVhash::iterator ivj = ivi; ++ivj;
      for (; ivj != ivhash.end(); ++ivj){
         subset_vals res = get_intersect(&((*ivi).second), 
                                         &((*ivj).second),NULL,id);
         if (res == subset){
            cover[(*ivi).first].push_back((*ivj).first);
         }
         else if (res == superset){
            cover[(*ivj).first].push_back((*ivi).first);
         }
         else if (res == equals){
            cover[(*ivi).first].push_back((*ivj).first);
            cover[(*ivj).first].push_back((*ivi).first);
         }
      }
   }

   ivi = cover.begin();
   for (; ivi != cover.end(); ++ivi){
      sort(cover[(*ivi).first].begin(),cover[(*ivi).first].end());
      //cout << "COVER " << (*ivi).first << " -- " << (*ivi).second << endl;
   }
   
   
   
   //node compute the minimal hitting sets of sets in diffs
   IBhash ibhash;
   mingen.clear();
   compute_mhs(node, diffs, diffs.begin(), mingen, ibhash, cover, ivhash);

   node->mingen()->sort();

   //now remove duplicate mingens if any
   list<vector<int> >::iterator ii = node->mingen()->begin();
   list<vector<int> >::iterator jj;
   for (; ii != node->mingen()->end();){
      jj = ii;
      ++jj;
      if (jj != node->mingen()->end()){
         if ((*ii) == (*jj)){
            ii = node->mingen()->erase(ii);
         }
         else ++ii;
      }
      else ++ii;
   }
   
}

void relabel_items(LatticeNode *node)
{
   for (int i=0; i < node->size(); ++i){
      (*node)[i] = Dbase_Ctrl_Blk::FreqIdx[(*node)[i]];
   }
}

bool cmpvecint(vector<int> &fit1, vector<int> &fit2)
{
   if (fit1.size() > fit2.size()) return false;
   else if (fit1.size() < fit2.size()) return true;

   //cout << "SIZE " << fit1->seqcnt << " " <<fit2->seqcnt << endl << flush;
   //compare items & template bits
   for (int i=0; i < fit1.size(); i++){
      if (fit1[i] < fit2[i]) return true;
      else if (fit1[i] > fit2[i]) return false;
   }
   return false;
}

void diffvec (vector<int> &res, vector<int> &v1, vector<int> &v2)
{
   //vector<int>::iterator lp = 
   res.clear();
   insert_iterator<vector<int> > ins(res,res.begin());
   set_difference(v1.begin(), v1.end(),v2.begin(), v2.end(), ins);
   //res.erase(lp, res.end());
}


void diffvec (vector<int> &res, vector<int> &v1, LatticeNode &v2)
{
   //vector<int>::iterator lp = 
   res.clear();
   insert_iterator<vector<int> > ins(res,res.begin());
   set_difference(v1.begin(), v1.end(),
                  v2.begin(), v2.end(), ins);
   //res.erase(lp, res.end());
}

bool subset_of_any (vector<int> &set, list<LatticeNode *> &setary)
{
   list<LatticeNode *>::iterator li = setary.begin();
   bool flg = false;
   LatticeNode *node;
   for (; li != setary.end() && !flg; ++li){
      node = (*li);
      flg = includes((*node).begin(), (*node).end(), set.begin(), set.end());
   }
   return flg;
}

bool superset_of_any (list<vector<int> >::iterator set, 
                      list<vector<int> >::iterator beg, 
                      list<vector<int> >::iterator enn)
{
   list<vector<int> >::iterator li = beg;
   bool flg = false;
   for (; li != enn && !flg; ++li){
      if (li != set)
         flg = includes((*set).begin(), (*set).end(), 
                        (*li).begin(), (*li).end());
   }
   return flg;
}

bool checkCons(vector<int> &v1, vector<int> &v2)
{
   vector<int> U = v1;
   U.insert(U.end(),v2.begin(),v2.end());
   sort(U.begin(),U.end());
   vector<vector<int> *>::const_iterator i;
   bool res = false;
   for (i = (constraints->colConstraints()).begin(); 
        i != (constraints->colConstraints()).end(); ++i){
      res = res || constraints->checkSubset(*(*i),U);
   }
   return res;      
}


//generate all closed rules between a parent and its child nodes
//makrer is a unique val to mark visited nodes
void rule_generation(LatticeNode *node)
{
   list<LatticeNode *>::iterator l;

   //cout << marker << " PROCESS NODE " << *node << endl;
   LatticeNode *child;

   list<vector<int> >::iterator n, k;
   vector<int> RHS, LHS;
   list<vector<int> > lRHS, lLHS;
   list<vector<int> >::iterator li, li2;
   list<double> confs;
   
   RHS.reserve(node->size());
   
    //generate rules between node and itself: 100% self-rules
   for (n = node->mingen()->begin(); n != node->mingen()->end(); ++n){
      if (!check_constraints || (check_constraints && checkCons(*n, *k))){
         for (k=n, ++k; k != node->mingen()->end(); ++k){
            NUMCLOSEDEXAMINED+=2; //for <== and ==> rules
            diffvec(LHS, (*n), (*k));
            if (LHS.size() == (*n).size()){
               NUMCLOSEDRULES+=2;
               if (print_rules){
                  outfile << "SELF " << (*n) << " <==> " << (*k) << " " 
                          << "( " << node->sup() << " " << 1.0 << " )" << endl;
               }
            }
            else if (conditional_self_rules){
               diffvec(RHS, (*k), (*n));
               vector<int> COM;
               diffvec(COM,(*n),LHS);
               if (print_rules){
                  outfile << "SELF-CONDITIONAL " << LHS << " <==> " << RHS << " " 
                          << " | " << COM
                          << " ( " << node->sup() << " " << 1.0 << " )" << endl;
               }              
            }
         }
      }
   }
   

//    for (n = node->mingen()->begin(); n != node->mingen()->end(); ++n){
//       lRHS.clear();
//       //first compute differences
//       for (k=node->mingen()->begin(); k != node->mingen()->end(); ++k){
//          NUMCLOSEDEXAMINED++;
//          RHS.clear();
//          diffvec(RHS, (*k), (*n));
//          //cout << "DIFF " << RHS << " -- "  << (*k) << " -- " << (*n) << endl;
//          if (!RHS.empty()){
//             lRHS.push_back(RHS);
//          }
//       }
//       //sort for checking superset info below
//       lRHS.sort(cmpvecint);
    
//       //make sure that RHS is not a superset of any other diff
//       for (li=lRHS.begin(); li != lRHS.end(); ){
//          //if (superset_of_any((*li), lRHS.begin(), li)){
//          if (superset_of_any(li, lRHS.begin(), lRHS.end())){
//             li2 = li;
//             ++li;
//             lRHS.erase(li2);
//          }
//          else ++li;
//       }
      
//       //generate self rules
//       for (li=lRHS.begin(); li != lRHS.end(); ++li){
//          if (!subset_of_any((*li), *(node->child()))){
//             NUMCLOSEDRULES++; 
//             if (print_rules)
//                cout << "SELF " << (*n) << " ==> " << (*li) << " " 
//                     << "( " << node->sup() << " " << 1.0 << " )" << endl;
//          }
//       }
//    }
   
   if (self_rules_only){
      return; //don't do steps below
   }
   
   for (l = node->child()->begin(); l != node->child()->end(); ++l){
      child = *l;
            
      //generate rules between node and child k using mingens
      for (n = node->mingen()->begin(); n != node->mingen()->end(); ++n){
         //100% conf rules (node to child)
         lRHS.clear();
         for (k = child->mingen()->begin(); k != child->mingen()->end(); ++k){
            //RHS.clear();
            //RHS must have enough memory for results below
            //RHS.reserve(k->size()); 

            diffvec(RHS, (*k), (*n));
            //cout << "DIFF " << *k << " -- " << *n << "-- " << RHS << endl;
            NUMCLOSEDEXAMINED++;
            
            if (!RHS.empty()) lRHS.push_back(RHS);
         }
         
         //sort for checking superset info below
         lRHS.sort(cmpvecint);
         
         //make sure that RHS is not a superset of any other diff
         for (li=lRHS.begin(); li != lRHS.end(); ){
            //if (superset_of_any((*li), lRHS.begin(), li)){
            if (superset_of_any(li, lRHS.begin(), lRHS.end())){
               li2 = li;
               ++li;
               lRHS.erase(li2);
            }
            else ++li;
         }

         //generate rules
         for (li=lRHS.begin(); li != lRHS.end(); ++li){
            //ensure that c(RHS) is equal to node!
            if (!subset_of_any(RHS, *(child->child()))){
               NUMCLOSEDRULES++; 
               if (print_rules)
                  outfile << "100% " << (*n) << " ==> " << *li << " " 
                    << "( " << node->sup() << " " << 1.0 << " )" << endl;
            }
         }   
      }
      
      //Rules with < 100% confidence 
      //if (child->sup() == 0){
      //   cout << "ZERO " << *node << endl;
      //   cout << "ZERO " << *child << endl;         
      //}
      
      double conf = ((double) node->sup()) / child->sup();
      //cout << "CONFMIN " << MINCONF << endl;
      if (conf >= MINCONF){
         //cout << "CONF " << conf << " " << MINCONF << endl;
         for (k = child->mingen()->begin(); k != child->mingen()->end(); ++k){
            lRHS.clear();
            for (n = node->mingen()->begin(); n != node->mingen()->end(); ++n){
               //direct the rules from mingen of k to mingen of n
               NUMCLOSEDEXAMINED++;
               //RHS.clear();
               //RHS must have enough memory for results below
               //RHS.reserve(n->size()); 
               
               diffvec(RHS, (*n), (*k));
               if (!RHS.empty()) lRHS.push_back(RHS);
            }
            
            //sort for checking superset info below
            if (!lRHS.empty()) lRHS.sort(cmpvecint);
            
            //make sure that RHS is not a superset of any other diff
            for (li=lRHS.begin(), ++li; li != lRHS.end(); ){
               //if (superset_of_any((*li), lRHS.begin(), li)){
               if (superset_of_any(li, lRHS.begin(), lRHS.end())){
                  li2 = li;
                  ++li;
                  lRHS.erase(li2);
               }
               else ++li;
            }
            
            //generate rules
            for (li=lRHS.begin(); li != lRHS.end(); ++li){
               if (print_rules)
                  outfile << "<100% " << (*k) << " ==> " << (*li) << " " 
                          << "( " << node->sup() << " " << conf << " )" << endl;
               NUMCLOSEDRULES++;
            }
         }
      }
   }
   //cout << "EXIT NODE " << *node << endl;
}

//carries out a chain of intersection using tmp idlists
int chain_intersect(idlist *l1, idlist *inl2=NULL){
   static bool usetmp1=true;
   static idlist tmp1;
   static idlist tmp2;
   idlist *res, *l2;

   if (inl2){
      //new intersetion
      usetmp1 = true;
      res = &tmp1;
      l2 = inl2;
   }
   else{
      //use previous results
      if (usetmp1){
         res = &tmp1;
         l2 = &tmp2;
      }
      else{
         res = &tmp2;
         l2 = &tmp1;
      }
   }
   res->clear();
   usetmp1 = !usetmp1;

   insert_iterator<idlist> inserter(*res,res->begin());
   set_intersection(l1->begin(), l1->end(),
                    l2->begin(), l2->end(),
                   inserter);

   return res->size();
}

//check if support of mingen v is same as sup
bool check_support(vector<int> &v, int sup)
{
   //v has items with original labels
   //need to convert it to remapped val for freq computation
   idlist tmp;
   
   //initialize tmp to the idlist of v[0]
   int it = Dbase_Ctrl_Blk::FreqMap[v[0]];
   Eqnode *l1 = Dbase_Ctrl_Blk::ParentClass[it];
   int res_sup = l1->sup;
   
   bool first = true;
   for (int i=1; i < v.size(); ++i){
      int it2 = Dbase_Ctrl_Blk::FreqMap[v[i]];
      Eqnode *l2 = Dbase_Ctrl_Blk::ParentClass[it2];
      if (first){
         first= false;
         res_sup = chain_intersect(&l1->tidset,&l2->tidset);
      }
      else res_sup = chain_intersect(&l2->tidset);
   }
   return (res_sup == sup);
}


//Due to constraints, when a closed set is found, some of its closed
//subsets may not be found. Consequently, some of the reported mingens
//may in fact not be the mingens of current node. To verify mingens,
//we compute the support of each mingen. IF the support is equal to
//the sup of the current node, then mingen is valid, else we remove
//the mingen from the list.
void verify_mingens(LatticeNode *node)
{
   list<vector<int> >::iterator n;
   
   for (n = node->mingen()->begin(); n != node->mingen()->end();){
      //if sup of mingen not equal to sup of node remove it
      if (!check_support((*n),node->sup())){
         n = node->mingen()->erase(n);
      }
      else ++n;
   }
}


void reverse_lattice(LatticeNode *node, LatticeNode *oldroot)
{
   if (node == oldroot){
      node->child()->clear();
      node->parent()->clear();
   }
   else{
      list<LatticeNode *> *tmp = node->parent();
      node->parent() = node->child();
      node->child() = tmp;
      
      //remove the oldroot
      if (node->child()->size() == 1 && node->child()->front() == oldroot)
         node->child()->clear();

      if (node->parent()->size() == 0){
         node->parent()->push_back(oldroot);
         oldroot->child()->push_back(node);
      }
   }
}


void generate_rules(LatticeNode *root, vector<LatticeNode *> &lattice)
{
   long marker = 0;
   
   TimeTracker tt;
   tt.Start();
   
   //replace each item with its true label from the db
   for(int i=0; i < lattice.size(); ++i){
      relabel_items(lattice[i]);
   }
   //reset lattice node markers
   //sort all nodes, sort child and parents too!
   sort(lattice.begin(), lattice.end(), LatticeNode::cmpless);
   
   for(int i=0; i < lattice.size(); ++i){
      lattice[i]->flag() = 0;
      lattice[i]->sort();//sort only the node
   }

   for(int i=0; i < lattice.size(); ++i){
      lattice[i]->sort(true);//true flag to sort child,par pointers
   }

   //reverse the whole lattice
   //this is for legacy pruposes, the way mingenerators and rule
   //generation works!!!
   cout << "REVERSING\n";
   
   reverse_lattice(root, root);
   for (int i=0; i < lattice.size(); ++i){
      //lattice[i]->print_node();
      reverse_lattice(lattice[i], root);
   }

   //for(int i=0; i < lattice.size(); ++i){
   //   lattice[i]->print_node();
   //}
   cout << "MINGEN\n";
   
   if (mingen_type == apriori){
      compute_mingenerators(root, ++marker);
   }
   else if (mingen_type == minhitset){
      compute_hs_mingenerators(root, ++marker);
   }
   else {
      cout << "WRONG MINGENTYPE\n";
      exit(-1);
   }
      
   if (check_constraints){
      cout << "VERIFYING MINGENS\n";
      for(int i=0; i < lattice.size(); ++i){
         verify_mingens(lattice[i]);
      }      
   }
   
   mingen_time = tt.Stop();

   if (mingen_only) return; //don't do rule generation
   
   tt.Start();
   
   //LatticeNode::print_lattice(root, ++marker);
   
   cout << "\nMINIMAL RULE GENERATION:\n";
   for(int i=0; i < lattice.size(); ++i){
      //lattice[i]->print_node(true);
      //if (lattice[i]->mingen()->size() > 1) cout << "GREATER " << endl;

      rule_generation(lattice[i]);
   }
   
   rulegen_time = tt.Stop();
   
   outfile << "RULE SUMMARY : (minconf) (num closed rules) (num examined)\n";
   outfile << MINCONF << " " << NUMCLOSEDRULES << " " 
        << NUMCLOSEDEXAMINED << endl;
   
}
