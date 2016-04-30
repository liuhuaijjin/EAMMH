#include "eammh/eammh.h"
#include "eammh/eammh_packet.h"
#include "stdlib.h"
#include "god.h"
#include "math.h"
#include <random.h>
#include <cmu-trace.h>

#define CURRENT_TIME Scheduler::instance().clock()

#define RT_PORT_1 255

int hdr_eammh::offset_;
static class EAMMHHeaderClass : public PacketHeaderClass {
public:
	EAMMHHeaderClass() : PacketHeaderClass("PacketHeader/EAMMH", 
					      sizeof(hdr_all_eammh)) {
		bind_offset(&hdr_eammh::offset_);
	}
} class_rtProtoEAMMH_hdr;

static class EAMMHClass : public TclClass {
public:
	EAMMHClass() : TclClass("Agent/EAMMH") {}
	TclObject* create(int argc, const char*const* argv) {
          assert(argc == 5);
          return (new EAMMH((nsaddr_t) Address::instance().str2addr(argv[4])));
    }
} class_rtProtoEAMMH;

/* 
 * Constructor
 */
EAMMH::EAMMH(nsaddr_t id) : Agent(PT_EAMMH),
			  chtimer(this), htimer(this), rtimer(this),  cftimer(this), prtimer(this), etimer(this), cctimer(this) {
	index = id;
	MobileNode	*iNode;
	iEnergy = 0.0;
	ifqueue = 0;
	logtarget = 0;
	percentage_CH = 0.04;
	round = (int)(1 / percentage_CH) + 1;
	is_cluster_head = false;
	is_clustered = false;
	LIST_INIT(&nbhead);
	LIST_INIT(&rthead);
}

EAMMH::EAMMH() : Agent(PT_EAMMH),
			  chtimer(this), htimer(this), rtimer(this),  cftimer(this), prtimer(this), etimer(this), cctimer(this) {       
	MobileNode *iNode;
	iEnergy = 0.0;
	ifqueue = 0;
	logtarget = 0;
	percentage_CH = 0.04;
	round = (int)(1 / percentage_CH) + 1;
	is_cluster_head = false;
	is_clustered = false;
	LIST_INIT(&nbhead);
	LIST_INIT(&rthead);
}

int EAMMH::command(int argc, const char*const* argv)
{

    Tcl& tcl = Tcl::instance();

    if(argc == 2) {    
		if(strncasecmp(argv[1], "id", 2) == 0) {
			tcl.resultf("%d", index);
			return TCL_OK;
		} else if(strncasecmp(argv[1], "start", 2) == 0) {
			sendHello();
			return TCL_OK;
		} else if(strcmp(argv[1],"cluster_formation") == 0) {
			cftimer.handle((Event*) 0);
			return TCL_OK;
		} else if(strcmp(argv[1],"print_neighbors") == 0) {
			nb_print();
			return TCL_OK;
		} else if(strcmp(argv[1],"print_rt") == 0) {
			prtimer.handle((Event*) 0);	
			return TCL_OK;
		} else if(strcmp(argv[1],"select_cluster_heads") == 0) {
			chtimer.handle((Event*) 0);	
			return TCL_OK;
		} else if(strcmp(argv[1],"print_is_clustered") == 0) {
			is_node_clustered();
			return TCL_OK;
		} else if(strcmp(argv[1],"change_color") == 0) {
			cctimer.handle((Event*) 0);	
			return TCL_OK;
		} else if(strcmp(argv[1],"routing_table_clear") == 0) {
			rtimer.handle((Event*) 0);	
			return TCL_OK;
		} else if(strcmp(argv[1],"record_energy_data") == 0) {
			etimer.handle((Event*) 0);
			return TCL_OK;
		}
	}
	else if(argc == 3) {
		if(strcmp(argv[1], "index") == 0) {
			index = atoi(argv[2]);
			return TCL_OK;
		} else if(strcmp(argv[1], "log-target") == 0 || strcmp(argv[1], "tracetarget") == 0) {
			logtarget = (Trace*) TclObject::lookup(argv[2]);
			if(logtarget == 0)
				return TCL_ERROR;
			return TCL_OK;
		} else if(strcmp(argv[1], "if-queue") == 0) {
			ifqueue = (PriQueue*) TclObject::lookup(argv[2]);
			if(ifqueue == 0)
				return TCL_ERROR;
			return TCL_OK;
		} else if (strcmp(argv[1], "port-dmux") == 0) {
			dmux_ = (PortClassifier *)TclObject::lookup(argv[2]);
			if (dmux_ == 0) {
				fprintf (stderr, "%s: %s lookup of %s failed\n", __FILE__,
				argv[1], argv[2]);
				return TCL_ERROR;
			}
			return TCL_OK;
		}
	}
  
	// If the command hasn't been processed by EammhAgent()::command,
	// call the command() function for the base class
	return (Agent::command(argc, argv));
}



