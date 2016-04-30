
#include "leach/leach.h"
#include "leach/leach_packet.h"
#include "stdlib.h"
#include "god.h"
#include "math.h"
#include <random.h>
#include <cmu-trace.h>

#define CURRENT_TIME    Scheduler::instance().clock()

#define RT_PORT_1	255

int hdr_leach::offset_;
static class LEACHHeaderClass : public PacketHeaderClass {
public:
	LEACHHeaderClass() : PacketHeaderClass("PacketHeader/LEACH", 
					      sizeof(hdr_all_leach)) {
		bind_offset(&hdr_leach::offset_);
	}
} class_rtProtoLEACH_hdr;


static class LEACHClass : public TclClass {
public:
	LEACHClass() : TclClass("Agent/LEACH") {}
	TclObject* create(int argc, const char*const* argv) {
          assert(argc == 5);
          return (new LEACH((nsaddr_t) Address::instance().str2addr(argv[4])));
        }
} class_rtProtoLEACH;


/* 
   Constructor
*/

LEACH::LEACH(nsaddr_t id) : Agent(PT_LEACH),
			  chtimer(this), cftimer(this), etimer(this), cctimer(this) {
                 
  index = id;
  distance_CH = 9999.0;
  
  MobileNode	*iNode;
  iEnergy = 0.0;
	
  ifqueue = 0;

  logtarget = 0;

  percentage_CH = 0.04;
  round = (int)(1 / percentage_CH) + 1;

  is_cluster_head = false;
  is_clustered = false;

}

LEACH::LEACH() : Agent(PT_LEACH),
			  chtimer(this), cftimer(this), etimer(this), cctimer(this) {
       
  distance_CH = 9999.0;

  MobileNode	*iNode;
  iEnergy = 0.0;

  ifqueue = 0;

  logtarget = 0;

  percentage_CH = 0.04;
  round = (int)(1 / percentage_CH) + 1;

  is_cluster_head = false;
  is_clustered = false;

}

int LEACH::command(int argc, const char*const* argv)
{

    Tcl& tcl = Tcl::instance();

    if(argc == 2) {
    
    if(strncasecmp(argv[1], "id", 2) == 0) {
      tcl.resultf("%d", index);
      return TCL_OK;
    }
    else if(strcmp(argv[1],"cluster_formation") == 0) {
	cftimer.handle((Event*) 0);
      return TCL_OK;
    }            
    else if(strcmp(argv[1],"select_cluster_heads") == 0) {
	chtimer.handle((Event*) 0);	
	return TCL_OK;
    }
    else if(strcmp(argv[1],"print_is_clustered") == 0) {	
	is_node_clustered();	
	return TCL_OK;
    }
    else if(strcmp(argv[1],"change_color") == 0) {
	cctimer.handle((Event*) 0);	
	return TCL_OK;
    }
    else if(strcmp(argv[1],"record_energy_data") == 0) {
	etimer.handle((Event*) 0);
	return TCL_OK;
    }
  }
  else if(argc == 3) {
    if(strcmp(argv[1], "index") == 0) {
      index = atoi(argv[2]);
      return TCL_OK;
    }
    else if(strcmp(argv[1], "log-target") == 0 || strcmp(argv[1], "tracetarget") == 0) {
      logtarget = (Trace*) TclObject::lookup(argv[2]);
      if(logtarget == 0)
	return TCL_ERROR;
      return TCL_OK;
    }

    else if(strcmp(argv[1], "if-queue") == 0) {
    ifqueue = (PriQueue*) TclObject::lookup(argv[2]);
      
      if(ifqueue == 0)
	return TCL_ERROR;
      return TCL_OK;
    }

    else if (strcmp(argv[1], "port-dmux") == 0) {
    	dmux_ = (PortClassifier *)TclObject::lookup(argv[2]);
	if (dmux_ == 0) {
		fprintf (stderr, "%s: %s lookup of %s failed\n", __FILE__,
		argv[1], argv[2]);
		return TCL_ERROR;
	}
	return TCL_OK;
    }
  }
  
  // If the command hasn't been processed by LeachAgent()::command,
  // call the command() function for the base class
  return (Agent::command(argc, argv));
}



/*
  Timers
*/

void
ClusterHeadSelectionTimerLeach::handle(Event*) {
  agent->select_cluster_head();
  Scheduler::instance().schedule(this, &intr, CLUSTER_HEAD_SELECTION);
}

