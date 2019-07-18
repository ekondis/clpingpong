__kernel  __attribute__((reqd_work_group_size(1, 1, 1)))
void kPingPong(__global int *buf){
	if( get_global_id(0) == 0 )
		(*buf)++;
}
