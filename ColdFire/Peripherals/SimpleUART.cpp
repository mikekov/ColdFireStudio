/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// Simple UART implementation copied from ColdFire simulator 0.3.1, with changes:

/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "pch.h"
#include "SimpleUART.h"
#include "../Context.h"
#include "../PeripheralRepository.h"

// register UART
static auto reg_uart= GetPeripherals().Register("uart", "5206", &CreateDevice<SimpleUART>);


enum class Offset	// offsets from MBAR
{
	ModeReg= 0x00,	// read/write
	// read
	StatusReg= 0x04,
	ReceiverBuf= 0x0c,
	InputPortChg= 0x10,
	InterruptStatus= 0x14,
	// r/w
	PrescaleMSB= 0x18,
	PrescaleLSB= 0x1c,
	// r/w
	InterruptVector= 0x30,
	InputPort= 0x34,
	// write
	ClockSelect= 0x04,
	CommandReg= 0x08,
	TransmitterBuf= 0x0c,
	AuxCtrlReg= 0x10,
	InterruptMask= 0x14,
	OutputPortBitSetCmd= 0x38,
	OutputPortBitResetCmd= 0x3c,
};


/* UMR1 -- Mode Register 1
 *    7     6    5   4   3   2   1   0
 * +-----+-----+---+-------+---+-------+
 * |RxRTS|RxIRQ|ERR| PM1-0 |PT |B/C1-0 |
 * +-----+-----+---+-------+---+-------+
 */

struct _UMR1 {
	uint8 RxRTS;
	uint8 RxIRQ;
	uint8 ER;
	uint8 PM;
	uint8 PT;
	uint8 BC;
};

/* UMR2 -- Mode Register 2
 *   7   6    5     4    3   2   1   0
 * +-------+-----+-----+---------------+
 * | CM1-0 |TxRTS|TxCTS|    SB3-0      |
 * +-------+-----+-----+---------------+
 */
struct _UMR2 {
	uint8 CM;
	uint8 TxRTS;
	uint8 TxCTS;
	uint8 SB;
};


/* USR -- Status Register
 *    7     6     5     4     3     2     1     0
 * +-----+-----+-----+-----+-----+-----+-----+-----+
 * | RB  | FE  | PE  | OE  |TxEMP|TxRDY|FFULL|RxRDY|
 * +-----+-----+-----+-----+-----+-----+-----+-----+
 */
struct _USR {
	uint8 RB;
	uint8 FE;
	uint8 PE;
	uint8 OE;
	uint8 TxEMP;
	uint8 TxRDY;
	uint8 FFULL;
	uint8 RxRDY;
};


/* UCSR -- Clock Select Register
 *    7     6     5     4     3     2     1     0
 * +-----------------------+-----------------------+
 * |        RCS3-0         |        TCS3-0         |
 * +-----------------------+-----------------------+
 */
struct _UCSR {
	uint8 RCS;
	uint8 TCS;
};


/* UCR -- Command Register
 *    7     6     5     4     3     2     1     0
 * +-----+-----------------+-----------+-----------+
 * | --- |     MISC2-0     |   TC1-0   |   RC1-0   |
 * +-----+-----------------+-----------+-----------+
 */
struct _UCR {
	uint8 MISC;
	uint8 TC;
	uint8 RC;
};

/* UIPCR - Input Port Change Register 
 *    7     6     5     4     3     2     1     0
 * +-----+-----+-----+-----+-----+-----+-----+-----+
 * |  0  |  0  |  0  | COS |  1  |  1  |  1  | CTS |
 * +-----+-----+-----+-----+-----+-----+-----+-----+
 */
struct _UIPCR {
	uint8 COS;
	uint8 CTS;
};


/* UISR - Interrupt Status Register
 *    7     6     5     4     3     2     1     0
 * +-----+-----+-----+-----+-----+-----+-----+-----+
 * | COS |  -  |  -  |  -  |  -  |  DB |RxRDY|TxRDY|
 * +-----+-----+-----+-----+-----+-----+-----+-----+
 */
struct _UISR {
	uint8 COS;
	uint8 DB;
	uint8 RxRDY;
	uint8 TxRDY;
};


