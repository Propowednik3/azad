#include <stdio.h>
#include "pthread2threadx.h"
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include "debug.h"

int tx_semaphore_create(TX_SEMAPHORE *psem, void *attr, unsigned int val)
{
		
	pthread_condattr_init(&psem->condattr);
	pthread_condattr_setclock(&psem->condattr, CLOCK_MONOTONIC);
	pthread_cond_init(&psem->condition, &psem->condattr);
	pthread_mutex_init(&psem->mutex, attr);
	psem->semval = val;
	psem->special = 0;
	psem->error = 0;
	psem->reset = 0;
	psem->list_count = 0;
	int i;
	for (i=0;i!=TX_MAX_LIST_SIZE;i++)
	 {
		psem->semval_list[i] = 0xFFFFFFFF;
		psem->semval_list2[i] = 0xFFFFFFFF;
		psem->special_list[i] = 0;
	  }

	return 0;
}

int tx_semaphore_put(TX_SEMAPHORE *psem)
{
	pthread_mutex_lock(&psem->mutex);
	psem->semval++;
	pthread_cond_signal(&psem->condition);
	pthread_mutex_unlock(&psem->mutex);
	return 0;
}

int tx_semaphore_get(TX_SEMAPHORE *psem, unsigned int waitmode)
{
	pthread_mutex_lock(&psem->mutex);
	while (psem->semval == 0)
	{
		pthread_cond_wait(&psem->condition, &psem->mutex);
	}
	psem->semval--;
	pthread_mutex_unlock(&psem->mutex);
	return 0;
}

int tx_semaphore_reset(TX_SEMAPHORE *psem)
{
	pthread_mutex_lock(&psem->mutex);
	psem->semval=0;
	psem->reset = 0;	
	psem->list_count=0;
	psem->special=0;
	psem->error = 0;
	pthread_mutex_unlock(&psem->mutex);
	return 0;
}

int tx_semaphore_add(TX_SEMAPHORE *psem)
{
	pthread_mutex_lock(&psem->mutex);
	psem->semval++;
	pthread_mutex_unlock(&psem->mutex);
	return 0;
}

int tx_semaphore_add_in_list(TX_SEMAPHORE *psem, unsigned int event, unsigned int cmd)
{
	pthread_mutex_lock(&psem->mutex);
	//printf("Add cmd event:%i, Cmd:%i,  total:%i\n",event, cmd, psem->list_count+1);
	psem->semval++;
	//printf("Add cmd event:%i\n", psem->list_count);
	int i;
	for (i = 0; i != TX_MAX_LIST_SIZE; i++)
	{
		if (((psem->semval_list[i] == 0) || (psem->semval_list[i] == -1)) && 
			((psem->semval_list2[i] == 0) || (psem->semval_list2[i] == -1)))
			{
				psem->semval_list[i] = cmd;
				psem->semval_list2[i] = event;
				psem->special_list[i] = 0;
				break;
			}
	}
	psem->list_count = 1;	
	pthread_mutex_unlock(&psem->mutex);
	return 0;
}

int tx_semaphore_add_in_list_spec(TX_SEMAPHORE *psem, unsigned int event, unsigned int cmd, int NotLock)
{
	if (NotLock == 0) pthread_mutex_lock(&psem->mutex);
	//printf("Add spec cmd event:%i, Cmd:%i,  total:%i\n",event, cmd, psem->list_count+1);
	psem->special++;
	int i;
	for (i = 0; i != TX_MAX_LIST_SIZE; i++)
	{
		if (((psem->semval_list[i] == 0) || (psem->semval_list[i] == -1)) && 
			((psem->semval_list2[i] == 0) || (psem->semval_list2[i] == -1)))
			{
				psem->semval_list[i] = cmd;
				psem->semval_list2[i] = event;
				psem->special_list[i] = 1;
				break;
			}
	}
	psem->list_count = 2;	
	if (NotLock == 0) pthread_mutex_unlock(&psem->mutex);
	return 0;
}

int tx_semaphore_delete_from_list(TX_SEMAPHORE *psem, unsigned int event, unsigned int cmd)
{
	pthread_mutex_lock(&psem->mutex);
	//printf("send cmd %i,%i\n",cmd, psem->list_count+1);
	int i;
	//count = psem->list_count;
	//printf("semval before :%i\n",psem->semval);	
	if (psem->list_count != 0)			
	  for (i=0;i!=TX_MAX_LIST_SIZE;i++)
	  {
		if (((psem->semval_list[i] == cmd) || (psem->semval_list[i] == TX_ANY)) && (psem->semval_list2[i] == event))
		{
			//printf("delete  %i) Event:%i      Cmd:%i     Total:%i\n",i,psem->semval_list2[i],psem->semval_list[i], psem->list_count);		
			psem->semval_list[i] = 0xFFFFFFFF;
			psem->semval_list2[i] = 0xFFFFFFFF;
			if (psem->special_list[i] == 1)
			{
				psem->special--;
				psem->special_list[i] = 0;
			}
			else psem->semval--;
			if (psem->semval == 0) psem->list_count = 0;
			//pthread_cond_signal(&psem->condition);
			//break;				
		}
	  }
	pthread_mutex_unlock(&psem->mutex);
	return 0;
}

