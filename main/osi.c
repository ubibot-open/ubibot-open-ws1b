/*******************************************************************************
  * @file     
  * @author 
  * @version
  * @date 
  * @brief
  ******************************************************************************
  * @attention
  *
  *
*******************************************************************************/  

/*-------------------------------- Includes ----------------------------------*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "osi.h"
#include "string.h"

portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

/**
 * @brief  Create a sync object (implemented using a binary semaphore) for synchronization between threads, or between a thread and an interrupt service routine
 * @param  pSyncObj Pointer to the sync object control block; on success, the created semaphore handle is output through this pointer
 * @return Returns OSI_OK on success; OSI_INVALID_PARAMS if the parameter is NULL; OSI_OPERATION_FAILED if semaphore creation fails
 */
OsiReturnVal_e osi_SyncObjCreate(OsiSyncObj_t* pSyncObj)
{
	//Check for NULL
	if(NULL == pSyncObj)
	{
		return OSI_INVALID_PARAMS;
	}
	SemaphoreHandle_t *pl_SyncObj = (SemaphoreHandle_t *)pSyncObj;

	*pl_SyncObj = xSemaphoreCreateBinary();

	if((SemaphoreHandle_t)(*pSyncObj) != NULL)
	{
		return OSI_OK; 
	}
	else
	{
		return OSI_OPERATION_FAILED;
	}
}

/**
 * @brief  Delete the specified sync object and release the semaphore resource it holds
 * @param  pSyncObj Pointer to the sync object control block; *pSyncObj is the semaphore handle to be deleted
 * @return Returns OSI_OK on success; OSI_INVALID_PARAMS if the parameter is NULL
 */
OsiReturnVal_e osi_SyncObjDelete(OsiSyncObj_t* pSyncObj)
{
	//Check for NULL
	if(NULL == pSyncObj)
	{
		return OSI_INVALID_PARAMS;
	}
	vSemaphoreDelete(*pSyncObj );
	
	return OSI_OK;
}

/**
 * @brief  Send a sync signal to the specified sync object, waking all threads currently waiting on it
 * @param  pSyncObj Pointer to the sync object control block; *pSyncObj is the semaphore handle to be given
 * @return Returns OSI_OK on success; OSI_INVALID_PARAMS if the parameter is NULL; also returns OSI_OK if the semaphore is given more than once
 */
OsiReturnVal_e osi_SyncObjSignal(OsiSyncObj_t* pSyncObj)
{
	//Check for NULL
	if(NULL == pSyncObj)
	{
		return OSI_INVALID_PARAMS;
	}

	if(pdTRUE != xSemaphoreGive( *pSyncObj ))
	{
		//In case of Semaphore, you are expected to get this if multiple sem
		// give is called before sem take
		return OSI_OK;
	}
	
	return OSI_OK;
}

/**
 * @brief  Send a sync signal to the specified sync object from an interrupt service routine (ISR) context, waking all threads waiting on it, and triggering a task switch if necessary
 * @param  pSyncObj Pointer to the sync object control block; *pSyncObj is the semaphore handle to be given
 * @return Returns OSI_OK on success; OSI_INVALID_PARAMS if the parameter is NULL
 */
OsiReturnVal_e osi_SyncObjSignalFromISR(OsiSyncObj_t* pSyncObj)
{
	//Check for NULL
	if(NULL == pSyncObj)
	{
		return OSI_INVALID_PARAMS;
	}
	xHigherPriorityTaskWoken = pdFALSE;
	if(pdTRUE == xSemaphoreGiveFromISR( *pSyncObj, &xHigherPriorityTaskWoken ))
	{
		if( xHigherPriorityTaskWoken )
		{
			taskYIELD ();
		}
		return OSI_OK;
	}
	else
	{
		//In case of Semaphore, you are expected to get this if multiple sem
		// give is called before sem take
		return OSI_OK;
	}
}

/**
 * @brief  Wait for a sync signal on the specified sync object, until the signal is received or the wait times out
 * @param  pSyncObj Pointer to the sync object control block; *pSyncObj is the semaphore handle to wait on
 * @param  Timeout Maximum wait time in milliseconds; may be OSI_WAIT_FOREVER (wait indefinitely) or OSI_NO_WAIT (don't wait)
 * @return Returns OSI_OK if the signal is received successfully; OSI_INVALID_PARAMS if the parameter is NULL; OSI_OPERATION_FAILED if the wait times out without receiving the signal
 */
