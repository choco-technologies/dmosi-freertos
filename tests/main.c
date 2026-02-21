#define DMOD_ENABLE_REGISTRATION
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <stdint.h>
#include "dmod.h"
#include "dmosi.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* =========================================================================
 * pvPortMalloc / vPortFree overrides for testing
 *
 * Use plain malloc/free instead of the DMOD-routing implementation in
 * dmosi_heap.c.  This avoids the circular dependency:
 *   pvPortMalloc -> dmosi_thread_get_module_name -> dmosi_thread_current
 *   -> thread_new -> pvPortMalloc -> ...
 * which would cause a stack overflow during dmosi_init().
 * ========================================================================= */
void* pvPortMalloc( size_t size )
{
    return malloc( size );
}

void vPortFree( void* ptr )
{
    free( ptr );
}

size_t xPortGetFreeHeapSize( void )
{
    return 0;
}

size_t xPortGetMinimumEverFreeHeapSize( void )
{
    return 0;
}

void vPortInitialiseBlocks( void )
{
}

/* =========================================================================
 * FreeRTOS application hooks
 * ========================================================================= */

#if ( configCHECK_FOR_STACK_OVERFLOW > 0 )
void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                    char * pcTaskName )
{
    ( void ) xTask;
    printf( "STACK OVERFLOW in task: %s\n", pcTaskName );
    abort();
}
#endif /* configCHECK_FOR_STACK_OVERFLOW */

/* FreeRTOSConfig.h sets configKERNEL_PROVIDED_STATIC_MEMORY=1, so the kernel
 * supplies vApplicationGetIdleTaskMemory and vApplicationGetTimerTaskMemory
 * internally.  No application-level definitions are needed. */

/* Stack depth multiplier for the test task.  A large value is used to
 * accommodate all nested test calls, timer callbacks, and thread operations. */
#define TEST_TASK_STACK_MULTIPLIER    32
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT( condition, test_name )                       \
    do {                                                          \
        if( condition )                                           \
        {                                                         \
            printf( "  \xE2\x9C\x93 PASS: %s\n", test_name );   \
            tests_passed++;                                       \
        }                                                         \
        else                                                      \
        {                                                         \
            printf( "  \xE2\x9C\x97 FAIL: %s\n", test_name );   \
            tests_failed++;                                       \
        }                                                         \
    } while( 0 )

/* =========================================================================
 * Mutex tests
 * ========================================================================= */
static void test_mutex( void )
{
    printf( "\n=== Testing mutex ===\n" );

    /* Non-recursive mutex: create, lock, unlock, destroy */
    dmosi_mutex_t m = dmosi_mutex_create( false );
    TEST_ASSERT( m != NULL, "Create non-recursive mutex" );

    TEST_ASSERT( dmosi_mutex_lock( m ) == 0, "Lock non-recursive mutex" );
    TEST_ASSERT( dmosi_mutex_unlock( m ) == 0, "Unlock non-recursive mutex" );

    dmosi_mutex_destroy( m );
    TEST_ASSERT( true, "Destroy non-recursive mutex" );

    /* Recursive mutex: create, lock twice, unlock twice, destroy */
    dmosi_mutex_t rm = dmosi_mutex_create( true );
    TEST_ASSERT( rm != NULL, "Create recursive mutex" );

    TEST_ASSERT( dmosi_mutex_lock( rm ) == 0, "Lock recursive mutex (1st)" );
    TEST_ASSERT( dmosi_mutex_lock( rm ) == 0, "Lock recursive mutex (2nd)" );
    TEST_ASSERT( dmosi_mutex_unlock( rm ) == 0, "Unlock recursive mutex (1st)" );
    TEST_ASSERT( dmosi_mutex_unlock( rm ) == 0, "Unlock recursive mutex (2nd)" );

    dmosi_mutex_destroy( rm );

    /* NULL input handling */
    TEST_ASSERT( dmosi_mutex_lock( NULL ) == -EINVAL, "Lock NULL mutex returns -EINVAL" );
    TEST_ASSERT( dmosi_mutex_unlock( NULL ) == -EINVAL, "Unlock NULL mutex returns -EINVAL" );
    dmosi_mutex_destroy( NULL );
    TEST_ASSERT( true, "Destroy NULL mutex does not crash" );
}

