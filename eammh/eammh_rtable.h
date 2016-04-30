#ifndef __eammh_rtable_h__
#define __eammh_rtable_h__

#include <assert.h>
#include <sys/types.h>
#include <config.h>
#include <lib/bsd-list.h>
#include <scheduler.h>

#define CURRENT_TIME Scheduler::instance().clock()

/*
 * EAMMH Neighbor Cache Entry
 */
class EAMMH_Neighbor {
    friend class eammh_rt_entry;
	friend class EAMMH;
	
public:
    EAMMH_Neighbor(u_int32_t a) { nb_addr = a; }

//protected:
    LIST_ENTRY(EAMMH_Neighbor) nb_link;
    nsaddr_t        nb_addr;
};

LIST_HEAD(eammh_ncache, EAMMH_Neighbor);

/*
 * Route Table Entry
 */
class eammh_rt_entry {
        friend class EAMMH;
	
public:
    eammh_rt_entry();
    ~eammh_rt_entry();
	void nb_insert(nsaddr_t id);
    EAMMH_Neighbor* nb_lookup(nsaddr_t id);
	void nb_delete(nsaddr_t id);

//protected:	
	LIST_ENTRY(eammh_rt_entry) rt_link;
	bool shortest_path;	//shortest path (or) alternate paths 
	nsaddr_t next_hop;	//next hop node
	double E_min;		//energy of the minimum energy node along the path
	int min_no_hops;	//number of hops along the shortest path
	double E_avg;		//average energy of the path
	double traffic;		//traffic information along the path
	double heuristic_value;	//heuristic value calculated
	
    eammh_ncache          rt_nblist;	//neighbor list

};

LIST_HEAD(eammh_rthead, eammh_rt_entry);

/*
  The Routing Table
*/
class eammh_rtable {
	friend class EAMMH;

public:
	eammh_rtable() { LIST_INIT(&rthead);} //intializing the routing table (i.e., head of the list)
    eammh_rt_entry* head() { return rthead.lh_first; } //return the head of the list (i.e., the first entry in the routing table)
    eammh_rt_entry* rt_add(nsaddr_t node_id,double e_min,int hop_count,double e_avg,double tra,int budget);	//add new entry to the routing table
    void rt_delete(nsaddr_t node_id); //delete the particular entry from the routing table
    eammh_rt_entry* rt_lookup(void); //lookup the routing table to find the optimum path to reach the cluster head and return the routing table entry
	eammh_rt_entry* rt_lookup(nsaddr_t node_id); //lookup if the particular node is reachable from this node i.e., it is present in the routing table    

	//LIST_HEAD(eammh_rthead, eammh_rt_entry) rthead;	//declare the routing table head
    eammh_rthead rthead;
};

//LIST_HEAD(eammh_rthead, eammh_rt_entry);
#endif /* _eammh__rtable_h__ */