int tx_semaphore_print_list(TX_SEMAPHORE *psem)
{
	pthread_mutex_lock(&psem->mutex);
	//printf("send cmd %i,%i\n",cmd, psem->list_count+1);
	int i;
	//count = psem->list_count;
	//printf("semval before :%i\n",psem->semval);	
	if (psem->list_count != 0)			
	  for (i=0;i!=TX_MAX_LIST_SIZE;i++)	  
		printf("print  %i) Event:%i      Cmd:%i     Spec:%i    Total:%i\n",i,psem->semval_list2[i],psem->semval_list[i],psem->special_list[i], psem->list_count);
     else printf("No event for wait std:%i , spec:%i\n",psem->semval,psem->special);
	
	pthread_mutex_unlock(&psem->mutex);
	return 0;
}

int tx_semaphore_go(TX_SEMAPHORE *psem, unsigned int event, unsigned int cmd)
{
	pthread_mutex_lock(&psem->mutex);
	if (cmd == 123456789)
	{
		psem->error = 1;
		pthread_cond_signal(&psem->condition);
		pthread_mutex_unlock(&psem->mutex);
		return 0;
	}
	if (psem->list_count == 0)
	{
		//printf("semval  :%i\n",psem->semval);			
		if (psem->semval !=0) 
		{
			psem->semval--;
			pthread_cond_signal(&psem->condition);
		}
		//else printf("error: empty events\n");
	}
	else
	{
			int i;
			//count = psem->list_count;
			//printf("semval before :%i\n",psem->semval);				
			for (i=0;i!=TX_MAX_LIST_SIZE;i++)
			{
				// printf("%i) Event:%i/%i      Cmd:%i/%i     Total:%i\n",i,psem->semval_list2[i],event,psem->semval_list[i],cmd, psem->list_count);
				if (((psem->semval_list[i] == cmd) || (cmd == TX_ANY) || (psem->semval_list[i] == TX_ANY)) && (psem->semval_list2[i] == event))
				{
					//printf("fined  %i) Event:%i/%i      Cmd:%i/%i    Spec:%i    Total:%i\n",i,psem->semval_list2[i],event,psem->semval_list[i],cmd, psem->special_list[i],psem->list_count);					
				    psem->semval_list[i] = 0xFFFFFFFF;
					psem->semval_list2[i] = 0xFFFFFFFF;					
					if (psem->special_list[i] == 1)
					{
						psem->special--;
						psem->special_list[i] = 0;
					}
					else psem->semval--;
					if ((psem->semval == 0) && (psem->special == 0)) psem->list_count = 0;
					pthread_cond_signal(&psem->condition);
					break;				
				}
				if ((psem->semval_list[i] != 0xFFFFFFFF) && (psem->semval_list2[i] != 0xFFFFFFFF) && (event == TX_RESET) && (psem->semval != 0))
				{
					//printf("fined  %i) Event:%i/%i      Cmd:%i/%i    Spec:%i    Total:%i\n",i,psem->semval_list2[i],event,psem->semval_list[i],cmd, psem->special_list[i],psem->list_count);					
				    psem->semval_list[i] = 0xFFFFFFFF;
					psem->semval_list2[i] = 0xFFFFFFFF;					
					if (psem->special_list[i] == 1)
					{
						psem->special--;
						psem->special_list[i] = 0;
					}
					else psem->semval--;
					if ((psem->semval == 0) && (psem->special == 0)) psem->list_count = 0;
					psem->reset = 1;
					dbgprintf(3, "BREAK WAIT0 %i, %i\n", psem->semval_list[i], psem->semval_list2[i]);							
					pthread_cond_signal(&psem->condition);									
				}
			}
			//printf("semval after :%i\n",psem->semval);										
	}
	pthread_mutex_unlock(&psem->mutex);	
	return 0;
}

int tx_semaphore_wait(TX_SEMAPHORE *psem)
{
	pthread_mutex_lock(&psem->mutex);	
	while (psem->semval > 0)
	{
		if (psem->error == 1) 
		{
			psem->semval = 0;
			psem->error = 0;
			pthread_mutex_unlock(&psem->mutex);
			return 0;
		}
		pthread_cond_wait(&psem->condition, &psem->mutex);
		//printf("semval:%i\n",psem->semval);
	}
	
	pthread_mutex_unlock(&psem->mutex);
	//printf("wait done\n");	
	return 1;
}

int tx_semaphore_wait_list_empty(TX_SEMAPHORE *psem)
{
	int ret = 0;
	pthread_mutex_lock(&psem->mutex);
	if (psem->semval == 0) ret = 1;
	pthread_mutex_unlock(&psem->mutex);
	return ret;
}

