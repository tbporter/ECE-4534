typedef struct __timer3_thread_struct {
	// "persistent" data for this "lthread" would go here
	unsigned int	msgcount;
} timer3_thread_struct;

void init_timer3_lthread(timer3_thread_struct *);
int timer3_lthread(timer3_thread_struct *,int,int,unsigned char*);
