#ifndef __eclat_H
#define __eclat_H

#include <vector>
#include <iostream>

using namespace std;

typedef vector<int> idlist;

//diffset types: diffin - use input db in diff format, diff2 - use diff for 
//F2, diff - use diff after F2, nodiff - do not use diff
enum diff_vals {nodiff, diff, diff2, diffin};
enum sort_vals {nosort, incr, decr, incr_noclass};
enum subset_vals {equals, subset, superset, notequal};
enum alg_vals {eclat, charm, basicmax, maxcharm, colex};
enum prune_vals {noprune, prune}; //do candidate pruning?
enum mingen_vals {minhitset, apriori}; //which mingen alg to use

//cnone - do not eliminate non-closed
//chash - use hash based technique, cmax -- use intersections
enum closed_vals {cnone, chash, cmax};


//externs vars
extern double MINSUP_PER;
extern int MINSUPPORT;
extern int DBASE_MAXITEM;
extern int DBASE_NUM_TRANS;
extern int max_closed_len; //max length of closed itemset
extern bool output;
extern bool output_idlist;
extern bool self_rules_only;
extern bool conditional_self_rules;
extern bool mingen_only;
extern bool check_constraints;


extern diff_vals diff_type;
extern diff_vals max_diff_type;
extern sort_vals sort_type;
extern alg_vals alg_type;
extern closed_vals closed_type;
extern prune_vals prune_type;   
extern mingen_vals mingen_type;
extern ofstream outfile;

extern ostream & operator<<(ostream& fout, idlist &vec);
extern subset_vals get_intersect(idlist *l1, idlist *l2, 
                                 idlist *join, int &idsum, int minsup=0);
#endif