/* =========================================================================
 * Semaphore tests
 * ========================================================================= */
static void test_semaphore( void )
{
    printf( "\n=== Testing semaphore ===\n" );

    /* Create a counting semaphore with initial_count=1, max_count=5 */
    dmosi_semaphore_t s = dmosi_semaphore_create( 1, 5 );
    TEST_ASSERT( s != NULL, "Create counting semaphore (initial=1, max=5)" );

    /* Decrement count from 1 to 0 */
    TEST_ASSERT( dmosi_semaphore_wait( s, 0 ) == 0,
                 "Wait on semaphore with count=1 succeeds" );

    /* Count is now 0: non-blocking wait must fail with -EAGAIN */
    TEST_ASSERT( dmosi_semaphore_wait( s, 0 ) == -EAGAIN,
                 "Wait on semaphore with count=0, no timeout returns -EAGAIN" );

    /* Post once: count becomes 1 */
    TEST_ASSERT( dmosi_semaphore_post( s ) == 0, "Post to semaphore" );

    /* Wait with short timeout should succeed */
    TEST_ASSERT( dmosi_semaphore_wait( s, 100 ) == 0,
                 "Wait on semaphore with timeout=100ms succeeds" );

    /* Fill to max and verify overflow */
    for( int i = 0; i < 5; i++ )
    {
        dmosi_semaphore_post( s );
    }
    TEST_ASSERT( dmosi_semaphore_post( s ) == -EOVERFLOW,
                 "Post beyond max_count returns -EOVERFLOW" );

    dmosi_semaphore_destroy( s );

    /* Invalid parameters */
    TEST_ASSERT( dmosi_semaphore_create( 0, 0 ) == NULL,
                 "Create semaphore with max_count=0 returns NULL" );
    TEST_ASSERT( dmosi_semaphore_create( 5, 3 ) == NULL,
                 "Create semaphore with initial_count>max_count returns NULL" );

    /* NULL input handling */
    TEST_ASSERT( dmosi_semaphore_wait( NULL, 0 ) == -EINVAL,
                 "Wait on NULL semaphore returns -EINVAL" );
    TEST_ASSERT( dmosi_semaphore_post( NULL ) == -EINVAL,
                 "Post to NULL semaphore returns -EINVAL" );
    dmosi_semaphore_destroy( NULL );
    TEST_ASSERT( true, "Destroy NULL semaphore does not crash" );
}

/* =========================================================================
 * Queue tests
 * ========================================================================= */
static void test_queue( void )
{
    printf( "\n=== Testing queue ===\n" );

    int item = 42;
    int received = 0;

    /* Create queue */
    dmosi_queue_t q = dmosi_queue_create( sizeof( int ), 5 );
    TEST_ASSERT( q != NULL, "Create queue (item_size=4, length=5)" );

    /* Send and receive a single item */
    TEST_ASSERT( dmosi_queue_send( q, &item, 0 ) == 0,
                 "Send item to queue" );
    TEST_ASSERT( dmosi_queue_receive( q, &received, 0 ) == 0,
                 "Receive item from queue" );
    TEST_ASSERT( received == 42, "Received value matches sent value" );

    /* Fill queue and verify full behaviour */
    for( int i = 0; i < 5; i++ )
    {
        dmosi_queue_send( q, &i, 0 );
    }
    TEST_ASSERT( dmosi_queue_send( q, &item, 0 ) == -EAGAIN,
                 "Send to full queue (no timeout) returns -EAGAIN" );

    /* Drain queue and verify empty behaviour */
    for( int i = 0; i < 5; i++ )
    {
        dmosi_queue_receive( q, &received, 0 );
    }
    TEST_ASSERT( dmosi_queue_receive( q, &received, 0 ) == -EAGAIN,
                 "Receive from empty queue (no timeout) returns -EAGAIN" );

    /* NULL item pointer */
    TEST_ASSERT( dmosi_queue_send( q, NULL, 0 ) == -EINVAL,
                 "Send NULL item returns -EINVAL" );
    TEST_ASSERT( dmosi_queue_receive( q, NULL, 0 ) == -EINVAL,
                 "Receive into NULL buffer returns -EINVAL" );

    dmosi_queue_destroy( q );

    /* Invalid parameters */
    TEST_ASSERT( dmosi_queue_create( 0, 5 ) == NULL,
                 "Create queue with item_size=0 returns NULL" );
    TEST_ASSERT( dmosi_queue_create( sizeof( int ), 0 ) == NULL,
                 "Create queue with queue_length=0 returns NULL" );

    /* NULL queue handle */
    TEST_ASSERT( dmosi_queue_send( NULL, &item, 0 ) == -EINVAL,
                 "Send to NULL queue returns -EINVAL" );
    TEST_ASSERT( dmosi_queue_receive( NULL, &received, 0 ) == -EINVAL,
                 "Receive from NULL queue returns -EINVAL" );
    dmosi_queue_destroy( NULL );
    TEST_ASSERT( true, "Destroy NULL queue does not crash" );
}

