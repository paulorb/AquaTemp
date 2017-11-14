/*
 * AquaSlaveV1.c
 *
 * Created: 18/12/2015 17:18:59
 *  Author: PauloRB
 */ 


#include <stdlib.h>
#include <avr/io.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/delay.h>
#include "global.h"		// include our global settings
#include "i2c.h"		// include i2c support
#include "onewire.h"
#include "ds18x20.h"

void i2cSlaveReceiveService(u08 receiveDataLength, u08* receiveData);
u08 i2cSlaveTransmitService(u08 transmitDataLengthMax, u08* transmitData);


//###################### Protocol Macros
#define ASPROTOCOL_CMD_PING 0x1
#define ASPROTOCOL_CMD_TEMP_READ 0x2
#define ASPROTOCOL_CMD_FAN_READ_RPM 0x03
#define ASPROTOCOL_CMD_FAN_CONTROL 0x6
#define ASPROTOCOL_CMD_PELTIER_CONTROL 0x7

//###################### Macros
#define uniq(LOW,HEIGHT)	((HEIGHT << 8)|LOW)			// 2x 8Bit 	--> 16Bit
#define LOW_BYTE(x)        	(x & 0xff)					// 16Bit 	--> 8Bit
#define HIGH_BYTE(x)       	((x >> 8) & 0xff)			// 16Bit 	--> 8Bit


#define sbi(ADDRESS,BIT) 	((ADDRESS) |= (1<<(BIT)))	// set Bit
#define cbi(ADDRESS,BIT) 	((ADDRESS) &= ~(1<<(BIT)))	// clear Bit
#define	toggle(ADDRESS,BIT)	((ADDRESS) ^= (1<<BIT))		// Bit umschalten

#define	bis(ADDRESS,BIT)	(ADDRESS & (1<<BIT))		// bit is set?
#define	bic(ADDRESS,BIT)	(!(ADDRESS & (1<<BIT)))		// bit is clear?

#define output_low(port,pin) port &= ~(1<<pin)
#define output_high(port,pin) port |= (1<<pin)
#define set_input(portdir,pin) portdir &= ~(1<<pin)
#define set_output(portdir,pin) portdir |= (1<<pin)
#define check_bit(address,bit) (address & (1<<bit))
#define check_bit_truthval(address,bit) !!(address & (1<<bit))
#define change_bit(number,n,x) number ^= (-x ^ number) & (1 << n);

// local data buffer
unsigned char localBuffer[] = "Pascal is cool!!Pascal is Cool!!";
unsigned char localBufferLength = 0x20;


//################### Serial
#define F_CPU 8000000UL 
#define BAUD 2400
#define MYUBRR F_CPU/16/BAUD-1

//This function is used to initialize the USART
//at a given UBRR value
void USARTInit(uint16_t ubrr_value)
{
   //Set Baud rate
   UBRRL = ubrr_value;
   UBRRH = (ubrr_value>>8);
   //UBRRL = 0x67, UBRRH = 0x00;
   
 

   /*Set Frame Format
    >> Asynchronous mode
    >> No Parity
    >> 1 StopBit
    >> char size 8 */
   UCSRC=(1<<URSEL)|(3<<UCSZ0);

   //Enable The receiver and transmitter
   UCSRB=(1<<RXEN)|(1<<TXEN);
}

////PaulorB
//bool USARTIsDataAvailable(){
 //return (UCSRA & (1<<RXC));	
//}

//This function is used to read the available data
//from USART. This function will wait until data is
//available.
unsigned char USARTReadChar()
{
   //Wait until data is available
   while(!(UCSRA & (1<<RXC)))
   {
      //Do nothing
   }

   //Now when USART receives data from host
   //and it is available in the buffer
   return UDR;
}


//This function writes the given "data" to
//the USART which then transmits it via TX line
void USARTWriteChar(char data)
{
   //Wait until the transmitter is ready
   while(!(UCSRA & (1<<UDRE)))
   {
      //Do nothing
   }

   //Now write the data to USART buffer
   UDR=data;
}


/*************************************************************************
Function: uart_puthex_nibble()
Purpose:  transmit lower nibble as ASCII-hex to UART
Input:    byte value
Returns:  none
**************************************************************************/
void uart_puthex_nibble(const unsigned char b)
{
	unsigned char  c = b & 0x0f;
	if ( c > 9 ) {
		c += 'A'-10;
	}
	else {
		c += '0';
	}
	USARTWriteChar(c);
	} /* uart_puthex_nibble */