/*
 * Timers
 */
void
ClusterHeadSelectionTimerEammh::handle(Event*) {
	agent->select_cluster_head();
	Scheduler::instance().schedule(this, &intr, CLUSTER_HEAD_SELECTION);
}

void
ChangeColorTimerEammh::handle(Event*) {
	agent->change_color();
	Scheduler::instance().schedule(this, &intr, CHANGE_COLOR);
}

void
HelloTimerEammh::handle(Event*e) {
	agent->sendHello();
	Scheduler::instance().schedule(this, &intr, HELLO_INTERVAL);
}

void
ClusterFormationTimerEammh::handle(Event*) {   
	if(agent->is_cluster_head)
		agent->sendPathAdv(BUDGET);
	Scheduler::instance().schedule(this, &intr,CLUSTER_FORMATION); 
}


void
RoutingTableClearTimerEammh::handle(Event*) {
	agent->rt_clear();
	Scheduler::instance().schedule(this, &intr, ROUTING_TABLE_CLEAR); 
}

void
PrintRoutingTableTimerEammh::handle(Event*) {
	agent->print_rt();
	Scheduler::instance().schedule(this, &intr, PRINT_ROUTING_TABLE); 
}


void
RecordEnergyDataTimerEammh::handle(Event*) {
	agent->record_energy_data();
	Scheduler::instance().schedule(this, &intr, RECORD_ENERGY_DATA); 
}

/*
 * Packet Reception Routines
 */
void
EAMMH::recv(Packet *p, Handler*) {
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	iNode = (MobileNode*) (Node::get_node_by_address(index));

	if(index != 100)
		iNode->energy_model()->DecrTxEnergy(0.001198,1);

	if(ch->ptype() == PT_EAMMH && ch->direction() == hdr_cmn::UP && index!=100) {
   		recvEAMMH(p);
   		return;
	}
	else {
		recvDataTrans(p);
	}
}


void
EAMMH::recvEAMMH(Packet *p) {
	struct hdr_eammh *eh = HDR_EAMMH(p);
	assert(HDR_IP (p)->sport() == RT_PORT_1);
	assert(HDR_IP (p)->dport() == RT_PORT_1);

	/*
	* Incoming Packets.
	*/
	switch(eh->eh_type) {
		case EAMMHTYPE_PATH_ADV: 
			recvPathAdv(p);
			break;

		case EAMMHTYPE_PATH_ACK: 
			recvPathAck(p);
			break;

		case EAMMHTYPE_HELLO: 
			recvHello(p);
			break;
        
		default:
			printf("nothing\n");
			exit(1);
	}
}