/* =========================================================================
 * Timer tests
 * ========================================================================= */
static volatile int g_timer_callback_count = 0;

static void timer_callback( void * arg )
{
    ( void ) arg;
    g_timer_callback_count++;
}

static void test_timer( void )
{
    printf( "\n=== Testing timer ===\n" );

    /* Create a one-shot timer (should not crash even if never started) */
    dmosi_timer_t one_shot = dmosi_timer_create( timer_callback, NULL, 100, false );
    TEST_ASSERT( one_shot != NULL, "Create one-shot timer" );
    dmosi_timer_destroy( one_shot );

    /* Create an auto-reload timer */
    g_timer_callback_count = 0;
    dmosi_timer_t t = dmosi_timer_create( timer_callback, NULL, 50, true );
    TEST_ASSERT( t != NULL, "Create auto-reload timer (period=50ms)" );

    /* Start the timer */
    TEST_ASSERT( dmosi_timer_start( t ) == 0, "Start timer" );

    /* Wait long enough for several callbacks to fire (~200ms = 4 periods) */
    vTaskDelay( pdMS_TO_TICKS( 200 ) );
    TEST_ASSERT( g_timer_callback_count >= 2,
                 "Auto-reload timer fires at least twice in 200ms" );

    /* Stop the timer and verify no further callbacks */
    TEST_ASSERT( dmosi_timer_stop( t ) == 0, "Stop timer" );
    int count_after_stop = g_timer_callback_count;
    vTaskDelay( pdMS_TO_TICKS( 100 ) );
    TEST_ASSERT( g_timer_callback_count == count_after_stop,
                 "Stopped timer does not fire any more" );

    /* Reset the timer (starts it again) */
    TEST_ASSERT( dmosi_timer_reset( t ) == 0, "Reset timer" );
    vTaskDelay( pdMS_TO_TICKS( 200 ) );
    TEST_ASSERT( g_timer_callback_count > count_after_stop,
                 "Timer fires again after reset" );

    /* Change period */
    TEST_ASSERT( dmosi_timer_set_period( t, 100 ) == 0,
                 "Change timer period to 100ms" );
    TEST_ASSERT( dmosi_timer_get_period( t ) == 100,
                 "Get timer period returns 100ms after change" );

    TEST_ASSERT( dmosi_timer_stop( t ) == 0, "Stop timer before destroy" );
    dmosi_timer_destroy( t );

    /* Invalid parameters */
    TEST_ASSERT( dmosi_timer_create( NULL, NULL, 100, false ) == NULL,
                 "Create timer with NULL callback returns NULL" );
    TEST_ASSERT( dmosi_timer_create( timer_callback, NULL, 0, false ) == NULL,
                 "Create timer with period_ms=0 returns NULL" );

    /* NULL handle */
    TEST_ASSERT( dmosi_timer_start( NULL ) == -EINVAL,
                 "Start NULL timer returns -EINVAL" );
    TEST_ASSERT( dmosi_timer_stop( NULL ) == -EINVAL,
                 "Stop NULL timer returns -EINVAL" );
    TEST_ASSERT( dmosi_timer_reset( NULL ) == -EINVAL,
                 "Reset NULL timer returns -EINVAL" );
    TEST_ASSERT( dmosi_timer_set_period( NULL, 100 ) == -EINVAL,
                 "Set period on NULL timer returns -EINVAL" );
    TEST_ASSERT( dmosi_timer_get_period( NULL ) == 0,
                 "Get period of NULL timer returns 0" );
    dmosi_timer_destroy( NULL );
    TEST_ASSERT( true, "Destroy NULL timer does not crash" );
}