OsiReturnVal_e osi_SyncObjWait(OsiSyncObj_t* pSyncObj , OsiTime_t Timeout)
{
    //Check for NULL
    if(NULL == pSyncObj)
    {
            return OSI_INVALID_PARAMS;
    }
    if(pdTRUE == xSemaphoreTake( (SemaphoreHandle_t)*pSyncObj, ( TickType_t )(Timeout/portTICK_PERIOD_MS) ))
    {
        return OSI_OK;
    }
    else
    {
        return OSI_OPERATION_FAILED;
    }
}

/**
 * @brief  Clear the current signal state of the sync object (attempts to take the signal once in non-blocking mode, resetting it to the un-signaled state)
 * @param  pSyncObj Pointer to the sync object control block
 * @return Returns OSI_OK on successful clear; OSI_INVALID_PARAMS if the parameter is NULL; OSI_OPERATION_FAILED if the clear fails
 */
OsiReturnVal_e osi_SyncObjClear(OsiSyncObj_t* pSyncObj)
{
	//Check for NULL
	if(NULL == pSyncObj)
	{
		return OSI_INVALID_PARAMS;
	}

    if (OSI_OK == osi_SyncObjWait(pSyncObj,0) )
    {
        return OSI_OK;
    }
    else
    {
        return OSI_OPERATION_FAILED;
    }
}

/**
 * @brief  Create a mutex lock object (implemented using a mutex semaphore) to protect resources shared between multiple threads
 * @param  pLockObj Pointer to the lock object control block; on success, the created mutex handle is output through this pointer
 * @return Returns OSI_OK on success; OSI_INVALID_PARAMS if the parameter is NULL; OSI_OPERATION_FAILED if creation fails
 */
OsiReturnVal_e osi_LockObjCreate(OsiLockObj_t* pLockObj)
{
	//Check for NULL
	if(NULL == pLockObj)
	{
		return OSI_INVALID_PARAMS;
	}
	*pLockObj = (OsiLockObj_t)xSemaphoreCreateMutex();
	if(pLockObj != NULL)
	{  
		return OSI_OK;
	}
	else
	{
		return OSI_OPERATION_FAILED;
	}
}

/**
 * @brief  Delete the specified mutex lock object and release the mutex semaphore resource it holds
 * @param  pLockObj Pointer to the lock object control block; *pLockObj is the mutex handle to be deleted
 * @return Always returns OSI_OK
 */
OsiReturnVal_e osi_LockObjDelete(OsiLockObj_t* pLockObj)
{
    vSemaphoreDelete((SemaphoreHandle_t)*pLockObj );
    return OSI_OK;
}

/**
 * @brief  Lock the specified mutex lock object; if the lock is already held by another thread, the current thread blocks and waits until the lock is acquired or the wait times out
 * @param  pLockObj Pointer to the lock object control block; *pLockObj is the mutex handle to be acquired
 * @param  Timeout Maximum wait time in milliseconds; may be OSI_WAIT_FOREVER (wait indefinitely) or OSI_NO_WAIT (don't wait)
 * @return Returns OSI_OK if the lock is acquired successfully; OSI_INVALID_PARAMS if the parameter is NULL; OSI_OPERATION_FAILED if the wait times out without acquiring the lock
 */
OsiReturnVal_e osi_LockObjLock(OsiLockObj_t* pLockObj , OsiTime_t Timeout)
{
	//Check for NULL
	if(NULL == pLockObj)
	{
		return OSI_INVALID_PARAMS;
	}
	//Take Semaphore
	if(pdTRUE == xSemaphoreTake( (SemaphoreHandle_t)*pLockObj, ( TickType_t ) (Timeout/portTICK_PERIOD_MS) ))
	{
		return OSI_OK;
	}
	else
	{
		return OSI_OPERATION_FAILED;
	}
}

/**
 * @brief  Release (unlock) the specified mutex lock object
 * @param  pLockObj Pointer to the lock object control block; *pLockObj is the mutex handle to be released
 * @return Returns OSI_OK on success; OSI_INVALID_PARAMS if the parameter is NULL; OSI_OPERATION_FAILED if the release fails
 */
OsiReturnVal_e osi_LockObjUnlock(OsiLockObj_t* pLockObj)
{
	//Check for NULL
	if(NULL == pLockObj)
	{
		return OSI_INVALID_PARAMS;
	}
	//Release Semaphore
	if(pdTRUE == xSemaphoreGive( (SemaphoreHandle_t)*pLockObj ))
	{
		return OSI_OK;
	}
	else
	{
		return OSI_OPERATION_FAILED;
	}
}

