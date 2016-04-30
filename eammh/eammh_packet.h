#ifndef __eammh_packet_h__
#define __eammh_packet_h__

/* =====================================================================
   Packet Formats...
   ===================================================================== */
#define EAMMHTYPE_HELLO 1
#define EAMMHTYPE_PATH_ADV 2
#define EAMMHTYPE_PATH_ACK 3

/*
 * EAMMH Routing Protocol Header Macros
 */
#define HDR_EAMMH(p) ((struct hdr_eammh*)hdr_eammh::access(p))
#define HDR_EAMMH_HELLO(p) ((struct hdr_eammh_hello*)hdr_eammh::access(p))
#define HDR_EAMMH_PATH_ADV(p) ((struct hdr_eammh_path_adv*)hdr_eammh::access(p))
#define HDR_EAMMH_PATH_ACK(p) ((struct hdr_eammh_path_ack*)hdr_eammh::access(p))

/*
 * General EAMMH Header - shared by all formats
 */
struct hdr_eammh {
        u_int8_t eh_type;
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_eammh* access(const Packet* p) {
		return (hdr_eammh*) p->access(offset_);
	}
};

struct hdr_eammh_hello {
	u_int8_t hel_type;
	nsaddr_t hel_src;
	double x_pos;
	double y_pos;
	double z_pos;

	inline int size() 
	{ 
  		int sz = 0;
  		sz = 2*sizeof(int32_t)+3*sizeof(double); 
	  	assert (sz >= 0);
		return sz;
  	}
};

struct hdr_eammh_path_adv 
{
	u_int8_t adv_type;
	nsaddr_t adv_src_node;
	int32_t no_of_hops;
	double min_energy;
	double avg_energy;
	double traffic_info;
	nsaddr_t neighbor_node;
	int32_t budget_alloted;
		

  	inline int size() 
	{ 
  		int sz = 0;
  		sz = 8*sizeof(int32_t) + 3*sizeof(double);	
	  	assert (sz >= 0);
		return sz;
  	}
};

struct hdr_eammh_path_ack 
{
    u_int8_t ack_type;
	nsaddr_t ack_src_node;
	nsaddr_t ack_dest_node;
	bool alternate_path;		
						
  	inline int size() 
	{
  		int sz = 0;
 	 	sz = 2*sizeof(int32_t)+sizeof(bool);
		assert (sz >= 0);
		return sz;
  	}

};


// for size calculation of header-space reservation
union hdr_all_eammh {
	hdr_eammh ah;
	hdr_eammh_hello hello;
	hdr_eammh_path_adv p_adv;
	hdr_eammh_path_ack p_ack;
};

#endif /* __eammh_packet_h__ */
