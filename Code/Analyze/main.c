#include <msp430.h> 
#include <time.h>
#define __UDPPERFORMANCETEST_C
#include "ipstack/TCPIPConfig.h"
#include "ipstack/TCPIP.h"
#include "hardware_board.h"
#include "config.h"
#include "ipstack/MAC.h"
#include "ipstack/UDP.h"
#include "ipstack/StackTsk.h"
#include "Tick.h"
#include "appconfig.h"
void UDPPerformanceTaskSending(void);
// Which UDP port to broadcast from for the UDP tests
#define PERFORMANCE_PORT	9
/*
 * This program tests the UDP performance of MSP430F5437A and ENC28J60
 */
int main(void) {
	int i;
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

	/*** PERIPHERALS INITIALIZATION ***/
	InitializeClocks(); // Initialize Unified Clock System
	InitializeTimers(); // Initialize Timers

	/*** BOARD INITIALIZATION ***/
	// InitializeButton(); // Initialize Button
	InitializeLeds(); // Initialize LEDs
	InitializeIO();

	/**** ETHERNET INITIALIZATION ****/
	InitAppConfig();
	InitializeEthSpi(); // Configure SPI module for ENC28J60
	StackInit(); // Initialize Stack

	//__bis_SR_register(LPM3_bits + GIE);

	LED_OUT &= ~(LED1+LED2);
	LED_OUT |= LED1;
	
	//Wait until the stack is properly configured to enter the main program
	while (StackIsInConfigMode()) StackTask();

	LED_OUT &= ~LED1;
	LED_OUT |= LED2;

    // Initialize UDP
	UDPInit();

	// MCU to PC testing
	for (i = 0; i < 1024; i++) UDPPerformanceTaskSending();

	// ENC28J60 Buffer to PC testing
	for (i = 0; i < 1024; i++) MACFlush();

	return 0;
}

/*****************************************************************************
  Function:
	void UDPPerformanceTask(void)

  Summary:
	Tests the transmit performance of the UDP module.

  Description:
	This function tests the transmit performance of the UDP module.  At boot,
	this module will transmit 1024 large UDP broadcast packets of 1024 bytes
	each.  Using a packet sniffer, one can determine how long this process
	takes and calculate the transmit rate of the stack.  This function tests
	true UDP performance in that it will open a socket, transmit one packet,
	and close the socket for each loop.  After this initial transmission, the
	module can be re-enabled by holding button 3.

	This function is particularly useful after development to determine the
	impact of your application code on the stack's performance.  A before and
	after comparison will indicate if your application is unacceptably
	blocking the processor or taking too long to execute.

  Precondition:
	UDP is initialized.

  Parameters:
	None

  Returns:
	None
  ***************************************************************************/
void UDPPerformanceTaskSending(void)
{
	UDP_SOCKET	MySocket;
	NODE_INFO	Remote;
	WORD		wTemp;
	BYTE* 		data = "1024b";
	static DWORD dwCounter = 1;


	// Set the socket's destination to be a broadcast over our IP
	// subnet
	// Set the MAC destination to be a broadcast
	memset(&Remote, 0xFF, sizeof(Remote));

	// Open a UDP socket for outbound transmission
	MySocket = UDPOpenEx((DWORD)(PTR_BASE)&Remote,UDP_OPEN_NODE_INFO,0,PERFORMANCE_PORT);

	// Abort operation if no UDP sockets are available
	// If this ever happens, incrementing MAX_UDP_SOCKETS in
	// StackTsk.h may help (at the expense of more global memory
	// resources).
	if(MySocket == INVALID_UDP_SOCKET) return;

	// Make certain the socket can be written to
	if(!UDPIsPutReady(MySocket))
	{
		UDPClose(MySocket);
		return;
	}

	// Put counter value into first 4 bytes of the packet
	UDPPutArray((BYTE*)&dwCounter, sizeof(dwCounter));
	dwCounter++;

	// Change the last parameter to desired UDP payload size
	// Formula: Desired payload size - 4
	// Data is fetched from the RAM
	wTemp = UDPPutArray(data, 1020);

	// Send UDP packet
	UDPFlush();

	// Close the socket so it can be used by other modules
	UDPClose(MySocket);
}