/**
 * @brief  Create a new task and add it to the ready task list
 * @param  pEntry Pointer to the task entry function
 * @param  pcName Task name string
 * @param  usStackDepth Task stack size in bytes; internally converted to a depth measured in stack words before being passed to the underlying API
 * @param  pvParameters Pointer to the parameter passed to the task entry function
 * @param  uxPriority Task priority
 * @param  pTaskHandle Output parameter used to return the task handle on successful creation
 * @return Returns OSI_OK on success; OSI_OPERATION_FAILED if creation fails
 */
OsiReturnVal_e osi_TaskCreate(P_OSI_TASK_ENTRY pEntry,const signed char * const pcName,
                              unsigned short usStackDepth, void *pvParameters,
                              unsigned long uxPriority,OsiTaskHandle* pTaskHandle)
{
	if(pdPASS == xTaskCreate( pEntry, (char const*)pcName,
                                (usStackDepth/(sizeof( portSTACK_TYPE ))), 
                                pvParameters,(unsigned portBASE_TYPE)uxPriority,
                                (TaskHandle_t*)pTaskHandle ))
	{
		return OSI_OK;
	}

	return OSI_OPERATION_FAILED;	
}


/**
 * @brief  Delete the specified task, removing it from the running task list
 * @param  pTaskHandle Pointer to the task handle; *pTaskHandle is the task handle to be deleted
 */
void osi_TaskDelete(OsiTaskHandle* pTaskHandle)
{
	vTaskDelete((TaskHandle_t)*pTaskHandle);
}

/**
 * @brief  Create a message queue for inter-thread communication; all required memory is allocated at once based on the message size and the maximum number of messages
 * @param  pMsgQ Pointer to the message queue control block; on success, the queue handle is output through this pointer
 * @param  pMsgQName Message queue name string (unused in the current implementation)
 * @param  MsgSize Size of a single message, in bytes
 * @param  MaxMsgs Maximum number of messages the queue can hold
 * @return Returns OSI_OK on success; OSI_INVALID_PARAMS if the parameter is NULL; OSI_OPERATION_FAILED if creation fails
 */
OsiReturnVal_e osi_MsgQCreate(OsiMsgQ_t* 		pMsgQ ,
			      char*			pMsgQName,
			      unsigned long 		MsgSize,
			      unsigned long		MaxMsgs)
{
	//Check for NULL
	if(NULL == pMsgQ)
	{
		return OSI_INVALID_PARAMS;
	}

	QueueHandle_t handle =0;

	//Create Queue
	handle = xQueueCreate( MaxMsgs, MsgSize );
	if (handle==0)
	{
		return OSI_OPERATION_FAILED;
	}

	*pMsgQ = (OsiMsgQ_t)handle;
	return OSI_OK;
}

/**
 * @brief  Delete the specified message queue and release the resources it holds; all threads currently waiting on the queue for messages will be woken
 * @param  pMsgQ Pointer to the message queue control block; *pMsgQ is the queue handle to be deleted
 * @return Returns OSI_OK on success; OSI_INVALID_PARAMS if the parameter is NULL
 */
OsiReturnVal_e osi_MsgQDelete(OsiMsgQ_t* pMsgQ)
{
	//Check for NULL
	if(NULL == pMsgQ)
	{
		return OSI_INVALID_PARAMS;
	}
	vQueueDelete((QueueHandle_t) *pMsgQ );
    return OSI_OK;
}

/**
 * @brief  Write a message to the specified message queue; the message content is copied from the memory region pointed to by pMsg into the queue. Can be called from interrupt context when Timeout is OSI_NO_WAIT
 * @param  pMsgQ Pointer to the message queue control block
 * @param  pMsg Pointer to the message content to be written
 * @param  Timeout Maximum wait time in milliseconds when the queue has insufficient space
 * @return Returns OSI_OK on success; OSI_INVALID_PARAMS if the parameter is NULL; OSI_OPERATION_FAILED if the wait times out or the write fails
 */

