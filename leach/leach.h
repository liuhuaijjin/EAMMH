
#ifndef ns_leach_h
#define ns_leach_h

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "address.h"
#include "ip.h"
#include "ll.h"
#include <cmu-trace.h>
#include <priqueue.h>
#include <classifier/classifier-port.h>
#include <mobilenode.h>
#include <energy-model.h>


class LEACH;


#define CLUSTER_HEAD_SELECTION  5		// 5s
#define CHANGE_COLOR  		5		// 5s
#define CLUSTER_FORMATION	1		// 1s
#define RECORD_ENERGY_DATA	5		// 5s

/*
  Timers ( Hello, clusterheadselection, change color, clusterformation, routing table, print routing table, record energy data)
*/
class ClusterHeadSelectionTimerLeach : public Handler {
public:
        ClusterHeadSelectionTimerLeach(LEACH* a) : agent(a) {}
        void	handle(Event*);
private:
        LEACH    *agent;
	Event	intr;
};

class ChangeColorTimerLeach : public Handler {
public:
        ChangeColorTimerLeach(LEACH* a) : agent(a) {}
        void	handle(Event*);
private:
        LEACH    *agent;
	Event	intr;
};


class ClusterFormationTimerLeach : public Handler {
public:
        ClusterFormationTimerLeach(LEACH* a) : agent(a) {}
        void	handle(Event*);
private:
        LEACH    *agent;
	Event	intr;
};


class RecordEnergyDataTimerLeach : public Handler {
public:
        RecordEnergyDataTimerLeach(LEACH* a) : agent(a) {}
        void	handle(Event*);
private:
        LEACH    *agent;
	Event	intr;
};



/*
  Routing Agent class
*/


class LEACH : public Agent {

	friend class RecordEnergyDataTimerLeach;
	friend class ChangeColorTimerLeach;
	friend class ClusterFormationTimerLeach;
	friend class ClusterHeadSelectionTimerLeach;

public:
	LEACH(nsaddr_t id);
	LEACH();

	virtual int command(int argc, const char*const* argv);
	virtual void recv(Packet*, Handler*);

	int             initialized() { return 1 && target_; }	

	void		select_cluster_head(void);
	void		is_node_clustered(void);
	void		change_color(void);
	void		record_energy_data(void);


	/*
         * Packet TX Routines
         */

        void            sendPathAdv(void);

        void            sendPathAck(nsaddr_t dest, nsaddr_t src);
	                          
        /*
         * Packet RX Routines
         */
        void		recvLEACH(Packet* p);
        void            recvPathAdv(Packet *p);
        void            recvPathAck(Packet *p);
        void            recvDataTrans(Packet *p);
	
	
	/*
	  LEACH class variables
	*/

	nsaddr_t        index;                  // IP Address of this node
        nsaddr_t	cluster_head;		// Cluster Head
	double		distance_CH;		// distance between Node and CH

        
	double		percentage_CH;			//percentage of cluster heads in the network : a pre-determined factor
	int 		round;			// no of rounds
	bool		is_cluster_head;	// to determine if this node has elected has cluster head
	bool		is_clustered;		// to determine if this node has joined any cluster
	

	/*
         * Timers
         */
        ClusterHeadSelectionTimerLeach  chtimer;
	ChangeColorTimerLeach		cctimer;
	ClusterFormationTimerLeach    cftimer;
	RecordEnergyDataTimerLeach	etimer;


	/*
         * A mechanism for logging the contents of the routing
         * table.
         */
        Trace           *logtarget;
	

	 /*
         * A pointer to the network interface queue that sits
         * between the "classifier" and the "link layer".
         */
        PriQueue        *ifqueue;
        
	/* for passing packets up to agents */
	PortClassifier *dmux_;

	/* mobile node to record node's energy */
	double		iEnergy;
	MobileNode	*iNode;

};

#endif // ns_leach_h