int tx_semaphore_wait_timeout(TX_SEMAPHORE *psem, int timeout_ms)
{	
	struct timespec   ts, ts2;
	int ret = timeout_ms;
	int rt = 0;
    clock_gettime(CLOCK_MONOTONIC, &ts);
	/* Convert from timeval to timespec */
    ts.tv_nsec = ts.tv_nsec + (timeout_ms % 1000) * 1000000;
    ts.tv_sec += timeout_ms / 1000;
	ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
    ts.tv_nsec %= (1000 * 1000 * 1000);

    pthread_mutex_lock(&psem->mutex);	
    while ((psem->semval != 0) && (rt == 0))
	{
		rt = pthread_cond_timedwait(&psem->condition, &psem->mutex, &ts);			
	}
	pthread_mutex_unlock(&psem->mutex);
	
	if (rt != ETIMEDOUT)
	{
		clock_gettime(CLOCK_MONOTONIC, &ts2);	
		ret = ((ts.tv_sec - ts2.tv_sec) * 1000) + ((ts.tv_nsec - ts2.tv_nsec) / 1000000);
	}
	else
	{
		ret = 0;		
	}
	
	if ((timeout_ms) && ((timeout_ms - ret) > 250))
	{
		dbgprintf(4,"tx_semaphore_wait_timeout____(%i) timeout_ms %i\n", timeout_ms - ret, timeout_ms);
		//dbg_func_print_entered();
	}
	
	return ret;
}

int tx_semaphore_wait_list_any_timeout(TX_SEMAPHORE *psem, EVENT_LIST * pEventList, int iEvListlen, int timeout_ms)
{	
	struct timespec   ts;
	struct timespec   ts2;
	int rt = 0;
	int ret = timeout_ms;
	int res = -1;
	int i, n;
	int clk = 0;
    clock_gettime(CLOCK_MONOTONIC, &ts);
	/* Convert from timeval to timespec */
    ts.tv_nsec = ts.tv_nsec + (timeout_ms % 1000) * 1000000;
    ts.tv_sec += timeout_ms / 1000;
	ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
    ts.tv_nsec %= (1000 * 1000 * 1000);
	pthread_mutex_lock(&psem->mutex);	
    if ((psem->semval != 0) && (psem->list_count != 0))
	{
		do
		{
			for (n = 0; n != iEvListlen; n++)
			{
				clk = 0;
				for (i = 0; i != TX_MAX_LIST_SIZE; i++)
					if ((psem->semval_list[i] == pEventList[n].cmd) 
						&& (psem->semval_list2[i] == pEventList[n].event)) 
						{
							clk = 1; 
							break;
						}
				if (clk == 0)
				{
					res = n;
					break;
				}
			}
			if (res == -1) rt = pthread_cond_timedwait(&psem->condition, &psem->mutex, &ts);	
		} while ((res == -1) && (rt != ETIMEDOUT));
	}
	pthread_mutex_unlock(&psem->mutex);
	if (rt != ETIMEDOUT)
	{
		clock_gettime(CLOCK_MONOTONIC, &ts2);	
		ret = ((ts.tv_sec - ts2.tv_sec) * 1000) + ((ts.tv_nsec - ts2.tv_nsec) / 1000000);   
	}
	else
	{
		ret = 0;		
	}	
	
	if ((timeout_ms) && ((timeout_ms - ret) > 250))
	{
		dbgprintf(4,"tx_semaphore_wait_list_any_timeout____(%i) , timeout_ms %i\n", timeout_ms - ret, timeout_ms);
		//dbg_func_print_entered();
	}
	
	return res;
}

int tx_semaphore_wait_list_empty_timeout2(TX_SEMAPHORE *psem, int timeout_ms)
{	
	int timerclk = timeout_ms*1000;
	while(1)
	{
		if (tx_semaphore_wait_list_empty(psem) == 0) 
		{
			usleep(1); 
			timerclk--;
		} 
		else return (int)(timerclk/1000);
		if (timerclk == 0) return 0;
	}
}
	
int tx_semaphore_wait_spec(TX_SEMAPHORE *psem)
{
	pthread_mutex_lock(&psem->mutex);
	while (psem->special != 0)
	{
		pthread_cond_wait(&psem->condition, &psem->mutex);
		//printf("semval:%i\n",psem->semval);
	}	
	//printf("wait done\n");
	pthread_mutex_unlock(&psem->mutex);
	return 0;
}

int tx_semaphore_wait_spec_list_empty(TX_SEMAPHORE *psem)
{
	int ret = 0;
	pthread_mutex_lock(&psem->mutex);
	if (psem->special == 0) ret = 1;
	pthread_mutex_unlock(&psem->mutex);
	return ret;
}