/* UIMR - Interrupt Mask Register
 *    7     6     5     4     3     2     1     0
 * +-----+-----+-----+-----+-----+-----+-----+-----+
 * | COS |  -  |  -  |  -  |  -  |  DB |FFULL|TxRDY|
 * +-----+-----+-----+-----+-----+-----+-----+-----+
 */
struct _UIMR {
	uint8 COS;
	uint8 DB;
	uint8 FFULL;
	uint8 TxRDY;
};


struct SimpleUART::_uart_data
{
	_uart_data()
	{
		memset(this, 0, sizeof(*this));
	}

	/* Read */
	struct _UMR1 UMR1;
	struct _UMR2 UMR2;
	uint8 UMR_pointer;
	struct _USR USR;
	char URB[3];	/* Recieve Buffer */
	uint8 URB_count;
	struct _UIPCR UIPCR;
	struct _UISR UISR;
	uint8 UBG1;
	uint8 UBG2;
	uint8 UIVR;
//	uint8 UIP; /* Don't need this, can figure it out from the fd state */
	
	/* Write */
	struct _UCSR UCSR;
	struct _UCR UCR; /* We dont' need to save this, but thisi is a good
			  * place to put the register for decoding */
	char UTB;	/* Transmit Buffer */
	char UACR;
	struct _UIMR UIMR;

	/* Connections and threads */
	char transmitter_enabled;
	char receiver_enabled;
	int fd;

//	pthread_t tid;
//	pthread_mutex_t lock;
	int port;

	int read_delay_;
};


SimpleUART::SimpleUART(PParam params) : Peripheral(params.IOAreaSize(0x40))
{
	uart = new _uart_data();
}


SimpleUART::~SimpleUART()
{
	delete uart;
}


