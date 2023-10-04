#ifndef _PTHREAD2THREADX_H_
#define _PTHREAD2THREADX_H_

#include <pthread.h>

#define TX_NO_WAIT          0
#define TX_WAIT_FOREVER     0xFFFFFFFFUL
#define TX_MAX_LIST_SIZE	32

//#define tx_semaphore_wait_event_timeout(x,y,z,m)		tx_semaphore_wait_event_timeout3(x,y,z,m, __FILE__, __func__, __LINE__)

#define	TX_ANY		0X7FFFFFFE
#define	TX_RESET	0X7FFFFFF0

typedef struct TX_SEMAPHORE
{
	pthread_condattr_t condattr;
    pthread_cond_t condition;
    pthread_mutex_t mutex;
    unsigned int error;
    unsigned int semval;
    unsigned int semval_list[TX_MAX_LIST_SIZE];
    unsigned int semval_list2[TX_MAX_LIST_SIZE];
    unsigned int reset;
    unsigned int special;
    unsigned int special_list[TX_MAX_LIST_SIZE];
   // unsigned int semval_list_ready[TX_MAX_LIST_SIZE];
    unsigned int list_count;
} TX_SEMAPHORE;

typedef struct EVENT_TYPE_LIST
{
	unsigned int 	code;
	unsigned int	recved;
} EVENT_TYPE_LIST;

typedef struct TX_EVENTER
{
	pthread_condattr_t 	condattr;
    pthread_cond_t 		condition;
    pthread_mutex_t 	mutex;
	char				status;
	unsigned int		recv_type;
	void*				recv_data;
	unsigned int		recv_datalen;
	EVENT_TYPE_LIST*	type_list;
	unsigned int		type_list_cnt;
	char 				undelete;	
} TX_EVENTER;

typedef struct EVENT_LIST
{
	unsigned int event;
    unsigned int cmd;
} EVENT_LIST;


int tx_semaphore_create(TX_SEMAPHORE *psem, void *attr, unsigned int val);
int tx_semaphore_put(TX_SEMAPHORE *psem);
int tx_semaphore_get(TX_SEMAPHORE *psem, unsigned int waitmode);
int tx_semaphore_reset(TX_SEMAPHORE *psem);
int tx_semaphore_add(TX_SEMAPHORE *psem);
int tx_semaphore_add_in_list(TX_SEMAPHORE *psem, unsigned int event, unsigned int cmd);
int tx_semaphore_go(TX_SEMAPHORE *psem, unsigned int event, unsigned int cmd);
int tx_semaphore_wait(TX_SEMAPHORE *psem);
int tx_semaphore_wait_spec(TX_SEMAPHORE *psem);
int tx_semaphore_wait_list_empty(TX_SEMAPHORE *psem);
int tx_semaphore_wait_spec_list_empty(TX_SEMAPHORE *psem);
int tx_semaphore_delete(TX_SEMAPHORE *psem);
int tx_semaphore_delete_from_list(TX_SEMAPHORE *psem, unsigned int event, unsigned int cmd);
int tx_semaphore_count_in_list(TX_SEMAPHORE *psem, unsigned int event, unsigned int cmd);
int tx_semaphore_add_in_list_spec(TX_SEMAPHORE *psem, unsigned int event, unsigned int cmd, int NotLock);
int tx_semaphore_print_list(TX_SEMAPHORE *psem);
int tx_semaphore_wait_timeout(TX_SEMAPHORE *psem, int timeout_ms);
int tx_semaphore_wait_list_any_timeout(TX_SEMAPHORE *psem, EVENT_LIST * pEventList, int iEvListlen, int timeout_ms);
int tx_semaphore_exist_in_list(TX_SEMAPHORE *psem, unsigned int event, unsigned int cmd);
//int tx_semaphore_wait_event(TX_SEMAPHORE *psem, unsigned int event, unsigned int cmd);
//int tx_semaphore_wait_event_timeout3(TX_SEMAPHORE *psem, unsigned int event, unsigned int cmd, int timeout_ms, char *pFileName, const char *pFuncName, int iLine);
int tx_semaphore_wait_event_timeout(TX_SEMAPHORE *psem, unsigned int event, unsigned int cmd, int timeout_ms);

int tx_eventer_create(TX_EVENTER *pevnt, char cUnDelete);
int tx_eventer_delete(TX_EVENTER *pevnt);
int tx_eventer_clear_event(TX_EVENTER *pevnt, unsigned int uiEventType);
int tx_eventer_clear(TX_EVENTER *pevnt);
int tx_eventer_add_event(TX_EVENTER *pevnt, unsigned int uiEventType);
int tx_eventer_delete_event(TX_EVENTER *pevnt, unsigned int uiEventType);
int tx_eventer_test_event(TX_EVENTER *pevnt, unsigned int uiEventType);
int tx_eventer_count_event(TX_EVENTER *pevnt, unsigned int uiEventType);
int tx_eventer_print_events(TX_EVENTER *pevnt);
int tx_eventer_return_data(TX_EVENTER *pevnt, unsigned int timeout_ms);
int tx_eventer_send_data(TX_EVENTER *pevnt, unsigned int uiEventType, void* pData, unsigned int uiDataLen, char cWaitReturn, unsigned int timeout_ms);
int tx_eventer_send_data_prelocked(TX_EVENTER *pevnt, unsigned int uiEventType, void* pData, unsigned int uiDataLen, char cWaitReturn, unsigned int timeout_ms);
int tx_eventer_send_event(TX_EVENTER *pevnt, unsigned int uiEventType);
unsigned int tx_eventer_recv_data(TX_EVENTER *pevnt, unsigned int uiType, void** pData, unsigned int *uiDataLen, unsigned int timeout_ms);
unsigned int tx_eventer_recv_data_prelocked(TX_EVENTER *pevnt, unsigned int uiNeedEvent, void** pData, unsigned int *uiDataLen, unsigned int timeout_ms);
unsigned int tx_eventer_recv(TX_EVENTER *pevnt, unsigned int *uiType, unsigned int timeout_ms, char cWaitAll);
unsigned int tx_eventer_recv_event(TX_EVENTER *pevnt, unsigned int uiType, unsigned int timeout_ms);

#endif