void
EAMMH::recvPathAdv(Packet *p) {
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_eammh *eh = HDR_EAMMH(p);
	struct hdr_eammh_path_adv *adv = HDR_EAMMH_PATH_ADV(p);
	eammh_rt_entry *rt;
	int budget = 0;
	bool fwd = false;
	nsaddr_t source_node;

	iNode = (MobileNode*) (Node::get_node_by_address(index));

	/*
	 * Drop if:
	 *      - I'm the source.
	 *      - I'm not a neighbor.
	 *	  - I'm the CH.
	 */

	if(adv->adv_src_node == index) {
		Packet::free(p);
		return;
	} 

	if(adv->neighbor_node != index) {
		drop(p,0);
		return;
	}

	if(is_cluster_head == true) {
		Packet::free(p);
		return;   
	}

	source_node = adv->adv_src_node;
	budget = adv->budget_alloted;
 
	/* 
	* We are either going to forward the REQUEST or generate a
	* REPLY. Before we do anything, we make sure that the REVERSE
	* route is in the route table.
	*/
	//recv 												------- something happened here !!!
	eammh_rt_entry *rt0; // rt0 is the reverse route 
   
	rt0 = rt_search(adv->adv_src_node);

	if(rt0 == 0) { /* if not in the route table */
		// create an entry for the reverse route.
		rt0 = rt_add(adv->adv_src_node,adv->min_energy,adv->no_of_hops,
     				adv->avg_energy,adv->traffic_info,budget);

		if(rt0->shortest_path == true) {
			budget--; // if it is the shortest path
			if(budget > 0)
				fwd = true;
		}
    }


	if(rt0)
		is_clustered = true;

	/*
	* send Path Ack
	*/
	if(rt0->shortest_path == true) {
		sendPathAck(adv->adv_src_node,index,false);
	} else {
		sendPathAck(adv->adv_src_node,index,true);
	}

	int hop_count = adv->no_of_hops;
	double eavg = adv->avg_energy;
	double emin = adv->min_energy;
	double tra = adv->traffic_info;

	Packet::free(p);
	/*
	 * We have taken care of the routing table entry stuff.
	 * Now see whether we should forward the path_adv. 
	 */
	/*
	  Forward the pathAdv
	*/
	if ( fwd == true ) {
		iNode = (MobileNode*) (Node::get_node_by_address(index));
		iEnergy = iNode->energy_model()->energy();
	
		double energy_node = iEnergy;
		int j=0;
		int count=0;

		EAMMH_Neighbor *nbc = nbhead.lh_first;
		for(; nbc; nbc = nbc->nb_link.le_next) {
			if(nbc->nb_addr != source_node)
				count++;
		}

		if(index!=100) {
			if(count) {
				nsaddr_t neighbor_list[10] = {0,0,0,0,0,0,0,0,0,0};
				int32_t budget_list[10] = {0,0,0,0,0,0,0,0,0,0};
				EAMMH_Neighbor *nb_1 = nbhead.lh_first;
 				int i=0;
				while(budget>0) {
					i=0;
					nb_1 = nbhead.lh_first;
 					for(; nb_1 && i<10; nb_1 = nb_1->nb_link.le_next) {			
						neighbor_list[i] = nb_1->nb_addr;
						budget_list[i] = budget_list[i]+1;	
						budget = budget-1;			
						if(budget == 0)
							break;
						i++;
					}
				}

				Packet *p[count];
 				EAMMH_Neighbor *nb = nbhead.lh_first;

 				for(; nb; nb = nb->nb_link.le_next)	{
					p[j] = Packet::alloc();
					struct hdr_cmn *ch = HDR_CMN(p[j]);
					struct hdr_ip *ih = HDR_IP(p[j]);
					struct hdr_eammh *eh = HDR_EAMMH(p[j]);
					struct hdr_eammh_path_adv *adv = HDR_EAMMH_PATH_ADV(p[j]);
	
					eh->eh_type = EAMMHTYPE_PATH_ADV;

					adv->adv_src_node = index;
					adv->no_of_hops = hop_count + 1;
					adv->avg_energy = (((eavg * hop_count) + energy_node) / (hop_count+1));
					adv->traffic_info = 1.0;
		
					if(energy_node < emin)
						adv->min_energy = energy_node;	
					else
						adv->min_energy = emin;

					adv->neighbor_node = nb->nb_addr;
					bool flag = false;
					int m=0;
					for(m=0;m<10;m++) {
						if(neighbor_list[m] == nb->nb_addr) {
							flag= true;
							break;
						}
					}
					if(flag == true)
						adv->budget_alloted = budget_list[m];
					else
						adv->budget_alloted = 0;
					flag = false;									

					ch->size() = IP_HDR_LEN + adv->size();
 					ch->ptype() = PT_EAMMH;
 					ch->iface() = -2;
 					ch->addr_type() = NS_AF_INET;
 					ch->prev_hop_ = index;          // eammh hack
					ch->direction() = hdr_cmn::DOWN;
					ch->next_hop_ = nb->nb_addr;

 					ih->saddr() = index;
 					ih->daddr() = nb->nb_addr;
 					ih->sport() = RT_PORT_1;
 					ih->dport() = RT_PORT_1;	
		
					if(nb->nb_addr != source_node)
						Scheduler::instance().schedule(target_, p[j], 0.0);
					iNode->energy_model()->DecrTxEnergy(0.001597,1);
					j++;
 				}
			}
		}
	}
	return;
}