/* =========================================================================
 * Thread tests
 * ========================================================================= */
static volatile bool g_thread_ran = false;

static void simple_thread_entry( void * arg )
{
    ( void ) arg;
    g_thread_ran = true;
    /* Thread exits here; thread_wrapper handles cleanup */
}

static void slow_thread_entry( void * arg )
{
    ( void ) arg;
    /* Delay "forever" – used to test kill */
    vTaskDelay( portMAX_DELAY );
}

static void test_thread( void )
{
    printf( "\n=== Testing thread ===\n" );

    /* Current thread */
    dmosi_thread_t current = dmosi_thread_current();
    TEST_ASSERT( current != NULL, "Get current thread returns non-NULL" );

    /* Thread name (test task was created with name "tests") */
    const char * name = dmosi_thread_get_name( current );
    TEST_ASSERT( name != NULL, "Get current thread name returns non-NULL" );

    /* Current thread name when passing NULL (returns current thread's name) */
    const char * name_null = dmosi_thread_get_name( NULL );
    TEST_ASSERT( name_null != NULL,
                 "Get thread name with NULL handle returns non-NULL" );

    /* Thread priority */
    int prio = dmosi_thread_get_priority( current );
    TEST_ASSERT( prio >= 0, "Get current thread priority >= 0" );

    /* Thread's process */
    dmosi_process_t proc = dmosi_thread_get_process( current );
    TEST_ASSERT( proc != NULL, "Get current thread's process returns non-NULL" );

    /* Thread module name */
    const char * mod = dmosi_thread_get_module_name( current );
    TEST_ASSERT( mod != NULL, "Get current thread module name returns non-NULL" );

    /* Thread sleep (just verify it doesn't crash) */
    dmosi_thread_sleep( 10 );
    TEST_ASSERT( true, "Thread sleep(10ms) does not crash" );

    /* Create a thread, wait for it to complete, then join it */
    g_thread_ran = false;
    dmosi_thread_t t = dmosi_thread_create(
        simple_thread_entry, NULL, 1, 4096, "simple_t", NULL );
    TEST_ASSERT( t != NULL, "Create simple thread" );

    TEST_ASSERT( dmosi_thread_join( t ) == 0, "Join completed thread returns 0" );
    TEST_ASSERT( g_thread_ran == true, "Thread entry function was executed" );
    dmosi_thread_destroy( t );

    /* Double join must fail */
    TEST_ASSERT( dmosi_thread_join( t ) == -EINVAL,
                 "Join already-joined thread returns -EINVAL" );

    /* Thread kill: create a slow thread, kill it, then join */
    dmosi_thread_t slow = dmosi_thread_create(
        slow_thread_entry, NULL, 1, 4096, "slow_t", NULL );
    TEST_ASSERT( slow != NULL, "Create slow thread for kill test" );
    TEST_ASSERT( dmosi_thread_kill( slow, 0 ) == 0,
                 "Kill running thread returns 0" );
    /* After kill the thread is marked completed; join must return immediately */
    TEST_ASSERT( dmosi_thread_join( slow ) == 0,
                 "Join killed thread returns 0" );
    dmosi_thread_destroy( slow );

    /* Get all threads count */
    size_t count = dmosi_thread_get_all( NULL, 0 );
    TEST_ASSERT( count >= 1, "thread_get_all count >= 1" );

    /* Get threads by process */
    size_t proc_count = dmosi_thread_get_by_process( proc, NULL, 0 );
    TEST_ASSERT( proc_count >= 1, "thread_get_by_process count >= 1" );

    /* NULL input handling */
    TEST_ASSERT( dmosi_thread_join( NULL ) == -EINVAL,
                 "Join NULL thread returns -EINVAL" );
    TEST_ASSERT( dmosi_thread_kill( NULL, 0 ) == -EINVAL,
                 "Kill NULL thread returns -EINVAL" );
    TEST_ASSERT( dmosi_thread_get_name( NULL ) != NULL,
                 "Get name with NULL (returns current thread name)" );
    TEST_ASSERT( dmosi_thread_get_priority( NULL ) >= 0,
                 "Get priority with NULL (returns current thread priority)" );
    TEST_ASSERT( dmosi_thread_get_process( NULL ) != NULL,
                 "Get process with NULL (returns current thread's process)" );
    TEST_ASSERT( dmosi_thread_get_module_name( NULL ) != NULL,
                 "Get module name with NULL (returns current thread's module name)" );
    dmosi_thread_destroy( NULL );
    TEST_ASSERT( true, "Destroy NULL thread does not crash" );
}

