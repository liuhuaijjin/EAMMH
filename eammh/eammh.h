#ifndef ns_eammh_h
#define ns_eammh_h

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "address.h"
#include "ip.h"
#include "ll.h"
#include <cmu-trace.h>
#include <priqueue.h>
#include <classifier/classifier-port.h>
#include <eammh/eammh_rtable.h>
#include <mobilenode.h>
#include <energy-model.h>

class EAMMH;

// Budget for each cluster
#define BUDGET 40
// Hello interval -- 20s
#define HELLO_INTERVAL 20
// Cluster head selection tick -- 5s
#define CLUSTER_HEAD_SELECTION 5
// Change color tick -- 5s
#define CHANGE_COLOR 5
// Cluster formation delay -- 1s
#define CLUSTER_FORMATION 1
// Routing table clear delay -- 1s
#define ROUTING_TABLE_CLEAR	1
// Routing table print interval -- 1s
#define PRINT_ROUTING_TABLE	1
// Record energy data interval
#define RECORD_ENERGY_DATA 5

/*
  Timers (hello, clusterheadselection, change color, clusterformation, routing table, print routing table, record energy data)
*/
class ClusterHeadSelectionTimerEammh : public Handler {
public:
        ClusterHeadSelectionTimerEammh(EAMMH* a) : agent(a) {}
        void handle(Event*);
private:
        EAMMH *agent;
		Event intr;
};

class ChangeColorTimerEammh : public Handler {
public:
        ChangeColorTimerEammh(EAMMH* a) : agent(a) {}
        void handle(Event*);
private:
        EAMMH *agent;
		Event intr;
};


class HelloTimerEammh : public Handler {
public:
        HelloTimerEammh(EAMMH* a) : agent(a) {}
        void handle(Event*);
private:
        EAMMH *agent;
	    Event intr;
};

class ClusterFormationTimerEammh : public Handler {
public:
        ClusterFormationTimerEammh(EAMMH* a) : agent(a) {}
        void handle(Event*);
private:
        EAMMH *agent;
	    Event intr;
};


class RoutingTableClearTimerEammh : public Handler {
public:
        RoutingTableClearTimerEammh(EAMMH* a) : agent(a) {}
        void handle(Event*);
private:
        EAMMH *agent;
		Event intr;
};

class PrintRoutingTableTimerEammh : public Handler {
public:
        PrintRoutingTableTimerEammh(EAMMH* a) : agent(a) {}
        void handle(Event*);
private:
        EAMMH *agent;
		Event intr;
};

class RecordEnergyDataTimerEammh : public Handler {
public:
        RecordEnergyDataTimerEammh(EAMMH* a) : agent(a) {}
        void handle(Event*);
private:
        EAMMH *agent;
		Event intr;
};



/*
  Routing Agent class
*/
class EAMMH : public Agent {

	friend class eammh_rt_entry;
	friend class eammh_rtable;
    friend class HelloTimerEammh;
    friend class NeighborTimerEammh;
    friend class RouteCacheTimerEammh;
	friend class ClusterTimerEammh;
	friend class RecordEnergyDataTimerEammh;
	friend class PrintRoutingTableTimerEammh;
	friend class ChangeColorTimerEammh;

public:
	EAMMH(nsaddr_t id);
	EAMMH();

	virtual int command(int argc, const char*const* argv);
	virtual void recv(Packet*, Handler*);

	int initialized() { return 1 && target_; }	

	/*
     * Neighbor Management
     */
    void nb_insert(nsaddr_t id);
    EAMMH_Neighbor* nb_lookup(nsaddr_t id);
    void nb_delete(nsaddr_t id);
	void nb_print(void);
    //void nb_purge(void);
	bool in_range(double s_x,double s_y,double s_z,double d_x,double d_y,double d_z);
	void print_rt(void);
	void select_cluster_head(void);
	void is_node_clustered(void);
	void change_color(void);
	void record_energy_data(void);


	/*
     * Packet TX Routines
     */

	void sendHello();
    void sendPathAdv(int budget);
    void sendPathAck(nsaddr_t dest, nsaddr_t src, bool alt_path);

    /*
     * Packet RX Routines
     */
    void recvEAMMH(Packet* p);
    void recvHello(Packet *p);
    void recvPathAdv(Packet *p);
    void recvPathAck(Packet *p);
    void recvDataTrans(Packet *p);
	
	/*
	 * Routing Routines
	 */
    eammh_rt_entry* head() { return rthead.lh_first; } //return the head of the list (i.e., the first entry in the routing table)
    eammh_rt_entry* rt_add(nsaddr_t node_id,double e_min,int hop_count,double e_avg,double tra,int budget);	//add new entry to the routing table
    void rt_delete(nsaddr_t node_id); //delete the particular entry from the routing table
    eammh_rt_entry*	rt_lookup(void); //lookup the routing table to find the optimum path to reach the cluster head and return the routing table entry
	eammh_rt_entry*	rt_search(nsaddr_t node_id); //lookup if the particular node is reachable from this node i.e., it is present in the routing table
	void rt_clear(void); //clear the routing table

	/*
	 * EAMMH class variables
	 */
	nsaddr_t index; // IP Address of this node
    nsaddr_t cluster_head; // Cluster Head
    eammh_ncache nbhead; // Neighbor Cache
	double percentage_CH; //percentage of cluster heads in the network : a pre-determined factor
	int round; // no of rounds
	bool is_cluster_head; // to determine if this node has elected has cluster head
	bool is_clustered; // to determine if this node has joined any cluster
	

	/*
     * Timers
     */
    ClusterHeadSelectionTimerEammh chtimer;
	ChangeColorTimerEammh cctimer;
    HelloTimerEammh htimer;
    RoutingTableClearTimerEammh	rtimer;
	ClusterFormationTimerEammh cftimer;
	PrintRoutingTableTimerEammh prtimer;
	RecordEnergyDataTimerEammh etimer;

    /*
     * Routing Table
     */
	// routing table-head
	eammh_rthead rthead;

	/*
     * A mechanism for logging the contents of the routing
     * table.
     */
    Trace *logtarget;
	
	 /*
      * A pointer to the network interface queue that sits
      * between the "classifier" and the "link layer".
      */
    PriQueue *ifqueue;
        
	/* for passing packets up to agents */
	PortClassifier *dmux_;

	/* mobile node to record node's energy */
	double iEnergy;
	MobileNode *iNode;
};

#endif // ns_eammh_h