void
EAMMH::recvPathAck(Packet *p) {
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_eammh_path_ack *ack = HDR_EAMMH_PATH_ACK(p);

	/*
	 * for now do nothing
	 */
	Packet::free(p);
}

void
EAMMH::recvDataTrans(Packet *p) {
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_cmn *ch = HDR_CMN(p);
	eammh_rt_entry *rt;

	iNode = (MobileNode*) (Node::get_node_by_address(index));
	iEnergy = iNode->energy_model()->energy();
	rt = rt_lookup();

	if(!is_cluster_head) {
		if(rt == 0) {
			drop(p,0);
		} else {
			ch->iface() = -2;
			ch->error() = 0;
			ch->addr_type() = NS_AF_INET;
			ch->prev_hop_ = index;          // EAMMH hack
			ch->next_hop_ = rt->next_hop;
			ch->direction() = hdr_cmn::DOWN ;	//change direction before forwarding : v imp
	
			ih->saddr() = index;
			ih->daddr() = rt->next_hop;		
			ih->sport() = RT_PORT_1;
			ih->dport() = RT_PORT_1;
			ih->ttl_ = 1;
	 
			iNode->energy_model()->DecrTxEnergy(0.001597,1);
			Scheduler::instance().schedule(target_, p, 0.0);
		}
	} else if(index!=100) {
		ch->iface() = -2;
		ch->error() = 0;
		ch->addr_type() = NS_AF_INET;
		ch->prev_hop_ = index;          // EAMMH hack
		ch->next_hop_ = (nsaddr_t)100;		
		ch->direction() = hdr_cmn::DOWN ;	//change direction before forwarding : v imp
	
		ih->saddr() = index;
		ih->daddr() = (nsaddr_t)100;		
		ih->sport() = RT_PORT_1;
		ih->dport() = RT_PORT_1;
		ih->ttl_ = 1;
	 
		iNode->energy_model()->DecrTxEnergy(0.01597,1);
		Scheduler::instance().schedule(target_, p, 0.0);
	} else {
		Packet::free(p);
	}
}

/*
 * Packet Transmission Routines
 */