int tx_semaphore_exist_in_list(TX_SEMAPHORE *psem, unsigned int event, unsigned int cmd)
{
	pthread_mutex_lock(&psem->mutex);
	int i;
	int clk = 0;
	for (i = 0; i != TX_MAX_LIST_SIZE; i++)
		if ((psem->semval_list[i] == cmd) && (psem->semval_list2[i] == event)) clk++;	
	pthread_mutex_unlock(&psem->mutex);
	return clk;
}
/*
int tx_semaphore_wait_event(TX_SEMAPHORE *psem, unsigned int event, unsigned int cmd)
{	
	while(1)
	{
		if (tx_semaphore_exist_in_list(psem, event, cmd) != 0) usleep(1000);  else break;
	}
	return 1;
}*/

int tx_semaphore_wait_event_timeout(TX_SEMAPHORE *psem, unsigned int event, unsigned int cmd, int timeout_ms)
{	
	struct timespec   ts, ts2;
	int rt = 0;
	int ret = timeout_ms;
	char cExtraEx = 0;
	int i;
	int clk = 0;
	int clk2 = 0;
	
	if (timeout_ms)
	{
		clock_gettime(CLOCK_MONOTONIC, &ts);
		/* Convert from timeval to timespec */
		ts.tv_nsec = ts.tv_nsec + (timeout_ms % 1000) * 1000000;
		ts.tv_sec += timeout_ms / 1000;
		ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
		ts.tv_nsec %= (1000 * 1000 * 1000);
	}
	
    pthread_mutex_lock(&psem->mutex);
	if ((psem->semval != 0) && (psem->list_count != 0))
	{
		clk = 0;	
		for (i = 0; i != TX_MAX_LIST_SIZE; i++)
			if ((psem->semval_list[i] == cmd) && (psem->semval_list2[i] == event)) clk++; 
		if (clk != 0)
		{
			do
			{				
				if (timeout_ms)
				{
					rt = pthread_cond_timedwait(&psem->condition, &psem->mutex, &ts);	
				}
				else 
				{
					//printf("cond wait\n");
					rt = pthread_cond_wait(&psem->condition, &psem->mutex);
				}
				if (rt == 0)
				{
					clk2 = 0;
					for (i = 0; i != TX_MAX_LIST_SIZE; i++)
					{
						if ((psem->semval_list[i] == cmd) && (psem->semval_list2[i] == event)) clk2++;
						if (psem->reset == 1) 
						{
							dbgprintf(3, "BREAK WAIT1\n");
							if (psem->semval == 0) psem->reset = 0;
							rt = ETIMEDOUT;
							cExtraEx = 1;
						}
					}
				}
			} while ((clk == clk2) && (rt == 0));
		}
	}
	pthread_mutex_unlock(&psem->mutex);
	if (((timeout_ms) && (rt != ETIMEDOUT)) || (cExtraEx == 1))
	{
		clock_gettime(CLOCK_MONOTONIC, &ts2);	
		ret = ((ts.tv_sec - ts2.tv_sec) * 1000) + ((ts.tv_nsec - ts2.tv_nsec) / 1000000);
	}
	else 
	{
		ret = 0;		
	}
	
	if ((timeout_ms) && ((timeout_ms - ret) > 250))
	{
		dbgprintf(4,"tx_semaphore_wait_event_timeout____(%i) %i, event %i, cmd %i, timeout_ms %i\n", cExtraEx, timeout_ms - ret, event, cmd, timeout_ms);
		//dbg_func_print_entered();
	}
	
	if (cExtraEx == 1) return 0;
	return ret;
}

int tx_semaphore_wait_event_timeout2(TX_SEMAPHORE *psem, unsigned int event, unsigned int cmd, int timeout_ms)
{	
	int timerclk = timeout_ms*1000;
	int clk = tx_semaphore_exist_in_list(psem, event, cmd);
	int clk2 = 0;
	while(1)
	{
		clk2 = tx_semaphore_exist_in_list(psem, event, cmd);
		if ((clk2 != 0) && (clk == clk2))
		{
			usleep(1); 
			timerclk--;
		} 
		else return (int)(timerclk/1000);
		if (timerclk == 0) return 0;
	}	
}

