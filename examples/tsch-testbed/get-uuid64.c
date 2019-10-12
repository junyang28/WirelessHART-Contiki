#include "contiki.h"
#include "net/linkaddr.h"
#include "node-id.h"
#include "sys/etimer.h"
#include "net/netstack.h"

#include <stdio.h> /* For printf() */

/*---------------------------------------------------------------------------*/
PROCESS(test_testbed_process, "Test Testbed process");
AUTOSTART_PROCESSES(&test_testbed_process);

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_testbed_process, ev, data)
{
  static int cnt = 0;
  static struct etimer et;

  PROCESS_BEGIN();

  node_id_restore();
  etimer_set(&et, CLOCK_SECOND * 1);
  PROCESS_WAIT_UNTIL(etimer_expired(&et));
  printf("Starting process\n");
  
  while(1) {
    int i;

    printf("Node id: %u, Rime address: ", node_id);
    for(i=0; i<7; i++) {
      printf("%02x:", linkaddr_node_addr.u8[i]);
    }
    printf("%02x (cnt %u)\n", linkaddr_node_addr.u8[7], cnt++);

    etimer_reset(&et);
    PROCESS_WAIT_UNTIL(etimer_expired(&et));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