void
EAMMH::sendPathAdv(int budget) {
	/* 
	 * Initial state of the implementation when the cluster head broadcasts the path adv pkt
	 * function i/p parameters shld include Emin , Eavg etc
	 */
    // Allocate a Path ADV packet 

	EAMMH_Neighbor *nb = nbhead.lh_first;
	int count =0;
	int j=0;
	budget--;
	FILE *fp = fopen("test_trace","a");

	fprintf(fp,"\n node is a cluster head : %d\n",(int)index);

	if(index!=100) {
		EAMMH_Neighbor *nbc = nbhead.lh_first;
		for(; nbc; nbc = nbc->nb_link.le_next) {
			count++;
		}

		if(count) {
			nsaddr_t neighbor_list[10] = {0,0,0,0,0,0,0,0,0,0};
			int32_t budget_list[10] = {0,0,0,0,0,0,0,0,0,0};
			EAMMH_Neighbor *nb_1 = nbhead.lh_first;
 			int i=0;
			while(budget>0)	{
				i=0;
				nb_1 = nbhead.lh_first;
 				for(; nb_1 && i<10; nb_1 = nb_1->nb_link.le_next) {			
					neighbor_list[i] = nb_1->nb_addr;
					budget_list[i] = budget_list[i]+1;	
					budget = budget-1;		
					if(budget == 0)
						break;
					i++;
				}
			}
	
			Packet *p[count];
 			nb = nbhead.lh_first;
		
			for(; nb; nb = nb->nb_link.le_next) {
				p[j] = Packet::alloc();
				struct hdr_cmn *ch = HDR_CMN(p[j]);
				struct hdr_ip *ih = HDR_IP(p[j]);
				struct hdr_eammh *eh = HDR_EAMMH(p[j]);
				struct hdr_eammh_path_adv *adv = HDR_EAMMH_PATH_ADV(p[j]);
	
				eh->eh_type = EAMMHTYPE_PATH_ADV;

				iNode = (MobileNode*) (Node::get_node_by_address(index));
				iEnergy = iNode->energy_model()->energy();

				adv->adv_src_node = index;
				adv->no_of_hops = 1;
				adv->min_energy = iEnergy;
				adv->avg_energy = iEnergy;
				adv->traffic_info = 1.0;

				adv->neighbor_node = nb->nb_addr;
				bool flag = false;
				int k=0;
				for(k=0;k<10;k++){
					if(neighbor_list[k] == nb->nb_addr) {
						flag= true;
						break;
					}			
				}

				if(flag == true)
					adv->budget_alloted = budget_list[k];
				else
					adv->budget_alloted = 0;
				flag = false;										

				ch->size() = IP_HDR_LEN + adv->size();
 				ch->ptype() = PT_EAMMH;
 				ch->iface() = -2;
 				ch->addr_type() = NS_AF_INET;
 				ch->prev_hop_ = index;          // eammh hack
				ch->direction() = hdr_cmn::DOWN;
				ch->next_hop_ = nb->nb_addr;

 				ih->saddr() = index;
 				ih->daddr() = nb->nb_addr;
 				ih->sport() = RT_PORT_1;
 				ih->dport() = RT_PORT_1;	
		
				Scheduler::instance().schedule(target_, p[j], 0.0);
				iNode->energy_model()->DecrTxEnergy(0.001597,1);

				j++;
 			}
		}
	}
	fclose(fp);
}

void
EAMMH::sendPathAck(nsaddr_t dest, nsaddr_t src, bool alt_path) {
Packet *p = Packet::alloc();
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_eammh_path_ack *ack = HDR_EAMMH_PATH_ACK(p);

if(index!=100)
{
 ack->ack_type = EAMMHTYPE_PATH_ACK;
 ack->ack_src_node = src;
 ack->ack_dest_node = dest;
 ack->alternate_path = alt_path;
   
 ch->ptype() = PT_EAMMH;
 ch->iface() = -2;
 ch->addr_type() = NS_AF_INET;
 ch->next_hop_ = dest;
 ch->prev_hop_ = index;          // EAMMH hack
 ch->direction() = hdr_cmn::DOWN;
 ch->error() = 0;

 ih->saddr() = index;
 ih->daddr() = dest;
 ih->sport() = RT_PORT_1;
 ih->dport() = RT_PORT_1;

 Scheduler::instance().schedule(target_, p, 0.);

iNode->energy_model()->DecrTxEnergy(0.001597,1);
}
return;
}


/*
   Neighbor Management Functions
*/