int tx_semaphore_delete(TX_SEMAPHORE *psem)
{
	pthread_cond_destroy(&psem->condition);
	pthread_mutex_destroy(&psem->mutex);
	pthread_condattr_destroy(&psem->condattr);
	return 0;
}
// create eventer
int tx_eventer_create(TX_EVENTER *pevnt, char cUnDelete)
{
	pthread_condattr_init(&pevnt->condattr);
	pthread_condattr_setclock(&pevnt->condattr, CLOCK_MONOTONIC);
	pthread_cond_init(&pevnt->condition, &pevnt->condattr);
	pthread_mutex_init(&pevnt->mutex, NULL);	
	pevnt->status = 0;	
	pevnt->undelete = cUnDelete;
	pevnt->recv_type = 0;
	pevnt->recv_datalen = 0;	
	pevnt->recv_data = NULL;
	pevnt->type_list = NULL;
	pevnt->type_list_cnt = 0;
	return 1;
}
// delete eventer
int tx_eventer_delete(TX_EVENTER *pevnt)
{
	pthread_mutex_lock(&pevnt->mutex);
	if (pevnt->type_list_cnt != 0)
	{
		pevnt->type_list_cnt = 0;	
		free(pevnt->type_list);
	}
	pevnt->status = 0;	
	pevnt->recv_type = 0;
	pevnt->recv_datalen = 0;	
	pevnt->recv_data = NULL;
	pevnt->type_list = NULL;
	pthread_mutex_unlock(&pevnt->mutex);
	
	pthread_cond_destroy(&pevnt->condition);
	pthread_condattr_destroy(&pevnt->condattr);
	pthread_mutex_destroy(&pevnt->mutex);
	return 1;
}

unsigned int tx_eventer_recv_event(TX_EVENTER *pevnt, unsigned int uiType, unsigned int timeout_ms)
{
	int ret = tx_eventer_recv(pevnt, &uiType, timeout_ms, 0);
	if ((ret) && (uiType == 0)) ret = 0;
	return ret;
}

unsigned int tx_eventer_recv(TX_EVENTER *pevnt, unsigned int *uiType, unsigned int timeout_ms, char cWaitAll)
{	
	struct timespec  ts;
	int rt = 0;
	int ret, i, result;
	unsigned int uiNeedEvent = 0;
	if (uiType != NULL) uiNeedEvent = *uiType;
	
	pthread_mutex_lock(&pevnt->mutex);
	
	if ((pevnt->type_list_cnt) || (cWaitAll))
	{
		ret = 0;					
		for(i = 0; i < pevnt->type_list_cnt; i++)
		{
			if ((pevnt->type_list[i].code != 0) && ((pevnt->type_list[i].code == uiNeedEvent) || (uiNeedEvent == 0))) 
			{
				if ((pevnt->undelete == 1) && (pevnt->type_list[i].recved))
				{
					if (uiType != NULL) *uiType = pevnt->type_list[i].code;
					pevnt->type_list[i].recved--;
					pthread_mutex_unlock(&pevnt->mutex);
					return 1;
				}
				ret = 1;
			}
		}
		if (ret == 0)
		{
			if (uiType != NULL) *uiType = 0;
			pthread_mutex_unlock(&pevnt->mutex);
			return 1;
		}
	}
	if (timeout_ms == 0)
	{
		if (uiType != NULL) *uiType = 0;
		pthread_mutex_unlock(&pevnt->mutex);
		return 0;
	}
	pevnt->recv_type = 0;	
	result = 0;
	if (timeout_ms != TX_WAIT_FOREVER)
	{
		clock_gettime(CLOCK_MONOTONIC, &ts);		
		ts.tv_nsec = ts.tv_nsec + (timeout_ms % 1000) * 1000000;
		ts.tv_sec += timeout_ms / 1000;
		ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
		ts.tv_nsec %= (1000 * 1000 * 1000);
	}
	
	while(1)
	{
		if (timeout_ms != TX_WAIT_FOREVER)
			rt = pthread_cond_timedwait(&pevnt->condition, &pevnt->mutex, &ts);	
			else rt = pthread_cond_wait(&pevnt->condition, &pevnt->mutex);
					
		if (rt != ETIMEDOUT)
		{
			if (uiType != NULL) *uiType = pevnt->recv_type; 
			if (pevnt->recv_type == 0) break;
			if ((pevnt->type_list_cnt) && (pevnt->undelete))
			{
				for(i = 0; i < pevnt->type_list_cnt; i++) 
				{
					if ((pevnt->type_list[i].code == pevnt->recv_type) && ((pevnt->type_list[i].code == uiNeedEvent) || (uiNeedEvent == 0)))
					{										
						if (uiType != NULL) *uiType = pevnt->type_list[i].code;
						if (pevnt->type_list[i].recved) pevnt->type_list[i].recved--;
						result = 1;
						break;
					}
				}
				if (result == 1) break;				
			}
			if ((uiNeedEvent == pevnt->recv_type) && (pevnt->undelete == 0) && (cWaitAll == 0))
			{
				result = 1;
				break;				
			}
			if ((cWaitAll) && (pevnt->undelete == 0))
			{
				for(i = 0; i < pevnt->type_list_cnt; i++)
				{
					if ((pevnt->type_list[i].code != 0) && ((pevnt->type_list[i].code == uiNeedEvent) || (uiNeedEvent == 0))) break;
				}
				if (i == pevnt->type_list_cnt) 
				{
					result = 1;
					break;
				}
			}
		} else break;
	}
	pthread_mutex_unlock(&pevnt->mutex);	
	return result;
}