// called during simulator run after executing single opcode
void SimpleUART::Update(Context& ctx)
{
	if (!uart->transmitter_enabled && !uart->receiver_enabled)
		return;

	/* This routine handles transmitting, receiving, and is
	 * in charge of asserting, or withdrawing the serial interrupt */
//	struct _uart_data *uart = (struct _uart_data *)s->data;
//	int status;
	
/*	s16 Baud = (uart->UBG1 << 8) | uart->UBG2;*/

/*	TRACE("%s: update: connected=%d, transmitterenabled=%d, receiverenabled=%d\n",
		PortNumber, uart->fd_connected, uart->transmitter_enabled, uart->receiver_enabled);
*/
	//if (!uart->fd) {
	//	/* If there is no connection, there's no chance we can
	//	 * do anything with this port */
	//	return;
	//}
	
	/* Transmit stuff */
	if (uart->transmitter_enabled)
	{
		/* If TransmitterReady (TxRDY bit of Status Reg USR)
		 *  0 - Character waiting in UTB to send
		 *  1 - Transmitter UTB is empty, and ready to be loaded */
		if (uart->USR.TxRDY == 0)
		{
			/* We use the fact that send() buffers characters
			 * for us, so we don't need to shift anything
			 * into transmit buffers */ 
			//TRACE("%s: Sending character %c(%d)\n", s->name, uart->UTB,uart->UTB);
			// output character into the simulator terminal
			ctx.SimWrite(cf::SimPort::IN_OUT, uart->UTB);
			//if (uart->fd) send(uart->fd, &uart->UTB, 1, 0);
			/* Signal that there is room in the buffer */
			uart->USR.TxRDY = 1;
			/* The UISR duplicates the USR for TxRDY */
			uart->UISR.TxRDY = 1;

			/* Now that the character is sent, set the buffer
			 * empty condition */
			uart->USR.TxEMP = 1;
		}
	}

	if (uart->receiver_enabled)
	{
		// read characters from simulator terminal

		//TODO:
//		uart->UIPCR.CTS = 0;
//		uart->UIPCR.COS = 1;

		// slow down reading, probing is expensive
		auto c= 0;
		if (uart->read_delay_ > 0)
			uart->read_delay_--;
		else
		{
			c = ctx.SimRead(cf::SimPort::IN_OUT);
			uart->read_delay_ = 10;
		}

		if (c != 0)
		{
			uart->UIPCR.CTS = 0;
			uart->UIPCR.COS = 1;
/*
			/ * If status == 0, that's EOF * /
			
			if (status <= 0) break;
//			if (status == -1 || c==0) goto interrupt_update;
			if (c==0) continue; //goto interrupt_update;
			if (c==0xff) {
				/ * Escape sequence * /
				status = recv(uart->fd, &c, 1, 0);
				status = recv(uart->fd, &c, 1, 0);
				continue;
//				goto interrupt_update;
			}

			TRACE("%s: Got character %d[%c]\n", s->name, c, c);
*/

			/* If URB has room, store the character */
			if (uart->URB_count < 3)
			{
				uart->URB[(int)uart->URB_count] = static_cast<char>(c);
				uart->URB_count++;
			}
			else
			{
//				ERR("%s: URB_count just overflowed!\n", s->name);
			}

			/* There's something in the buffer now */
			uart->USR.RxRDY = 1;
			if (uart->URB_count == 3)
				uart->USR.FFULL=1;

			if (uart->UMR1.RxIRQ == 0)
				uart->UISR.RxRDY = uart->USR.RxRDY;
			else
				uart->UISR.RxRDY = uart->USR.FFULL;
		}
	}
	
//interrupt_update:
	auto interrupts_on= false;

	/* Adjust the interrupt status */
	/* If the TX is masked on, and there is room in the buffer, we 
		want the interrupt on */
	if (uart->UIMR.TxRDY && uart->USR.TxRDY)
	{
		//TRACE("%s: Transmit Interrupt Condition exists\n", s->name);
		interrupts_on = true;
	}
	/* If Rx is masked on, and we are interrupting on RxRDY, and RxRDY is on, then interrupt */
	else if (uart->UIMR.FFULL && !uart->UMR1.RxIRQ && uart->USR.RxRDY)
	{
		//TRACE("%s: Recieve RxRDY interrupt Condition exists\n", s->name);
		interrupts_on = true;
	}
	/* If Rx is masked on, and we are interrupting on FFULL, and FFULL is on, then interrupt */
	else if (uart->UIMR.FFULL && uart->UMR1.RxIRQ && uart->USR.FFULL)
	{
		//TRACE("%s: Recieve FFULL interrupt Condition exists\n", s->name);
		interrupts_on = true;
	}

	if (!interrupts_on)
	{
	/* If we get here, we couln't find a condition to turn/leave
		the interrupts on, so turn 'em off */
/*	TRACE("%s: No interrupt conditions, withdrawing any interrupt requests\n", s->name);*/
		ctx.InterruptClear(InterruptSource());
	}
	else
	{
	//TRACE("%s: Posting interrupt request for %s\n", s->name);
		ctx.InterruptAssert(InterruptSource(), static_cast<CpuExceptions>(uart->UIVR));
	}
}


// resetting device
void SimpleUART::Reset()
{
	memset(&uart->UMR1, 0, sizeof(uart->UMR1));
	memset(&uart->UCR, 0, sizeof(uart->UCR));
	uart->URB[0] = 0;
	uart->URB_count = 0;
	uart->UIVR = EX_UninitializedInterrupt;
}