/* =========================================================================
 * Init / deinit tests
 * ========================================================================= */
static void test_init_deinit( void )
{
    printf( "\n=== Testing init / deinit ===\n" );

    /* Calling init again on an already-initialised system must fail */
    TEST_ASSERT( dmosi_init() == false, "Double dmosi_init() returns false" );

    /* Calling deinit when not initialised (after deinit) must succeed */
    TEST_ASSERT( dmosi_deinit() == true, "dmosi_deinit() returns true" );
    /* Re-initialise so the rest of the task teardown works correctly */
    TEST_ASSERT( dmosi_init() == true, "dmosi_init() succeeds after deinit" );
}

/* =========================================================================
 * Test task
 * ========================================================================= */
static int g_test_result = 0;

static void test_task( void * pvParameters )
{
    ( void ) pvParameters;

    printf( "========================================\n" );
    printf( "  DMOSI FreeRTOS Implementation Tests\n" );
    printf( "========================================\n" );

    test_mutex();
    test_semaphore();
    test_queue();
    test_timer();
    test_thread();
    test_init_deinit();

    printf( "\n========================================\n" );
    printf( "  Test Summary\n" );
    printf( "========================================\n" );
    printf( "Total tests: %d\n", tests_passed + tests_failed );
    printf( "Passed:      %d\n", tests_passed );
    printf( "Failed:      %d\n", tests_failed );
    printf( "========================================\n" );

    if( tests_failed == 0 )
    {
        printf( "\n\xE2\x9C\x93 ALL TESTS PASSED\n\n" );
        g_test_result = 0;
    }
    else
    {
        printf( "\n\xE2\x9C\x97 SOME TESTS FAILED\n\n" );
        g_test_result = 1;
    }

    /* dmosi_deinit() cleans up all DMOSI resources and stops the scheduler,
     * causing dmosi_init() in main() to return. */
    dmosi_deinit();
    vTaskEndScheduler();
    vTaskDelete( NULL );
}

/* =========================================================================
 * main
 * ========================================================================= */
int main( void )
{
    xTaskCreate( test_task,
                 "tests",
                 configMINIMAL_STACK_SIZE * TEST_TASK_STACK_MULTIPLIER,
                 NULL,
                 configMAX_PRIORITIES - 2,
                 NULL );

    /* dmosi_init() creates the system process and starts the FreeRTOS
     * scheduler.  It blocks here until vTaskEndScheduler() is called.
     * test_task calls dmosi_deinit() followed by vTaskEndScheduler() to
     * stop the scheduler and allow this call to return. */
    if( !dmosi_init() )
    {
        printf( "ERROR: dmosi_init() failed\n" );
        return 1;
    }

    return g_test_result;
}