int tx_eventer_send_event(TX_EVENTER *pevnt, unsigned int uiEventType)
{
	int i;
	pthread_mutex_lock(&pevnt->mutex);
	
	if ((pevnt->type_list_cnt == 0) || (uiEventType == 0))
	{
		pevnt->recv_type = uiEventType;
		pthread_cond_signal(&pevnt->condition);
		pthread_mutex_unlock(&pevnt->mutex);
		return 1;
	}
			
	for(i = 0; i < pevnt->type_list_cnt; i++)
	{
		if (pevnt->type_list[i].code == uiEventType)
		{			
			if (pevnt->undelete == 0)
				pevnt->type_list[i].code = 0;
				else 
				pevnt->type_list[i].recved++;
			pevnt->recv_type = uiEventType;	
			pthread_cond_signal(&pevnt->condition);
			pthread_mutex_unlock(&pevnt->mutex);
			return 1;			
		}
	}
	pthread_mutex_unlock(&pevnt->mutex);	
	return 0;
}

unsigned int tx_eventer_recv_data(TX_EVENTER *pevnt, unsigned int uiNeedEvent, void** pData, unsigned int *uiDataLen, unsigned int timeout_ms)
{
	pthread_mutex_lock(&pevnt->mutex);	
	return tx_eventer_recv_data_prelocked(pevnt, uiNeedEvent, pData, uiDataLen, timeout_ms);
}

unsigned int tx_eventer_recv_data_prelocked(TX_EVENTER *pevnt, unsigned int uiNeedEvent, void** pData, unsigned int *uiDataLen, unsigned int timeout_ms)
{	
	struct timespec  ts;
	int rt = 0;
	int ret;
	
	//pthread_mutex_lock(&pevnt->mutex);	
			
	if (pevnt->status == 1)
	{
		ret = 0;	
		if ((pevnt->recv_type == uiNeedEvent) || (pevnt->recv_type == 0))
		{				
			if (pData != NULL) *pData = pevnt->recv_data; 
			if (uiDataLen != NULL) *uiDataLen = pevnt->recv_datalen; 
			if (pevnt->recv_type != 0) ret = 1;
			pthread_cond_signal(&pevnt->condition);	
		} else dbgprintf(2, "tx_eventer_recv_data: Wrong type data to recv need: %i, exist: %i\n",uiNeedEvent, pevnt->recv_type);
		pthread_mutex_unlock(&pevnt->mutex);
		return ret;	
	}
		
	if (pevnt->status != 0)	dbgprintf(2, "tx_eventer_recv_data: Wrong status to recv %i, need: %i, exist: %i\n", pevnt->status,uiNeedEvent, pevnt->recv_type);
	
	if (timeout_ms == 0)
	{
		pthread_mutex_unlock(&pevnt->mutex);
		return 0;
	}	
	//if (uiNeedEvent == 6) printf("tx_eventer_recv_data1\n");
		
	pevnt->status = 1;
	pevnt->recv_type = uiNeedEvent;	
	if (timeout_ms != TX_WAIT_FOREVER)
	{
		clock_gettime(CLOCK_MONOTONIC, &ts);
		ts.tv_nsec = ts.tv_nsec + (timeout_ms % 1000) * 1000000;
		ts.tv_sec += timeout_ms / 1000;
		ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
		ts.tv_nsec %= (1000 * 1000 * 1000);
	}
	
	do
	{
		ret = 0;
		if (timeout_ms != TX_WAIT_FOREVER)
			rt = pthread_cond_timedwait(&pevnt->condition, &pevnt->mutex, &ts);	
			else rt = pthread_cond_wait(&pevnt->condition, &pevnt->mutex);
		if (rt != ETIMEDOUT)
		{
			if (pData != NULL) *pData = pevnt->recv_data; 
			if (uiDataLen != NULL) *uiDataLen = pevnt->recv_datalen; 
			if (pevnt->recv_type == 0) 
			{
				rt = ETIMEDOUT;
				break;
			}
			if (pevnt->recv_type != uiNeedEvent)
			{
				dbgprintf(2, "Wrong Type %i, Wait Other %i\n", pevnt->recv_type, uiNeedEvent);	
				ret = 1;
			}
		} else break;
	} while(ret);
		
	if (rt != ETIMEDOUT) ret = 1; 
	else
	{
		ret = 0;
		pevnt->status = 0;
	}
	
	pevnt->recv_type = 0;	
	pevnt->recv_data = 0;
	pevnt->recv_datalen = 0;
	pthread_mutex_unlock(&pevnt->mutex);
	
	return ret;
}

int tx_eventer_send_data(TX_EVENTER *pevnt, unsigned int uiEventType, void* pData, unsigned int uiDataLen, char cWaitReturn, unsigned int timeout_ms)
{
	pthread_mutex_lock(&pevnt->mutex);
	return tx_eventer_send_data_prelocked(pevnt, uiEventType, pData, uiDataLen, cWaitReturn, timeout_ms);
}