void
ChangeColorTimerLeach::handle(Event*) {
  agent->change_color();
  Scheduler::instance().schedule(this, &intr, CHANGE_COLOR);
}

void
ClusterFormationTimerLeach::handle(Event*) {
   
   if(agent->is_cluster_head)
	agent->sendPathAdv();
   Scheduler::instance().schedule(this, &intr,CLUSTER_FORMATION); 
}

void
RecordEnergyDataTimerLeach::handle(Event*) {
   agent->record_energy_data();
   Scheduler::instance().schedule(this, &intr, RECORD_ENERGY_DATA); 
}

/*
  Packet Reception Routines
*/


void
LEACH::recv(Packet *p, Handler*) {
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);

iNode = (MobileNode*) (Node::get_node_by_address(index));

if(index != 100)
	iNode->energy_model()->DecrTxEnergy(0.001198,1);


if(ch->ptype() == PT_LEACH && ch->direction() == hdr_cmn::UP && index!=100) 
{
   	recvLEACH(p);
   	return;
}
else
	recvDataTrans(p);
}


void
LEACH::recvLEACH(Packet *p) {
 struct hdr_leach *le = HDR_LEACH(p);

 assert(HDR_IP (p)->sport() == RT_PORT_1);
 assert(HDR_IP (p)->dport() == RT_PORT_1);

 /*
  * Incoming Packets.
  */
 switch(le->le_type) {

 case LEACHTYPE_PATH_ADV: 
   recvPathAdv(p);
   break;

 case LEACHTYPE_PATH_ACK: 
   recvPathAck(p);
   break;

 default:	printf("nothing\n");
   exit(1);
 }

}


void
LEACH::recvPathAdv(Packet *p) {
struct hdr_ip *ih = HDR_IP(p);
struct hdr_leach *eh = HDR_LEACH(p);
struct hdr_leach_path_adv *adv = HDR_LEACH_PATH_ADV(p);

iNode = (MobileNode*) (Node::get_node_by_address(index));

  /*
   * Drop if:
   *      - I'm the source.
   *      - I'm the Base Station.
   *	  - I'm the CH.
   */

  if(adv->adv_src_node == index) {
    Packet::free(p);
    return;
  } 

  if(index == 100) {
    Packet::free(p);
    return;
  } 

  if(is_cluster_head == true) {
    Packet::free(p);
    return;   
  }

  double dist;
  double d_x = adv->CH_x_pos;
  double d_y = adv->CH_y_pos;
  double d_z = adv->CH_z_pos;

  double s_x = iNode->X();
  double s_y = iNode->Y();
  double s_z = iNode->Z();

  dist = sqrt( ( (d_x-s_x)*(d_x-s_x) ) +
		( (d_y-s_y)*(d_y-s_y) ) +
		( (d_z-s_z)*(d_z-s_z) ) );

  if(dist < distance_CH)
  {
	distance_CH = dist;
	cluster_head = adv->adv_src_node;
  }

  sendPathAck(adv->adv_src_node,index);

 return;
}

void
LEACH::recvPathAck(Packet *p) {
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_leach_path_ack *ack = HDR_LEACH_PATH_ACK(p);

/*
	for now do nothing
*/

Packet::free(p);

}

void
LEACH::recvDataTrans(Packet *p) {
struct hdr_ip *ih = HDR_IP(p);
struct hdr_cmn *ch = HDR_CMN(p);

iNode = (MobileNode*) (Node::get_node_by_address(index));
iEnergy = iNode->energy_model()->energy();
double decr_energy = 0.0;


if(!is_cluster_head)
{

	 ch->iface() = -2;
	 ch->error() = 0;
	 ch->addr_type() = NS_AF_INET;
	 ch->prev_hop_ = index;          // EAMMH hack
	 ch->next_hop_ = cluster_head;
	 ch->direction() = hdr_cmn::DOWN ;	//change direction before forwarding : v imp
	
	 ih->saddr() = index;
	 ih->daddr() = cluster_head;		
	 ih->sport() = RT_PORT_1;
	 ih->dport() = RT_PORT_1;
	 ih->ttl_ = 1;
	 
	 if(distance_CH <= 50.0)
		 decr_energy = 0.001597;
	 else
		 decr_energy = ( 0.001597 + ( ( ( 0.005027 * distance_CH) * ( 0.005027 * distance_CH) ) * 0.001597 ));

	 iNode->energy_model()->DecrTxEnergy(decr_energy,1);

	 Scheduler::instance().schedule(target_, p, 0.0);
}
else
if(index!=100)
{
	 ch->iface() = -2;
	 ch->error() = 0;
	 ch->addr_type() = NS_AF_INET;
	 ch->prev_hop_ = index;          // LEACH hack
	 ch->next_hop_ = (nsaddr_t)100;		
	 ch->direction() = hdr_cmn::DOWN ;	//change direction before forwarding : v imp
	
	 ih->saddr() = index;
	 ih->daddr() = (nsaddr_t)100;		
	 ih->sport() = RT_PORT_1;
	 ih->dport() = RT_PORT_1;
	 ih->ttl_ = 1;
	 
	 iNode->energy_model()->DecrTxEnergy(0.01597,1);

	 Scheduler::instance().schedule(target_, p, 0.0);
}
else
	Packet::free(p);
}