/*************************************************************************
Function: uart_puthex_byte()
Purpose:  transmit upper and lower nibble as ASCII-hex to UART
Input:    byte value
Returns:  none
**************************************************************************/
void uart_puthex_byte( const unsigned char  b )
{
	uart_puthex_nibble( b >> 4 );
	uart_puthex_nibble( b );
} /* uart_puthex_byte */

void uart_puts_P(const char *s )
{
	USARTPuts(s);
}
/*************************************************************************
Function: uart_puts()
Purpose:  transmit string to UART
Input:    string to be transmitted
Returns:  none
**************************************************************************/
void USARTPuts(const char *s )
{
	while (*s)
	USARTWriteChar(*s++);

	}/* uart_puts */

/*************************************************************************
Function: uart_put_int()
Purpose:  transmit integer as ASCII to UART
Input:    integer value
Returns:  none
**************************************************************************/
void uart_put_int( const int val )
{
	char buffer[10];
	USARTPuts( itoa( val, buffer, 10 ) );
	} /* uart_puti */

///// SENSOR DE TEMPERATURA

#define MAXSENSORS 5
#define NEWLINESTR "\r\n"


void uart_put_long(unsigned long val) {
	char str_val[20];
	sprintf(str_val,"%lu",val);
	uart_puts_P(str_val);
}







void TraceMessage(const char *s ){
	USARTPuts(s);
}


void PCFan_ON(){
	set_output(DDRD,PD3);
	output_high(PORTD,PD3);
}

void PCFan_OFF(){
	set_output(DDRD,PD3);
	output_low(PORTD,PD3);
}

void Peltier_ON(){
	set_output(DDRB,PB2);
	output_high(PORTB,PB2);
}

void Peltier_OFF(){
	set_output(DDRB,PB2);
	output_low(PORTB,PB2);
}




//################################ START FAN ##################################33
int g_nFanSpeedRPM = 0;
int g_nOverflowCounter =0;

// configuring T1 as external clock
// configuring timer T1
void  ConfigurePCFanWatch() {
	DDRD &= ~(1 << PD4);     // Clear the PB0 pin
	// PD0 is now an input
	PORTD |= (1 << PORTD4);   // turn On the Pull-up
	// PB0 is now an input with pull-up enabled

	TIMSK |= (1 << TOIE0);    // enable timer interrupt

	TCCR0 |= (1 << CS02) | (1 << CS01) | (0 << CS00);
	// Turn on the counter, Clock on falling
	

	
	sei();
}


//ISR (TIMER1_OVF_vect)
ISR (TIMER0_OVF_vect)
{
	// interrupt just fired
	g_nOverflowCounter++;
	
}


void ResetVariables() {
	cli();
	g_nOverflowCounter = 0;
	sei();
}


int GetFanSpeed() {
	ResetVariables();
	
	int nOvfC1 = g_nOverflowCounter;
	TCNT0=0;
	_delay_ms(1000);
	
	int nOvfC2 = g_nOverflowCounter;
	
	//Calculate
	return (int)((((nOvfC2-nOvfC1)*255) + TCNT0)*30);   //* 60   / 2
	
}

//################################ END FAN ##################################33


//############################### TEMPERATURE SENSOR START #######################
#define MAXSENSORS 3
static bool g_IsTempSensorFound=false;
static uint8_t gTemp_measurementstatus=0; // 0=ok,1=error
static uint8_t gSensorIDs[MAXSENSORS][OW_ROMCODE_SIZE];
static int16_t gTempdata[MAXSENSORS]; // temperature times 10
static int8_t gNsensors=0;
static bool g_IsTraceEnabled=false;
static int g_fanRPM=0;
#define TEMPHIST_BUFFER_SIZE 60

// writes to global array gSensorIDs
int8_t search_sensors(void)
{
	uint8_t diff;
	uint8_t nSensors=0;
	for( diff = OW_SEARCH_FIRST;
	diff != OW_LAST_DEVICE && nSensors < MAXSENSORS ; )
	{
		DS18X20_find_sensor( &diff, &(gSensorIDs[nSensors][0]) );
		
		if( diff == OW_PRESENCE_ERR ) {
			return(-1); //No Sensor found
		}
		if( diff == OW_DATA_ERR ) {
			return(-2); //Bus Error
		}
		nSensors++;
	}
	return nSensors;
}