int tx_eventer_send_data_prelocked(TX_EVENTER *pevnt, unsigned int uiEventType, void* pData, unsigned int uiDataLen, char cWaitReturn, unsigned int timeout_ms)
{
	struct timespec  ts;
	int rt = 0;
	
	//pthread_mutex_lock(&pevnt->mutex);
	
	if ((pevnt->status == 0) && (timeout_ms != 0))
	{
		pevnt->status = 1;
		pevnt->recv_type = uiEventType;
		pevnt->recv_data = pData;
		pevnt->recv_datalen = uiDataLen;
		
		if (timeout_ms != TX_WAIT_FOREVER)
		{
			clock_gettime(CLOCK_MONOTONIC, &ts);
			ts.tv_nsec = ts.tv_nsec + (timeout_ms % 1000) * 1000000;
			ts.tv_sec += timeout_ms / 1000;
			ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
			ts.tv_nsec %= (1000 * 1000 * 1000);		
			rt = pthread_cond_timedwait(&pevnt->condition, &pevnt->mutex, &ts);
		}
		else rt = pthread_cond_wait(&pevnt->condition, &pevnt->mutex);
		
		if (rt == ETIMEDOUT)
		{
			pevnt->status = 0;
			pthread_mutex_unlock(&pevnt->mutex);
			return 0;
		}
		if ((cWaitReturn) && (uiEventType != 0)) 
		{
			pevnt->status = 2;		
			if ((timeout_ms != TX_WAIT_FOREVER) && (timeout_ms != 0))
				rt = pthread_cond_timedwait(&pevnt->condition, &pevnt->mutex, &ts);
				else
				rt = pthread_cond_wait(&pevnt->condition, &pevnt->mutex);
			if (rt == ETIMEDOUT) 
			{
				pevnt->status = 0;
				dbgprintf(2, "tx_eventer_send_data:Timeout return data1: %i\n", uiEventType);
			}
		} else pevnt->status = 0;
		pthread_mutex_unlock(&pevnt->mutex);
		return 1;
	}
	
	if ((pevnt->status == 1) && (pevnt->recv_type != 0) && (pevnt->recv_type != uiEventType) && (timeout_ms))
		dbgprintf(2, "tx_eventer_send_data:Wrong data type: need: %i, sending: %i\n", pevnt->recv_type, uiEventType);	
	if ((pevnt->status == 1) && ((pevnt->recv_type == 0) || (pevnt->recv_type == uiEventType)))
	{
		if (pevnt->recv_type == 0) pevnt->recv_type = uiEventType;
		pevnt->recv_data = pData;
		pevnt->recv_datalen = uiDataLen;
		pthread_cond_signal(&pevnt->condition);
		if ((cWaitReturn) && (uiEventType != 0)) 
		{
			pevnt->status = 2;				
			if ((timeout_ms != TX_WAIT_FOREVER) && (timeout_ms != 0))
			{
				clock_gettime(CLOCK_MONOTONIC, &ts);
				ts.tv_nsec = ts.tv_nsec + (timeout_ms % 1000) * 1000000;
				ts.tv_sec += timeout_ms / 1000;
				ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
				ts.tv_nsec %= (1000 * 1000 * 1000);		
				rt = pthread_cond_timedwait(&pevnt->condition, &pevnt->mutex, &ts);
			}
			else rt = pthread_cond_wait(&pevnt->condition, &pevnt->mutex);
			if (rt == ETIMEDOUT) 
			{
				pevnt->status = 0;
				dbgprintf(2, "tx_eventer_send_data:Timeout return data2: %i\n", uiEventType);		
			}
		} 
		else 
		{
			pevnt->status = 0;
		}
					
		pthread_mutex_unlock(&pevnt->mutex);
		return 1;
	}
	
	pthread_mutex_unlock(&pevnt->mutex);
	return 0;
}

// add event in eventlist
int tx_eventer_add_event(TX_EVENTER *pevnt, unsigned int uiEventType)
{
	int ret = 1;
	int i;
	pthread_mutex_lock(&pevnt->mutex);
	for(i = 0; i < pevnt->type_list_cnt;i++)
		if (pevnt->type_list[i].code == uiEventType) break;
	if (i == pevnt->type_list_cnt)
	{
		for(i = 0; i < pevnt->type_list_cnt;i++) 
		{
			if (pevnt->type_list[i].code == 0) 
			{
				pevnt->type_list[i].code = uiEventType;
				pevnt->type_list[i].recved = 0;
				break;
			}
		}
		if (i == pevnt->type_list_cnt)
		{
			pevnt->type_list_cnt++;	
			pevnt->type_list = (EVENT_TYPE_LIST*)realloc(pevnt->type_list, pevnt->type_list_cnt*sizeof(EVENT_TYPE_LIST));
			pevnt->type_list[pevnt->type_list_cnt-1].code = uiEventType;
			pevnt->type_list[pevnt->type_list_cnt-1].recved = 0;
		}
	}
	pthread_mutex_unlock(&pevnt->mutex);
	return ret;
}