/*
   Packet Transmission Routines
*/

/* 

Initial state of the implementation when the cluster head broadcasts the path adv pkt

function i/p parameters shld include Emin , Eavg etc

*/
void
LEACH::sendPathAdv(void) {
// Allocate a Path ADV packet 

if(index!=100)
{
	Packet *p = Packet::alloc();
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_leach *le = HDR_LEACH(p);
	struct hdr_leach_path_adv *adv = HDR_LEACH_PATH_ADV(p);

	iNode = (MobileNode*) (Node::get_node_by_address(index));

	le->le_type = LEACHTYPE_PATH_ADV;	
	
	adv->adv_src_node = index;
	adv->CH_x_pos = iNode->X();
	adv->CH_y_pos = iNode->Y();
	adv->CH_z_pos = iNode->Z();
	
	 ch->ptype() = PT_LEACH;
	 ch->iface() = -2;
	 ch->addr_type() = NS_AF_NONE;
	 ch->next_hop_ = IP_BROADCAST;
	 ch->prev_hop_ = index;          // LEACH hack
	 ch->direction() = hdr_cmn::DOWN;
	 ch->error() = 0;
	
	 ih->saddr() = index;
	 ih->daddr() = IP_BROADCAST;
	 ih->sport() = RT_PORT_1;
	 ih->dport() = RT_PORT_1;
	
	 Scheduler::instance().schedule(target_, p, 0.);
	
	 iNode->energy_model()->DecrTxEnergy(0.01597,1);
}
return;
}

void
LEACH::sendPathAck(nsaddr_t dest, nsaddr_t src) {
Packet *p = Packet::alloc();
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_leach *le = HDR_LEACH(p);
struct hdr_leach_path_ack *ack = HDR_LEACH_PATH_ACK(p);

if(index!=100)
{

 le->le_type = LEACHTYPE_PATH_ACK;	
 
 ack->ack_type = LEACHTYPE_PATH_ACK;
 ack->ack_src_node = src;
 ack->ack_dest_node = dest;
   
 ch->ptype() = PT_LEACH;
 ch->iface() = -2;
 ch->addr_type() = NS_AF_INET;
 ch->next_hop_ = dest;
 ch->prev_hop_ = src;          // LEACH hack
 ch->direction() = hdr_cmn::DOWN;
 ch->error() = 0;

 ih->saddr() = src;
 ih->daddr() = dest;
 ih->sport() = RT_PORT_1;
 ih->dport() = RT_PORT_1;

 Scheduler::instance().schedule(target_, p, 0.);

 iNode->energy_model()->DecrTxEnergy(0.001597,1);
}
return;
}


void 
LEACH::select_cluster_head(void)
{

	double threshold;
	
	threshold = ( (percentage_CH) / ( 1 - ( percentage_CH * ( round % ( (int)(1 / percentage_CH) ) ) ) ) );

	double t_check;

	t_check = rand() % 1000;
	t_check = t_check / 1000;

	if( t_check < threshold && round > (int)(1 / percentage_CH) )
	{
		is_cluster_head = true;
		round = 1;
	}
	else
	{
		is_cluster_head = false;
		round++;
	}
}

void
LEACH::is_node_clustered(void)
{
	if(is_clustered == false)
		printf("\n node %d is not clustered",(int)index);
}

void
LEACH::change_color(void)
{

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
LEACH::record_energy_data(void)
{
	FILE *fp = fopen("energy_trace.tr","a");

	iNode = (MobileNode*) (Node::get_node_by_address(index));

	fprintf(fp,"%d %f\n",index,iNode->energy_model()->energy());

	fclose(fp);
}