// read from device; access_size is 1, 2, or 4
uint32 SimpleUART::Read(uint32 offset, int access_size)
{
	if (access_size != 1)
	{
		ASSERT(false);
		return 0;
	}

	//switch (addr)
	//{
	//case Offset::ModeReg:
	//	break;

	//case Offset::StatusReg:
	//	break;
	//}

	uint32 result= 0;
	
	switch (offset)
	{
	case 0x0000: /* Mode Register (UMR1, UMR2) */
		break;
	case 0x0004: /* Status Register (USR) */
		result = 
			(uart->USR.RB   ? 0x80 : 0x00) |
			(uart->USR.FE   ? 0x40 : 0x00) |
			(uart->USR.PE   ? 0x20 : 0x00) |
			(uart->USR.OE   ? 0x10 : 0x00) |
			(uart->USR.TxEMP? 0x08 : 0x00) |
			(uart->USR.TxRDY? 0x04 : 0x00) |
			(uart->USR.FFULL? 0x02 : 0x00) |
			(uart->USR.RxRDY? 0x01 : 0x00);
		break;

	case 0x0008: /* DO NOT ACCESS */
		return 0;

	case 0x000C: /* Receiver Buffer (URB) */
		result = uart->URB[0];
		/* Shift the FIFO */
		uart->URB[0] = uart->URB[1];
		uart->URB[1] = uart->URB[2];

		/* See if we can decrement the fifo count */
		if (uart->URB_count == 0) {
			//ERR("%s: underrun (read but no data in FIFO)\n", s->name);
			uart->URB_count=0;
		} else {
			uart->URB_count--;
		}
			

		/* Check to see if there is nothing left in the buffer */
		if (uart->URB_count==0)
			uart->USR.RxRDY = 0;

		/* The buffer cannot be full anymore, we just did a read */
		uart->USR.FFULL=0;

		/* Set the UISR */
		if (uart->UMR1.RxIRQ==0)
			uart->UISR.RxRDY = uart->USR.RxRDY;
		else
			uart->UISR.RxRDY = uart->USR.FFULL;

		/* Update the status of the serial intrrupt */
/*		Serial_InterruptUpdate(PortNumber);*/
		break;

	case 0x0010: /* Input Port Change Register (UIPCR) */
		result = (uart->UIPCR.COS ? 0x40 : 0x00) |
			  (uart->UIPCR.CTS ? 0x01 : 0x00);
		/* Reading from this register should clear the 
		 * COS (change of state) if it was asserted */
		uart->UIPCR.COS = 0;
		break;
	case 0x0014: /* Interrupt Status Register (UISR) */
		result = (uart->UISR.COS  ? 0x80 : 0x00) |
			  (uart->UISR.DB   ? 0x04 : 0x00) |
			  (uart->UISR.RxRDY? 0x02 : 0x00) |
			  (uart->UISR.TxRDY? 0x01 : 0x00);
		break;
	case 0x018: /* Baud Rate Generator Prescale MSB (UBG1) */
		result = uart->UBG1;
		break;
	case 0x01C: /* Baud Rate Generator Prescale LSB (UBG2) */
		result = uart->UBG2;
		break;
	case 0x0030: /* Interrupt Vector Register (UIVR) */
		result = uart->UIVR;
		break;
	case 0x0034: /* Input Port Register (UIP) */
		/* Set to 0 (meaning nCTS=0, meaning we're Clear to Send) 
		 * if there is something connected */
		result = (uart->fd != 0) ? 0 : 1;
		break;
	case 0x0038: /* NO NOT ACCESS */
	case 0x003C: /* NO NOT ACCESS */
	default: 
		//ERR("%s: Unaligned access!\n",s->name);
		break;;
	}

//	TRACE("%s: result=0x%08lx\n", s->name, *result);

	return result;
}