// test event in eventlist
int tx_eventer_test_event(TX_EVENTER *pevnt, unsigned int uiEventType)
{
	int ret = 0;
	int i;
	pthread_mutex_lock(&pevnt->mutex);
	for(i = 0; i < pevnt->type_list_cnt;i++)
		if (pevnt->type_list[i].code == uiEventType) 
		{
			ret = 1;
			break;
		}
	pthread_mutex_unlock(&pevnt->mutex);
	return ret;
}

int tx_eventer_count_event(TX_EVENTER *pevnt, unsigned int uiEventType)
{
	int ret = 0;
	int i;
	pthread_mutex_lock(&pevnt->mutex);
	for(i = 0; i < pevnt->type_list_cnt;i++)
		if (pevnt->type_list[i].code == uiEventType) 
		{
			ret++;
		}
	pthread_mutex_unlock(&pevnt->mutex);
	return ret;
}

int tx_semaphore_count_in_list(TX_SEMAPHORE *psem, unsigned int event, unsigned int cmd)
{
	pthread_mutex_lock(&psem->mutex);
	//printf("send cmd %i,%i\n",cmd, psem->list_count+1);
	int i;
	int ret = 0;
	//count = psem->list_count;
	//printf("semval before :%i\n",psem->semval);	
	if (psem->list_count != 0)			
	  for (i=0;i!=TX_MAX_LIST_SIZE;i++)
	  {
		if (((psem->semval_list[i] == cmd) || (cmd == TX_ANY)) && (psem->semval_list2[i] == event))
		{
			ret++;			
		}
	  }
	pthread_mutex_unlock(&psem->mutex);
	return ret;
}

// clear eventlist of spec event
int tx_eventer_delete_event(TX_EVENTER *pevnt, unsigned int uiEventType)
{
	int ret = 1;
	int i;
	
	pthread_mutex_lock(&pevnt->mutex);
	for(i = 0; i < pevnt->type_list_cnt;i++)
		if (pevnt->type_list[i].code == uiEventType)
			pevnt->type_list[i].code = 0;
	pthread_mutex_unlock(&pevnt->mutex);
	return ret;
}

int tx_eventer_clear_event(TX_EVENTER *pevnt, unsigned int uiEventType)
{
	int ret = 1;
	int i;
	
	pthread_mutex_lock(&pevnt->mutex);
	if (pevnt->undelete)
	{
		for(i = 0; i < pevnt->type_list_cnt;i++)
			if (pevnt->type_list[i].code == uiEventType) 
				pevnt->type_list[i].recved = 0;
	}
	else
	{
		for(i = 0; i < pevnt->type_list_cnt;i++)
			if (pevnt->type_list[i].code == uiEventType) 
				pevnt->type_list[i].code = 0;
	}
	pthread_mutex_unlock(&pevnt->mutex);
	return ret;
}

// clear eventlist
int tx_eventer_clear(TX_EVENTER *pevnt)
{
	int ret = 1;
	int i;
	
	pthread_mutex_lock(&pevnt->mutex);
	if (pevnt->undelete)
	{
		for(i = 0; i < pevnt->type_list_cnt;i++)
			if (pevnt->type_list[i].code != 0) 
				pevnt->type_list[i].recved = 0;
	}
	else
	{
		for(i = 0; i < pevnt->type_list_cnt;i++)
			if (pevnt->type_list[i].code != 0) 
				pevnt->type_list[i].code = 0;
	}
	pthread_mutex_unlock(&pevnt->mutex);
	return ret;
}

// print eventlist
int tx_eventer_print_events(TX_EVENTER *pevnt)
{
	int ret = 1;
	int i;
	
	pthread_mutex_lock(&pevnt->mutex);
	for(i = 0; i < pevnt->type_list_cnt;i++)
		printf("tx_events: addr:%i; code:%i; recvd:%i\n",
			(int)(intptr_t)pevnt, pevnt->type_list[i].code, pevnt->type_list[i].recved);
	if (pevnt->type_list_cnt == 0) printf("NO tx_events: addr:%i\n", (int)(intptr_t)pevnt);
	pthread_mutex_unlock(&pevnt->mutex);
	return ret;
}

// return recved data
int tx_eventer_return_data(TX_EVENTER *pevnt, unsigned int timeout_ms)
{
	pthread_mutex_lock(&pevnt->mutex);	
		
	while ((pevnt->status != 2) && (timeout_ms))
	{
		pthread_mutex_unlock(&pevnt->mutex);
		timeout_ms--;
		usleep(1000);		
		pthread_mutex_lock(&pevnt->mutex);
	}
	if (pevnt->status == 2) 
	{
		pevnt->status = 0;
		pthread_cond_signal(&pevnt->condition); 
	} else dbgprintf(2, "timeout send return data\n");
		 
	pthread_mutex_unlock(&pevnt->mutex);
	return timeout_ms;
}