OsiReturnVal_e osi_MsgQWrite(OsiMsgQ_t* pMsgQ, void* pMsg , OsiTime_t Timeout)
{
	//Check for NULL
	if(NULL == pMsgQ)
	{
		return OSI_INVALID_PARAMS;
	}

    // if(pdPASS == xQueueSendFromISR((QueueHandle_t) *pMsgQ, pMsg, &xHigherPriorityTaskWoken ))
	if(pdPASS == xQueueSend((QueueHandle_t) *pMsgQ, pMsg, ( TickType_t ) (Timeout/portTICK_PERIOD_MS) ))
    {
		taskYIELD ();
		return OSI_OK;
    }
	else
	{
		return OSI_OPERATION_FAILED;
	}
}
/**
 * @brief  Read a message from the specified message queue; the message content read is copied into the memory region pointed to by pMsg
 * @param  pMsgQ Pointer to the message queue control block
 * @param  pMsg Output parameter pointing to the memory region used to store the message content read
 * @param  Timeout Maximum wait time in milliseconds when the queue is empty; if OSI_WAIT_FOREVER, waits indefinitely until a message is available
 * @return Returns OSI_OK on success; OSI_INVALID_PARAMS if the parameter is NULL; OSI_OPERATION_FAILED if the wait times out or the read fails
 */

OsiReturnVal_e osi_MsgQRead(OsiMsgQ_t* pMsgQ, void* pMsg , OsiTime_t Timeout)
{
	//Check for NULL
	if(NULL == pMsgQ)
	{
		return OSI_INVALID_PARAMS;
	}

	if ( Timeout == OSI_WAIT_FOREVER )
	{
		Timeout = portMAX_DELAY ;
	}

	//Receive Item from Queue
	if( pdTRUE  == xQueueReceive((QueueHandle_t)*pMsgQ,pMsg,( TickType_t ) (Timeout/portTICK_PERIOD_MS)) )
	{
		return OSI_OK;
	}
	else
	{
		return OSI_OPERATION_FAILED;
	}
}

/**
 * @brief  Suspend the scheduler, pausing task-switching for all tasks
 * @return Always returns OSI_OK, which can be passed as the suspend key to a subsequent call to osi_TaskEnable (the current implementation does not use this return value to track nesting depth)
 */
unsigned long osi_TaskDisable(void)
{
   vTaskSuspendAll();

   return OSI_OK;
}

/**
 * @brief  Resume the scheduler, allowing all tasks to be scheduled again
 * @param  key Suspend key returned by osi_TaskDisable (unused in the current implementation)
 */
void osi_TaskEnable(unsigned long key)
{
   xTaskResumeAll();
}

/**
 * @brief  Enter a critical section, preventing task switches or interrupts from disrupting access to the protected resource
 * @return Always returns 0 (the current implementation does not use this return value to track critical-section nesting depth)
 */
unsigned long osi_EnterCritical(void)
{
     vPortEnterCritical();
    return 0;
}

/**
 * @brief  Exit the critical section, restoring normal access to the previously protected resource
 * @param  ulKey Key returned by osi_EnterCritical when entering the critical section (unused in the current implementation)
 */

void osi_ExitCritical(unsigned long ulKey)
{
     vPortExitCritical();
}

/**
 * @brief  Start the FreeRTOS task scheduler and begin multitasking
 */
void osi_start(void)
{
	vTaskStartScheduler();
}

/**
 * @brief  Suspend (delay) the current task for the specified number of milliseconds
 * @param  MilliSecs Time to suspend and wait, in milliseconds
 */
void osi_Sleep(unsigned int MilliSecs)
{
	TickType_t xDelay = MilliSecs / portTICK_PERIOD_MS;
	vTaskDelay(xDelay);
}

/**
 * @brief  Call the FreeRTOS memory allocation API to allocate a block of the given size on the heap
 * @param  Size Size of the memory to allocate, in bytes
 * @return Returns a pointer to the allocated memory on success; returns NULL on failure
 */

void * mem_Malloc(unsigned long Size)
{
	return ( void * ) pvPortMalloc( (size_t)Size );
}

/**
 * @brief  Call the FreeRTOS memory free API to release previously allocated memory
 * @param  pMem Pointer to the memory block to be freed
 */
void mem_Free(void *pMem)
{
	vPortFree( pMem );
}

/**
 * @brief  Fill the specified memory region with the given value (wraps the standard library memset function)
 * @param  pBuf Pointer to the memory region to fill
 * @param  Val Value used to fill the memory
 * @param  Size Size of the memory to fill, in bytes
 */

void  mem_set(void *pBuf,int Val,size_t Size)
{
	memset( pBuf,Val,Size);
}

/**
 * @brief  Copy data from the source memory region to the destination memory region (wraps the standard library memcpy function)
 * @param  pDst Pointer to the destination memory region
 * @param  pSrc Pointer to the source memory region
 * @param  Size Size of the memory to copy, in bytes
 */
void  mem_copy(void *pDst, void *pSrc,size_t Size)
{
    memcpy(pDst,pSrc,Size);
}

/*******************************************************************************
                                      END         
*******************************************************************************/