void
EAMMH::sendHello() {
	Packet *p = Packet::alloc();
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_eammh_hello *he = HDR_EAMMH_HELLO(p);

	iNode = (MobileNode*) (Node::get_node_by_address(index));
	iNode->energy_model()->setenergy(30.0);

	double x_ = iNode->X();
	double y_ = iNode->Y();
	double z_ = iNode->Z();

	if(index!=100) {
		he->hel_type = EAMMHTYPE_HELLO;
		he->hel_src = index;
		he->x_pos = x_;
		he->y_pos = y_;
		he->z_pos = z_;

		ch->ptype() = PT_EAMMH;
		ch->size() = IP_HDR_LEN + he->size();
		ch->iface() = -2;
		ch->error() = 0;
		ch->addr_type() = NS_AF_NONE;
		ch->prev_hop_ = index;          // EAMMH hack
		ch->next_hop_ = IP_BROADCAST;		

		ih->saddr() = index;
		ih->daddr() = IP_BROADCAST;		
		ih->sport() = RT_PORT_1;
		ih->dport() = RT_PORT_1;
		ih->ttl_ = 1;

		Scheduler::instance().schedule(target_, p, 0.0);
		iNode->energy_model()->DecrTxEnergy(0.01597,1);
	}
	return; 
}

bool
EAMMH::in_range(double s_x,double s_y,double s_z,double d_x,double d_y,double d_z) {
	double dist;
	dist = sqrt(((d_x-s_x)*(d_x-s_x)) +
			((d_y-s_y)*(d_y-s_y)) +
			((d_z-s_z)*(d_z-s_z)));
	if(dist<=50.0)
		return true;
	else 
		return false;
}

void
EAMMH::recvHello(Packet *p) {
	struct hdr_eammh_hello *hel = HDR_EAMMH_HELLO(p);
	EAMMH_Neighbor *nb;

	iNode = (MobileNode*) (Node::get_node_by_address(index));
	double x_ = iNode->X();
	double y_ = iNode->Y();
	double z_ = iNode->Z();

	if(index!=100) {
		nb = nb_lookup(hel->hel_src);
		if(nb == 0) {   
			bool range;
			range = in_range(x_,y_,z_,hel->x_pos,hel->y_pos,hel->z_pos);

			if(range)   
				nb_insert(hel->hel_src);
		}
	}
	Packet::free(p);
}

void
EAMMH::nb_insert(nsaddr_t id) {
	EAMMH_Neighbor *nb = new EAMMH_Neighbor(id);
	assert(nb);

	LIST_INSERT_HEAD(&nbhead, nb, nb_link);
}

EAMMH_Neighbor*
EAMMH::nb_lookup(nsaddr_t id) {
	EAMMH_Neighbor *nb = nbhead.lh_first;

	for(; nb; nb = nb->nb_link.le_next) {
		if(nb->nb_addr == id) 
			break;
	}
	return nb;
}

void
EAMMH::nb_print(void) {
	EAMMH_Neighbor *nb = nbhead.lh_first;
	for(; nb; nb = nb->nb_link.le_next) {
		printf("\nNode %d 's neighbors : %d\n",index,nb->nb_addr);
	}
}


/*
 * Called when we receive *explicit* notification that a Neighbor
 * is no longer reachable.
 */
void
EAMMH::nb_delete(nsaddr_t id) {
	EAMMH_Neighbor *nb = nbhead.lh_first;

	for(; nb; nb = nb->nb_link.le_next) {
		if(nb->nb_addr == id) {
			LIST_REMOVE(nb,nb_link);
			delete nb;
			break;
		}
	}
}

/*
 * The Routing Table
 */
eammh_rt_entry*
EAMMH::rt_lookup() {
	eammh_rt_entry *rt = rthead.lh_first;
	eammh_rt_entry *rt_h;
	double heur_value=0.0;

 	for(; rt; rt = rt->rt_link.le_next) {
  		if(rt->heuristic_value > heur_value) {
			if(rt->E_min >= (rt->E_avg/10.0)) {
				heur_value = rt->heuristic_value;
				rt_h = rt;
			}
		}
 	}

 	return rt_h;
}

