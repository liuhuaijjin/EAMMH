#include <eammh/eammh_rtable.h>

/*
 * The Routing Table Entry
 */
eammh_rt_entry::eammh_rt_entry()
{
	shortest_path = false;
	next_hop = (nsaddr_t)999;
	E_min = 5.0;
	min_no_hops = 0;
	E_avg = 0.0;
	traffic = 0.0;
	heuristic_value = 0.0;
	LIST_INIT(&rt_nblist);
}

eammh_rt_entry::~eammh_rt_entry()
{
	EAMMH_Neighbor *nb;
    while((nb = rt_nblist.lh_first)) {
	   LIST_REMOVE(nb, nb_link);
	   delete nb;
	}
}

void 
eammh_rt_entry::nb_insert(nsaddr_t id)
{
	EAMMH_Neighbor *nb = new EAMMH_Neighbor(id);
	assert(nb);
	LIST_INSERT_HEAD(&rt_nblist, nb, nb_link);
}

EAMMH_Neighbor*
eammh_rt_entry::nb_lookup(nsaddr_t id)
{
	EAMMH_Neighbor *nb = rt_nblist.lh_first;
	for(; nb; nb = nb->nb_link.le_next) {
		if(nb->nb_addr == id)
		break;
	}
	return nb;
}


void
eammh_rt_entry::nb_delete(nsaddr_t id)
{
	EAMMH_Neighbor *nb = nb_lookup(id);
	if(nb) {
		LIST_REMOVE(nb, nb_link);
		delete nb;
	}
}

/*
 * The Routing Table
 */
eammh_rt_entry*
eammh_rtable::rt_lookup()
{
	eammh_rt_entry *rt = rthead.lh_first;
	eammh_rt_entry *rt_h;
	double heur_value=0.0;

 	for(; rt; rt = rt->rt_link.le_next) {
  		if(rt->heuristic_value > heur_value)
		{
			if(rt->E_min > (rt->E_avg/10.0))
			{
				heur_value = rt->heuristic_value;
				rt_h = rt;
			}
		}
 	}
 	return rt_h;
}

eammh_rt_entry*
eammh_rtable::rt_lookup(nsaddr_t node_id)
{
	eammh_rt_entry *rt = rthead.lh_first;
	//printf("inside lookup %d\n",(int)node_id);

	for(; rt; rt = rt->rt_link.le_next) {
	if(rt->next_hop == node_id)
		break;
	}

	printf("damn fpe");
	return rt;
}


void
eammh_rtable::rt_delete(nsaddr_t node_id)
{
	eammh_rt_entry *rt = rt_lookup(node_id);

	if(rt) {
		LIST_REMOVE(rt, rt_link);
		delete rt;
	}
}

eammh_rt_entry*
eammh_rtable::rt_add(nsaddr_t node_id,double e_min,int hop_count,double e_avg,double tra,int budget)
{
	eammh_rt_entry *rt;
	eammh_rt_entry *rt_head = rthead.lh_first;
	bool alternate_path = true;
	double heur_value;

 	assert(rt_lookup(node_id) == 0);
 	rt = new eammh_rt_entry;
 	assert(rt);
	
	if(rt_head == NULL)	{
		alternate_path = false;
	}
	else if(budget > 0) {
		alternate_path = false;
		eammh_rt_entry *rt_p = rthead.lh_first;

		for(; rt_p; rt_p = rt_p->rt_link.le_next) {
  			if(rt_p->shortest_path == true) {
				alternate_path = true;
  				break;
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