// start a measurement for all sensors on the bus:
void start_temp_meas(void){
	gTemp_measurementstatus=0;
	if ( DS18X20_start_meas(NULL) != DS18X20_OK) {
		USARTPuts("Error reading");
		gTemp_measurementstatus=1;
	}
}

// read the latest measurement off the scratchpad of the ds18x20 sensor
// and store it in gTempdata
void read_temp_meas(void){
	uint8_t i;
	uint8_t subzero, cel, cel_frac_bits;
	for ( i=0; i<gNsensors; i++ ) {
		
		if ( DS18X20_read_meas( &gSensorIDs[i][0], &subzero,
		&cel, &cel_frac_bits) == DS18X20_OK ) {
			gTempdata[i]=cel*10;
			gTempdata[i]+=DS18X20_frac_bits_decimal(cel_frac_bits);
			if(g_IsTraceEnabled == true){
				uart_put_int(gTempdata[i]);
			}
			if (subzero){
				gTempdata[i]=-gTempdata[i];
			}
			}else{
			gTempdata[i]=0;
		}
	}
}

//################################ TEMPERATURE SENSOR END ################################

// slave operations
int g_state=0;
int g_nCommand=-1;
void i2cSlaveReceiveService(u08 receiveDataLength, u08* receiveData)
{
	TraceMessage("\n\ri2cSlaveReceiveService\n\r");
	u08 i;

	// this function will run when a master somewhere else on the bus
	// addresses us and wishes to write data to us

//	showByte(*receiveData);
	//cbi(PORTB, PB6);
	
	
	//STATE MACHINE START
	int j=0;
	int fanRPM=0;
	for(j=0;j<=receiveDataLength;j++){
		switch(g_state){
			case 0:
				if(receiveData[j] == 'A'){
					g_state=1;
					}else{
					g_state=0;
				}
			break;
			
			case 1:
				if(receiveData[j] == 'T'){
					g_state=2;
					}else{
					g_state=0;
				}
			break;
			
			case 2:  //Command
				switch(receiveData[j]) {
					case ASPROTOCOL_CMD_PING:
						localBuffer[0] = 'A';
						localBuffer[1] = 'S';
						localBuffer[2] = 0x1;
						localBufferLength=3;
						g_state=0;
					break;
					case ASPROTOCOL_CMD_TEMP_READ:
						g_state=5;
					break;
					case ASPROTOCOL_CMD_FAN_READ_RPM:
						g_state=4;
					break;
					case ASPROTOCOL_CMD_FAN_CONTROL:
						g_state=3;
					break;
					case ASPROTOCOL_CMD_PELTIER_CONTROL:
						g_state=6;
					break;
					default:
					break;
				};
			break;
			
			case 3:  //ASPROTOCOL_CMD_FAN_CONTROL BYTE
				//depois tem que mudar devido ao CRC
				if(receiveData[j]==0x0){
					//desliga fan
					PCFan_OFF();
				}else
				if(receiveData[j]==0x1){
					//liga fan
					PCFan_ON();
				}
				
				//Response
				localBuffer[0] = 'A';
				localBuffer[1] = 'S';
				localBuffer[2] = 0x6;
				localBuffer[3] = 0xFF; //ACK
				g_state=0;
			break;
		
			case 4: // READ RPM
				//Response
				localBuffer[0] = 'A';
				localBuffer[1] = 'S';
				localBuffer[2] = 0x3;
				localBuffer[3] = (g_fanRPM & 0xff000000)>>24;
				localBuffer[4] = (g_fanRPM & 0x00ff0000)>>16;
				localBuffer[5] = (g_fanRPM & 0x0000ff00)>>8;
				localBuffer[6] = (g_fanRPM & 0x000000ff);
				g_state=0;
			break;
			
			case 5: //ASPROTOCOL_CMD_TEMP_READ
				if(g_IsTempSensorFound==false){
					gNsensors=search_sensors();
					// initialize gTempdata in case somebody reads the web page immediately after reboot
					i=0;
					while(i<MAXSENSORS){
						gTempdata[i]=0;
						i++;
					}
					if (gNsensors>0){
							g_IsTempSensorFound = true;
							start_temp_meas();
							 _delay_ms(100);
							read_temp_meas();
							localBuffer[0] = 'A';
							localBuffer[1] = 'S';
							localBuffer[2] = 0x2;
							localBuffer[3] = (((unsigned int) gTempdata[0]) & 0xff000000)>>24;
							localBuffer[4] = (((unsigned int) gTempdata[0]) & 0x00ff0000)>>16;
							localBuffer[5] = (((unsigned int) gTempdata[0]) & 0x0000ff00)>>8;
							localBuffer[6] = (((unsigned int) gTempdata[0]) & 0x000000ff);
							localBufferLength=7;
							g_state=0;
					}else{
						TraceMessage("Nenhum sensor encontrado");
						//Response
						localBuffer[0] = 'A';
						localBuffer[1] = 'E';
						localBuffer[2] = 0x1;
						localBuffer[3] = 0x0;
						localBuffer[4] = 0x0;
						localBuffer[5] = 0x0;
						localBuffer[6] = 0x0;
						localBufferLength=7;
						g_state=0;
					}
				}else{
					//Just get the temperature
					//Response
					localBuffer[0] = 'A';
					localBuffer[1] = 'S';
					localBuffer[2] = 0x2;
					localBuffer[3] = (((unsigned int) gTempdata[0]) & 0xff000000)>>24;
					localBuffer[4] = (((unsigned int) gTempdata[0]) & 0x00ff0000)>>16;
					localBuffer[5] = (((unsigned int) gTempdata[0]) & 0x0000ff00)>>8;
					localBuffer[6] = (((unsigned int) gTempdata[0]) & 0x000000ff);
					localBufferLength=7;
					g_state=0;
				}
			break;
			case 6:  //ASPROTOCOL_CMD_PELTIER_CONTROL
				//depois tem que mudar devido ao CRC
				if(receiveData[j]==0x0){
					//desliga fan
					Peltier_OFF();
				}else
				if(receiveData[j]==0x1){
					//liga fan
					if(g_fanRPM > 100){
						Peltier_ON();
					}else{
						localBuffer[0] = 'A';
						localBuffer[1] = 'E';
						localBuffer[2] = 0x2; //RPM Fan Low
						localBuffer[3] = 0xFF; //ACK
						localBufferLength=4;
						g_state=0;
						break;
					}
				}
				
				//Response
				localBuffer[0] = 'A';
				localBuffer[1] = 'S';
				localBuffer[2] = 0x7;
				localBuffer[3] = 0xFF; //ACK
				localBufferLength=4;
				g_state=0;
				break;
			
		}
	}
	
	//STATE MACHINE END

	//// copy the received data to a local buffer
	//for(i=0; i<receiveDataLength; i++)
	//{
		//localBuffer[i] = *receiveData++;
	//}
	//localBufferLength = receiveDataLength;
	//
	//localBuffer[localBufferLength] = 0x0;  //so pra teste
	//uart_put_int(localBufferLength);
	//TraceMessage(localBuffer);
}