eammh_rt_entry*
EAMMH::rt_search(nsaddr_t node_id)
{
	eammh_rt_entry *rt = rthead.lh_first;

	for(; rt; rt = rt->rt_link.le_next) {
		if(rt->next_hop == node_id)
			break;
	}
	return rt;
}

void
EAMMH::rt_clear(void)
{
	eammh_rt_entry *rt = rthead.lh_first;

	for(; rt; rt = rt->rt_link.le_next) {
		LIST_REMOVE(rt, rt_link);
	}
	rthead.lh_first == NULL;

}


void
EAMMH::rt_delete(nsaddr_t node_id)
{
	eammh_rt_entry *rt = rt_search(node_id);

	if(rt) {
		LIST_REMOVE(rt, rt_link);
		delete rt;
	}
}

eammh_rt_entry*
EAMMH::rt_add(nsaddr_t node_id,double e_min,int hop_count,double e_avg,double tra,int budget)
{
	eammh_rt_entry *rt;
	eammh_rt_entry *rt_head = rthead.lh_first;
	bool alternate_path = true;
	double heur_value;
		 	
	rt = new eammh_rt_entry;
 	if( budget > 0) {
		if(rt_head == 0) {
			alternate_path = false;
		} else {
			alternate_path = false;
			eammh_rt_entry *rt_p = rthead.lh_first;

			for(; rt_p; rt_p = rt_p->rt_link.le_next) {
  				if(rt_p->shortest_path == true) {
					alternate_path = true;
  					break;
				}
 			}
		}
	}	

	heur_value = e_avg / (hop_count * tra);
	
	if(alternate_path == false)
	 	rt->shortest_path = true;
	else
	 	rt->shortest_path = false;		
	rt->next_hop = node_id;
	rt->E_min = e_min;
	rt->min_no_hops = hop_count;
	rt->E_avg = e_avg;
	rt->traffic = tra;
	rt->heuristic_value = heur_value;	
	
 	LIST_INSERT_HEAD(&rthead, rt, rt_link);

 	return rt;
}

void
EAMMH::print_rt(void) {
	FILE *fp=fopen("test_trace","a");
	eammh_rt_entry *rt = rthead.lh_first;

	for(; rt; rt = rt->rt_link.le_next) {
		fprintf(fp,"node : %d shortest : %d nexthop : %d emin : %f eavg : %f tra : %f heuristic : %f\n",(int)index,rt->shortest_path,
		(int)rt->next_hop,rt->E_min,rt->E_avg,rt->traffic,rt->heuristic_value);   
	}
	fclose(fp);
}


void 
EAMMH::select_cluster_head(void) {
	double threshold;
	threshold = ((percentage_CH) / (1 - (percentage_CH * (round % ((int)(1/percentage_CH))))));
	double t_check;

	t_check = rand() % 1000;
	t_check = t_check / 1000;

	if(t_check < threshold && round > (int)(1 / percentage_CH)) {
		is_cluster_head = true;
		round = 1;
	} else {
		is_cluster_head = false;
		round++;
	}
}

void
EAMMH::is_node_clustered(void) {
	if(is_clustered == false)
		printf("\n node %d is not clustered",(int)index);
}

void
EAMMH::change_color(void) {
	Tcl& tcl = Tcl::instance();
	char x[128];
	char y[128];
	char z[128];
	sprintf(x, "$node_(%d) color red\n",index);
	sprintf(y, "$ns at %f \"$node_(%d) color red\"\n",CURRENT_TIME,index);
	sprintf(z, "$ns at %f \"$node_(%d) label cluster_head\"\n",CURRENT_TIME,index);

	if(is_cluster_head)
	{
		//tcl.eval(x);		
		tcl.eval(y);		
		tcl.eval(z);		
	}
}

void
EAMMH::record_energy_data(void)
{
	FILE *fp = fopen("energy_trace.tr","a");

	iNode = (MobileNode*) (Node::get_node_by_address(index));

	fprintf(fp,"%d %f\n",index,iNode->energy_model()->energy());

	fclose(fp);
}
