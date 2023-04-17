/*
 * udp_server.c
 *
 *  Created on: Apr 12, 2023
 *      Author: Nikita Kliatskyi
 */

#include "sockets.h"
#include "cmsis_os.h"
#include <string.h>
#include <strings.h>

#define RX_BUF_LEN 20
#define TX_BUF_LEN 50

static struct sockaddr_in serv_addr;
static int socket_fd;
static uint16_t nport;

static int udpServerInit(void)
{
	socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_fd == -1)
	{
		return -1;
	}

	nport = 5678UL;
	nport = htons((uint16_t)nport);

	bzero(&serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = nport;

	if(bind(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))==-1)
	{
		close(socket_fd);
		return -1;
	}

	return 0;
}

void StartUdpServerTask(void const * argument)
{
	struct sockaddr_storage peer_addr;
	socklen_t peer_addr_len;

	char rx_buf[RX_BUF_LEN] = { 0 };
	char tx_buf[TX_BUF_LEN] = { 0 };

	osDelay(5000);

	if(udpServerInit() < 0) {
		return;
	}

	for(;;)
	{
		peer_addr_len = sizeof(peer_addr);

		bzero(rx_buf, RX_BUF_LEN);
		bzero(tx_buf, TX_BUF_LEN);

		if(recvfrom(socket_fd, rx_buf, RX_BUF_LEN, 0, (struct sockaddr *)&peer_addr, &peer_addr_len) == -1)
		{
			continue;
		}

		if(rx_buf[strlen(rx_buf) - 1] == '\n')
		{
			rx_buf[strlen(rx_buf) - 1] = '\0';
		}

		char *tok;
		tok = strtok(rx_buf, " ");

		if((strcasecmp(tok, "sversion") == 0) &&
				(strtok(NULL, " ") == NULL))
		{
			sendto(socket_fd, "udp_srv_nikita_kliatskyi_13042023\r\nOK\r\n", sizeof("udp_srv_nikita_kliatskyi_13042023\r\nOK\r\n"), 0, (struct sockaddr *)&peer_addr, peer_addr_len);
		}
		else if((strlen(tok) == 4) &&
				(strncasecmp(tok, "led", 3) == 0) &&
				'3' <= tok[3] && tok[3] <= '6')
		{
			int led_id = tok[3] - '0';
			uint16_t pins[] = { GPIO_PIN_13, GPIO_PIN_12, GPIO_PIN_14, GPIO_PIN_15 };
			uint16_t led_pin = pins[led_id - 3];

			tok = strtok(NULL, " ");

			if(strcasecmp(tok, "on") == 0)
			{
				HAL_GPIO_WritePin(GPIOD, led_pin, GPIO_PIN_SET);
				sendto(socket_fd, "OK\r\n", sizeof("OK\r\n"), 0, (struct sockaddr *)&peer_addr, peer_addr_len);
			}
			else if(strcasecmp(tok, "off") == 0)
			{
				HAL_GPIO_WritePin(GPIOD, led_pin, GPIO_PIN_RESET);
				sendto(socket_fd, "OK\r\n", sizeof("OK\r\n"), 0, (struct sockaddr *)&peer_addr, peer_addr_len);
			}
			else if(strcasecmp(tok, "toggle") == 0)
			{
				HAL_GPIO_TogglePin(GPIOD, led_pin);
				sendto(socket_fd, "OK\r\n", sizeof("OK\r\n"), 0, (struct sockaddr *)&peer_addr, peer_addr_len);
			}
			else if(strcasecmp(tok, "status") == 0)
			{
				GPIO_PinState state = HAL_GPIO_ReadPin(GPIOD, led_pin);
				sprintf(tx_buf, "LED%d %s\r\n", led_id, state == GPIO_PIN_SET ? "ON" : "OFF");
				sendto(socket_fd, tx_buf, sizeof(tx_buf), 0, (struct sockaddr *)&peer_addr, peer_addr_len);
				sendto(socket_fd, "OK\r\n", sizeof("OK\r\n"), 0, (struct sockaddr *)&peer_addr, peer_addr_len);
			}
			else
			{
				sendto(socket_fd, "ERROR\r\n", sizeof("ERROR\r\n"), 0, (struct sockaddr *)&peer_addr, peer_addr_len);
			}
		}
		else
		{
			sendto(socket_fd, "ERROR\r\n", sizeof("ERROR\r\n"), 0, (struct sockaddr *)&peer_addr, peer_addr_len);
		}
	}
}