// write to device; access_size is 1, 2, or 4
void SimpleUART::Write(Context& ctx, uint32 offset, int access_size, uint32 value)
{
	if (access_size != 1)
	{
		ASSERT(false);
		return;
	}

	//switch (addr)
	//{
	//case Offset::ModeReg:
	//	break;

	//case Offset::CommandReg:
	//	break;
	//}

	/* We ASSUME that we are passed an offset that is valid for us, 
		and that MBAR has already been subtracted from it */

//	TRACE("%s: size=%d, offset=0x%04lx, value=0x%08lx\n", s->name, size, offset, value);

	switch (offset)
	{
	case 0x0000: /* Mode Register (UMR1, UMR2) */
		if (uart->UMR_pointer==0) {
			uart->UMR1.RxRTS = (value & 0x80) ? 1 : 0;
			uart->UMR1.RxIRQ = (value & 0x40) ? 1 : 0;
			uart->UMR1.ER   = (value & 0x20) ? 1 : 0;
			uart->UMR1.PM = (value & 0x18) >> 3;
			uart->UMR1.PT = (value & 0x04) ? 1 : 0;
			uart->UMR1.BC = (value & 0x03);
			
			uart->UMR_pointer=1;
//			TRACE("%s: Setting Mode Register 1 (UMR1)\n", s->name);
//			TRACE("%s:    RxRTS=%d, RxIRQ=%d, ErrorMode(ERR)=%d, ParityMode(PM)=%d\n", s->name, uart->UMR1.RxRTS, uart->UMR1.RxIRQ, uart->UMR1.ER, uart->UMR1.PM);
//			TRACE("%s:    ParityType(PT)=%d, BitsPerCharacter(BC)=%d\n", s->name, uart->UMR1.PT, uart->UMR1.BC);
		} else {
			uart->UMR2.CM = (value & 0xC0) >> 6;
			uart->UMR2.TxRTS = (value & 0x02) ? 1 : 0;
			uart->UMR2.TxCTS = (value & 0x01) ? 1 : 0;
			uart->UMR2.SB = (value & 0x0F);
//			TRACE("%s: Setting Mode Register 2 (UMR2)\n", s->name);
//			TRACE("%s:    CM=%d, TxRTS=%d, TxCTS=%d, SB=%d\n", s->name, 
//					uart->UMR2.CM, uart->UMR2.TxRTS, 
//					uart->UMR2.TxCTS, uart->UMR2.SB);
		}
		break;
	case 0x0004: /* Clock-Select Register (UCSR) */
		uart->UCSR.RCS = (value & 0xF0) >> 4;
		uart->UCSR.TCS = (value & 0x0F);
		//TRACE("%s: Clock-Select Register (UCSR)\n", s->name);
		//TRACE("%s:    RCS=%d, TCS=%d\n", s->name, 
		//		uart->UCSR.RCS, uart->UCSR.TCS); 
		break;
		
	case 0x0008: /* Command Register (UCR) */
		uart->UCR.MISC = (value & 0x70) >> 4;
		uart->UCR.TC = (value & 0x0C) >> 2;
		uart->UCR.RC = (value & 0x03);
		switch(uart->UCR.MISC) {
		case 0: /* No Command */
			break;
		case 1: /* Reset Mode Register Pointer */
			//TRACE("   Resetting Mode Register Pointer\n");
			uart->UMR_pointer=0;
			break;
		case 2: /* Reset Receiver */
			//TRACE("   Resetting Receiver\n");
			uart->receiver_enabled=0;
			uart->USR.FFULL=0;
			uart->USR.RxRDY=0;
			break;
		case 3: /* Reset Transmitter */
			//TRACE("   Resetting Transmitter\n");
			uart->transmitter_enabled=0;
			uart->USR.TxEMP=0;
			uart->USR.TxRDY=0;
			uart->UISR.TxRDY=0;
/*				Serial_InterruptUpdate(PortNumber);*/
			break;
		case 4: /* Reset Error Status */
			//TRACE("   Resetting Error Status\n");
			uart->USR.OE=0;
			uart->USR.FE=0;
			uart->USR.PE=0;
			uart->USR.RB=0;
			break;
		case 5: /* Reset Break-Change Interrupt */
			//ERR("%s: Resetting Break-Change Interrupt (NOT IMPLEMENTED)\n", s->name);
			break;
		case 6: /* Start Break */
			//ERR("%s: Setting Start Break (NOT IMPLEMENTED)\n", s->name);
			break;
		case 7: /* Stop Break */
			//ERR("%s: Setting Stop Break (NOT IMPLEMENTED)\n", s->name);
			break;
		}

		switch(uart->UCR.TC) {
		case 0:	/* No action */
			break;
		case 1: /* Transmitter Enable */
			//TRACE("   Enabling Transmitter\n");
			if (!uart->transmitter_enabled) {
				uart->transmitter_enabled=1;
				uart->USR.TxEMP=1;
				uart->USR.TxRDY=1;
				uart->UISR.TxRDY=1;
/*					Serial_InterruptUpdate(PortNumber);*/
			}
			break;
		case 2: /* Transmitter Disable */
			//TRACE("   Disabling Transmitter\n");
			if (uart->transmitter_enabled) {
				uart->transmitter_enabled=0;
				uart->USR.TxEMP=0;
				uart->USR.TxRDY=0;
				uart->UISR.TxRDY=0;
/*					Serial_InterruptUpdate(PortNumber);*/
			}
			break;
		case 3: /* Do Not Use */
			//ERR("%s: Accessed DO NOT USE bit for UCR:TC bits\n", s->name);
			break;
		}

		/* Receiver stuff */
		switch(uart->UCR.RC) {
		case 0:	/* No action */
			break;
		case 1: /* Receiver Enable */
			//TRACE("   Enabling Receiver\n");
			if (!uart->receiver_enabled)
				uart->receiver_enabled=1;
			break;
		case 2: /* Receiver Disable */
			//TRACE("   Disabling Receiver\n");
			if (uart->receiver_enabled)
				uart->receiver_enabled=0;
			break;
		case 3: /* Do Not Use */
			//ERR("%s: Accessed DO NOT USE bit for UCR:RC bits\n", s->name);
			break;
		}
		break;
		
	case 0x000C: /* Transmitter Buffer (UTB) */
		//TRACE("   Transmitting character 0x%02x\n", value);
		uart->UTB = (char)value;

		/* A write to the UTB Clears the TxRDY bit */
		uart->USR.TxRDY=0;
		uart->UISR.TxRDY=0;

		/* Also ensure the empty bit is off, we just wrote
		 * to the transmitter, the buffer is not empty */
		uart->USR.TxEMP=0;

		break;

	case 0x0010: /* Auxiliary Control Register (UACR) */
		/* We can't interrupt on a change in the CTS line of 
		  * the serial port.. because.. well.. we're not connected
		  * to a real serial port.. so we ignore writes to
		  * this register, but print an error if someone
		  * tries to enable this */
		if ( ((char)value) & 0x01) {
			/* Enable CTS interrupt */
			//ERR("%s: Setting Auxiliary Control Register (UACR) "
			//	"IEC (Interrupt Enable Control) has no effect, "
			//	"because we don't have a CTS pin on the serial "
			//	"port to interrupt on.  Sorry.\n", s->name);
		}
		break;
	case 0x0014: /* Intrerrupt Mask Register (UIMR) */
		//TRACE("   Setting Interrupt Mask Register (UMIR) to 0x%02x\n", value);
		uart->UIMR.COS = (value &0x80) ? 1 : 0;
 		uart->UIMR.DB =  (value &0x04) ? 1 : 0;
 		uart->UIMR.FFULL = (value &0x02) ? 1 : 0;
 		uart->UIMR.TxRDY = (value &0x01) ? 1 : 0;
		//TRACE("   Change-of-State (COS) interrupt: %s\n", uart->UIMR.COS ? "enabled":"disabled");
		//TRACE("   Delta Break (DB) interrupt: %s\n", uart->UIMR.DB ? "enabled":"disabled");
		//TRACE("   FIFO Full (FFULL) interrupt: %s\n", uart->UIMR.FFULL ? "enabled":"disabled");
		//TRACE("   Transmitter Ready (TxRDY) interrupt: %s\n", uart->UIMR.TxRDY ? "enabled":"disabled");
/*		Serial_InterruptUpdate(PortNumber);*/
		break;
	case 0x0018: /* Baud Rate Generator Prescale MSB (UBG1) */
		//TRACE("   Baud Rate Generator Prescale MSB to 0x%02x\n", value);
		uart->UBG1 = (unsigned char)value;
		//TRACE("   Baud Rate is %d\n", (uart->UBG1 << 8) + uart->UBG2);
		break;
	case 0x001C: /* Baud Rate Generator Prescale LSB (UBG2) */
		//TRACE("   Baud Rage Generator Prescale LSB to 0x%02x\n", value);
		uart->UBG2 = (unsigned char)value;
		//TRACE("   Baud Rate is %d\n", (uart->UBG1 << 8) + uart->UBG2);
		break;
	case 0x0030: /* Interrupt Vector Register (UIVR) */
		//TRACE("   Setting Interrupt Vector Register to 0x%02x\n", value);
		uart->UIVR = (unsigned char)value;
		break;
	case 0x0034: /* NO NOT ACCESS */
		break;
	case 0x0038: /* Output Port Bit Set CMD (UOP1) */
		/* Since we don't have a direct connection to a sieral
		 * port, this does nothing */
		//TRACE("%s: Set Output Port Bit (UOP1) (no effect)\n", s->name);
		break;
	case 0x003C: /* Output Port Bit Reset CMD (UOP0) */
		//TRACE("%s: Reset Output Port Bit (UOP0) (no effect)\n", s->name);
		break;
	default:
		//return 0;
		break;
	}
	//return 1;
}
