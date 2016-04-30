
#ifndef __leach_packet_h__
#define __leach_packet_h__

/* =====================================================================
   Packet Formats...
   ===================================================================== */
#define LEACHTYPE_PATH_ADV   	1
#define LEACHTYPE_PATH_ACK   	2

/*
 * LEACH Routing Protocol Header Macros
 */
#define HDR_LEACH(p)		((struct hdr_leach*)hdr_leach::access(p))
#define HDR_LEACH_PATH_ADV(p)  	((struct hdr_leach_path_adv*)hdr_leach::access(p))
#define HDR_LEACH_PATH_ACK(p)	((struct hdr_leach_path_ack*)hdr_leach::access(p))

/*
 * General LEACH Header - shared by all formats
 */
struct hdr_leach {
        u_int8_t        le_type;
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_leach* access(const Packet* p) {
		return (hdr_leach*) p->access(offset_);
	}
};

struct hdr_leach_path_adv 
{
	u_int8_t        adv_type;

	nsaddr_t	adv_src_node;	
	double 		CH_x_pos;
	double 		CH_y_pos;
	double 		CH_z_pos;

  	inline int size() 
	{ 
  		int sz = 0;
  
  		sz = 3*sizeof(int32_t);		
	   
	  	assert (sz >= 0);
		return sz;
  	}
};

struct hdr_leach_path_ack 
{
        u_int8_t        ack_type;
	nsaddr_t	ack_src_node;
	nsaddr_t	ack_dest_node;
						
  	inline int size() 
	{ 
  		int sz = 0;
 
 	 	sz = 4*sizeof(int32_t);
		assert (sz >= 0);
		return sz;
  	}

};


// for size calculation of header-space reservation
union hdr_all_leach {
  hdr_leach          ah;
  hdr_leach_path_adv  p_adv;
  hdr_leach_path_ack    p_ack;
};

#endif /* __leach_packet_h__ */
