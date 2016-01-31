#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "contiki-net.h"
#include "dev/battery-sensor.h"
#define H_PREFIX "ZBXD\x01"

#define K_PING "agent.ping"
#define K_AGENT_NAME "agent.name"
#define K_AGENT_VERSION "agent.version"
#define K_BATTERY "mote.battery"
#define K_TEMPERATURE "mote.temperature"

#define CD_K_PING 1
#define CD_K_AGENT_NAME 2
#define CD_K_AGENT_VERSION 3
#define CD_K_BATTERY 4
#define CD_K_TEMPERATURE 5

#define ZABBIX_PORT 10050
#define INPUTBUFSIZE 100
#define OUTPUTBUFSIZE 100

static struct psock ps;
static uint8_t inputptr[INPUTBUFSIZE];
static int cd_key;

void double_to_char(double f,char * buffer) {
  gcvt(f,10,buffer);
}
/*---------------------------------------------------------------------------*/
static uint8_t cmp(char *str1, char *key) {
 printf("[cmp] str1: '%s' key: '%s'\n", str1, key);
 return strncmp(str1, key, strlen(key));
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(handle_connection(struct psock *p))
{
  PSOCK_BEGIN(p);

  PSOCK_READTO(p, '\n');

  int inputdatalen = sizeof(inputptr) / sizeof(inputptr[0]); 
  printf("buffer %d bytes '%s'\r\n", inputdatalen, inputptr);
  printf("buffer [");
  int i;
  for (i = 0; i < inputdatalen; ++i) {
    if (i > 0) 
      printf(", ");  
    printf("'%d'", (uint8_t)inputptr[i]);
  }
  printf("]\n");

  if(strncmp((char *) inputptr, H_PREFIX, 5) == 0) {
    printf("ZBXD\\x01 received\n");

    int cmdSize = (uint8_t)inputptr[5];
    printf("Tamanho da chave: %d\n", cmdSize);
    char* cmd = (char*)malloc(cmdSize + 1);
    memcpy(cmd, &inputptr[13], cmdSize);
    cmd[cmdSize] = '\0';
    printf("cmd: %s\n", cmd);

    if(cmp(cmd, K_PING) == 0) {
      cd_key = CD_K_PING;
      printf("Comando ping\r\n");
    } else if (cmp(cmd, K_AGENT_NAME) == 0) {
      cd_key = CD_K_AGENT_NAME;
      printf("Pedindo nome do agente\r\n");
    } else if (cmp(cmd, K_AGENT_VERSION) == 0) {
      cd_key = CD_K_AGENT_VERSION;
      printf("Pedindo versao do agente\r\n");
    } else if (cmp(cmd, K_BATTERY) == 0) {
      cd_key = CD_K_BATTERY;
      printf("Pedindo bateria\r\n");
    } else if (cmp(cmd, K_TEMPERATURE) == 0) {
      cd_key = CD_K_TEMPERATURE;
      printf("Pedindo temperatura\r\n");
    } else {
      cd_key = 0;
    }

    if (cd_key > 0) {
      printf("Responding chave %d\n", cd_key);
      
      PSOCK_SEND_STR(p, H_PREFIX);

      char *zbx_data;
      if (cd_key == CD_K_PING) {
        uint8_t datalen[] = { 1, 0, 0, 0, 0, 0, 0, 0 };
        PSOCK_SEND(p, datalen, 8);
        PSOCK_SEND_STR(p, "1");
      } else if (cd_key == CD_K_AGENT_NAME) {
        uint8_t datalen[] = { 14, 0, 0, 0, 0, 0, 0, 0 };
        PSOCK_SEND(p, datalen, 8);
        PSOCK_SEND_STR(p, "mote.zbx.agent");
      } else if (cd_key == CD_K_AGENT_VERSION) {
        uint8_t datalen[] = { 5, 0, 0, 0, 0, 0, 0, 0 };
        PSOCK_SEND(p, datalen, 8);
        PSOCK_SEND_STR(p, "1.0.0");
      } else if (cd_key == CD_K_BATTERY) {
        uint8_t datalen[] = { 3, 0, 0, 0, 0, 0, 0, 0 };
        PSOCK_SEND(p, datalen, 8);

        uint16_t bat_int = battery_sensor.value(0);
        char output[3];
        snprintf(output, 3, "%d", bat_int);
        printf("output: %s\n", output);
        printf("Battery: %d | %i (%f mV)\n", bat_int, bat_int, bat_int);
        PSOCK_SEND_STR(p, output);
      }
      zbx_data = "";
      printf("Result1: %s\n", zbx_data);
      uint8_t zbx_data_len = strlen(zbx_data);
      
      if (zbx_data_len > 0) {
        // Enviando o <datalen>
        uint8_t datalen[] = { zbx_data_len, 0, 0, 0, 0, 0, 0, 0 };
        PSOCK_SEND(p, datalen, 8);
        // Enviando o resultado
        printf("<data>: %s\n", zbx_data);
        PSOCK_SEND_STR(p, zbx_data);
      }
    }
  }
  
  printf("Fechando conexao...\n");
  PSOCK_CLOSE(p);
  PSOCK_END(p);
}

/*---------------------------------------------------------------------------*/
PROCESS(zabbix_proto_agent_process, "Zabbix Agent process");
AUTOSTART_PROCESSES(&zabbix_proto_agent_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(zabbix_proto_agent_process, ev, data)
{
  printf("Iniciando...\n");
  PROCESS_BEGIN();
  SENSORS_ACTIVATE(battery_sensor);

  printf("Escutando TCP...\n");
  tcp_listen(UIP_HTONS(ZABBIX_PORT));

  while(1) {
    printf("Esperando conexao TCP...\n");

    PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);

    if(uip_connected()) {
      printf("Conexao TCP estabelecida\n");
      PSOCK_INIT(&ps, inputptr, sizeof(inputptr));
      while(!(uip_aborted() || uip_closed() || uip_timedout())) {
        PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
        handle_connection(&ps);
      }
    }
  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
