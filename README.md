# CHARM-L Algorithm and Lattice and Rule Generation

Charm mines all the frequent closed itemsets as described in [2002-charm]. 
Charm-L adds the ability to construct the entire frequent concept lattice (also called as the '''iceberg lattice'''), that is, 
it adds the links 
between all sub/super-concepts (or closed itemsets) [2005-charm:tkde]. 
This ability is used to mine the non-redundant association rules [2004-nonredundant:dmkd].

[2002-charm] Mohammed J. Zaki and Ching-Jui Hsiao. CHARM: an efficient algorithm for closed itemset mining. In 2nd SIAM International Conference on Data Mining. April 2002.

[2005-charm:tkde] Mohammed J. Zaki and Ching-Jui Hsiao. Efficient algorithms for mining closed itemsets and their lattice structure. IEEE Transactions on Knowledge and Data Engineering, 17(4):462–478, April 2005. doi:10.1109/69.846291.

[2004-nonredundant:dmkd] Mohammed J. Zaki. Mining non-redundant association rules. Data Mining and Knowledge Discovery: An International Journal, 9(3):223–248, November 2004.

You will also need access to the utils files: https://github.com/zakimjz/tposedb


# HOW TO

1) two types of datasets are supported

    - ibm format (cid tid #items itemset)

    Here each line contains the 
	   
    customerid, transid, number of items, and list of items

   - basic ascii format (itemset) 
   
        Here each line is just a list of items

  only integer datasets are supported

  two example test databases are included in the dir

  	    testdb_ibm.ascii (in ibm format)
	    testdb.ascii (non-ibm format)

2) type charm to see the list of options supported

  a basic run of charm is as follows:

	charm -i <DB> -o <PAT_OUT> -s <MINSUP> -r <MINCONF> 
	
	this will mine the database file <DB>
	it will output the closed freq patterns to file <PAT_OUT>
	it will use MINSUP as the relative minsup (e.g. 0.5 is 50%)
	it will use MINCONF as the minimum conf for the rules
	both MINSUP and MINCONF must be between 0 and 1.
	the rules are printed to stdout if -R flag is given.
	
        EXAMPLE: charm -i testdb_ibm.ascii -o patout -s 0.5 -r 0.75 -R

	some example rules are:
	100% 2  ==> 1  ( 4 1 )
	explanation: this is a 100% conf rule, the rule is 2 ==> 1,
		the rule support is 4, and the rule conf is 1 (100%)
	<100% 1  ==> 4  ( 5 0.833333 )
	explanation: this is a <100% conf rule, the rule is 1 ==> 4,
		the rule support is 5, and the rule conf is 0.83 (83.3%)

 TO RUN NON_IBM DB without offset:

	just add the -x flag (for non-ibm datasets)
	EXAMPLE: charm -x -i testdb.ascii -o patout -s 0.5 -r 0.75

# Using charm to generate riles with constraints

First try running a dataset with high values of support to get a feel
for what the output is like, without generating the rules.

For example: charm -i G1.asc -S 5 -C G1.cons -o COUT

Here G1.asc is a gene file (each line denotes a property of one gene)

-S 5 means mine patterns with at least minsup of 5

-o COUT means print to stdout, you can also give an output file name

-C G1.cons means that G1.cons is a contraints file

    //read the constraints, one per line
    //if first field is C then it is an col/attribute constraint
    //if first field is R then it is a row constraint
    //items on each line are considered to be conjunctions
    //different lines are considered as disjunctions
    //each line is assumed to be sorted in increasing order	
    // the semantics are that each pattern must satisfy at least one 
    //row constraint and one column constraint.

an example constraints file is shown for G1.asc datafile

Here only those items 'C' specified are the ones
whose extensions will be reported, and at least those genes specified
in 'R' must be in each closed sets geneset.


Now say you want to see the gene ids also, which support each itemset, 
then run  it as:

    charm -i G1.asc -S 5 -C G1.cons -o COUT -d 0 -l

The -l prints the geneids, and -d 0 turns off diffsets, so that each
list printed in [ ] after an itemset is indeed a tidset of gene ids.

Finally to print the self rules, run it as: 

    charm -i G1.asc -S 5 -C G1.cons -o COUT -d 0 -l -E -R -r 1.0

-r 1.0 means mine rules with min conf 1.0 -R means print rules -E
means print only self rules

This will print out the rules, e.g.  

    SELF 24 <==> 1626 ( 8 1 ) 
    
which means that 8 genes support it and conf is 1

to see which 8 genes support it, just go to the list of closed sets
and see which one has support 8 and contains both 24 and 1626. The
geneset of that closed set is the same as the geneset for this rule.

BTW, there is also one other option to restrict the number of closed
rules.  

    try -Z <maxlen> 
    
to not produce any new closed set with length
more than maxlen.  note that this will still report a long closed set,
if for example a set smaller than maxlen actually corresponds to,
i.e., is equivalent to the longer closed set.

Finally, you can store the mined rules in a file, say foo 

    charm -i G1.asc -S 5 -C G1.cons -o COUT -d 0 -l -E -R -r 1.0 > foo

or

    charm -i G1.asc -S 5 -C G1.cons -o foo -d 0 -l -E -R -r 1.0

then you can run "remap.pl foo G1.labels" to convert each rule into a
readable format.

Finally, you can turn on the -F option to generate conditional rules

and -L option to print the lattice
	