u08 i2cSlaveTransmitService(u08 transmitDataLengthMax, u08* transmitData)
{
	TraceMessage("\n\i2cSlaveTransmitService\n\r");
	u08 i;

	// this function will run when a master somewhere else on the bus
	// addresses us and wishes to read data from us

//	showByte(*transmitData);
	//cbi(PORTB, PB7);

	// copy the local buffer to the transmit buffer
	for(i=0; i<localBufferLength; i++)
	{
		*transmitData++ = localBuffer[i];
	}

	//localBuffer[0]++;

	return localBufferLength;
}

//Input switch 
//Device ID Selector
//NOT TESTED
BYTE GetDeviceID() {
	//configure some ios as input
	set_input(DDRB,PORTB1); //dip1
	set_input(DDRB,PORTB0); //dip2
	set_input(DDRD,PORTD7); //dip3
	set_input(DDRD,PORTD6); //dip4
	set_input(DDRD,PORTD2); //dip5
	set_input(DDRC,PORTC3); //dip6
	set_input(DDRC,PORTC2); //dip7
	set_input(DDRC,PORTC1); //dip8
	TraceMessage("\n\rDevice (Binary) Start:\n\r");
	uart_put_int(check_bit_truthval(PINB,PINB1)==1?1:0);
	uart_put_int(check_bit_truthval(PINB,PINB0)==1?1:0);
	uart_put_int(check_bit_truthval(PIND,PIND7)==1?1:0);
	uart_put_int(check_bit_truthval(PIND,PIND6)==1?1:0);
	uart_put_int(check_bit_truthval(PIND,PIND2)==1?1:0);
	uart_put_int(check_bit_truthval(PINC,PINC3)==1?1:0);
	uart_put_int(check_bit_truthval(PINC,PINC2)==1?1:0);
	uart_put_int(check_bit_truthval(PINC,PINC1)==1?1:0);
	TraceMessage("\n\rDevice (Binary) End:\n\r");
	BYTE deviceID = change_bit(deviceID,0,check_bit_truthval(PINB,PINB1));
	deviceID = change_bit(deviceID,1,check_bit_truthval(PINB,PINB0));
	deviceID = change_bit(deviceID,2,check_bit_truthval(PIND,PIND7));
	deviceID = change_bit(deviceID,3,check_bit_truthval(PIND,PIND6));
	deviceID = change_bit(deviceID,4,check_bit_truthval(PIND,PIND2));
	deviceID = change_bit(deviceID,5,check_bit_truthval(PINC,PINC3));
	deviceID = change_bit(deviceID,6,check_bit_truthval(PINC,PINC2));
	deviceID = change_bit(deviceID,7,check_bit_truthval(PINC,PINC1));
	TraceMessage("\n\rDevice ID:\n\r");
	uart_put_int(deviceID);
	return deviceID;
}

int main(void)
{
	 DDRC &= ~(1 << 0);
	 PORTC &= ~(1 << 0);
	 _delay_ms(500);
	 PORTC |= (1 << 0);
	 _delay_ms(500);
	
		Peltier_OFF();

/*First Initialize the USART with baud rate = 9600bps
   for Baud rate = 9600bps at 16 MHz frequency
   UBRR value = 103
   */

   USARTInit(103);    //UBRR = 103    BAUD RATE para 1mhz (internal default) 600bps este eh o valor que deve ser colocado no real term
   //agora ta em 9600 ja que estamos rodando a 16mhz
   TraceMessage("\n\rAquaSlave V1 2016 PauloRB\n\r");

	// initialize i2c function library
	i2cInit();
	BYTE test = GetDeviceID();
	// set local device address and allow response to general call
	u08 dev = 11;
	i2cSetLocalDeviceAddr(dev, TRUE);
	// set the Slave Receive Handler function
	// (this function will run whenever a master somewhere else on the bus
	//  writes data to us as a slave)
	i2cSetSlaveReceiveHandler( i2cSlaveReceiveService );
	// set the Slave Transmit Handler function
	// (this function will run whenever a master somewhere else on the bus
	//  attempts to read data from us as a slave)
	i2cSetSlaveTransmitHandler( i2cSlaveTransmitService );
	

	
	ConfigurePCFanWatch();
	sei();

while(1==1) {
	sei();
	eI2cStateType state = i2cGetState();
	if(state == I2C_BUSY){
		TraceMessage("\n\I2C_BUSY!!\n\r");
	}
	if(state == I2C_IDLE){
		TraceMessage("\n\I2C_IDLE!!\n\r");
	}
	if(state == I2C_MASTER_RX){
		TraceMessage("\n\I2C_MASTER_RX!!\n\r");
	}
	if(state == I2C_MASTER_TX){
		TraceMessage("\n\I2C_MASTER_TX!!\n\r");
	}
	if(state == I2C_SLAVE_RX){
		TraceMessage("\n\I2C_SLAVE_RX!!\n\r");
	}
	if(state == I2C_SLAVE_TX){
		TraceMessage("\n\I2C_SLAVE_TX!!\n\r");
	}
	u08 stat = i2cGetStatus();
	uart_put_int(stat);
	

	//Temperature
	int i=0;
	if(g_IsTempSensorFound==false){
		gNsensors=search_sensors();
		// initialize gTempdata in case somebody reads the web page immediately after reboot
		i=0;
		while(i<MAXSENSORS){
			gTempdata[i]=0;
			i++;
		}
		if (gNsensors>0){
			g_IsTempSensorFound = true;
			start_temp_meas();
		}else{
			TraceMessage("Nenhum sensor encontrado");
		}
	}else{
		start_temp_meas();
		 _delay_ms(100);
		 read_temp_meas();
	}
	
	g_fanRPM = GetFanSpeed();


	_delay_ms(100);
}

}

