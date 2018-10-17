#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include "adc128d818.h"

//#define DEBUG

#ifdef DEBUG
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif

#ifdef DEBUG  // discipline to only use dump in debug mode
void dump(void *buf, int count) {
  int i = 0;
  for( i = 0; i < count; i++ ) {
    if( (i%8) == 0 ) 
      printf( "\n" );
    printf( "%02x ", ((char *) buf)[i] );
  }
}
#endif

// caller is responsible for allocating a large enough data buffer
// addresses 0x20-0x40 will return two chars, all else returns 1 char
int adc128d818_read_reg(int device_address, unsigned char address, unsigned char *data) {
  int i2cfd;
  unsigned char i2cbuf[4]; 

  struct i2c_msg msg[2];
		
  struct i2c_ioctl_rdwr_data {
    struct i2c_msg *msgs;  /* ptr to array of simple messages */              
    int nmsgs;             /* number of messages to exchange */ 
  } msgst;
  
  i2cfd = open("/dev/i2c-1", O_RDWR);
  if( i2cfd < 0 ) {
    perror("Unable to open /dev/i2c-1\n");
    i2cfd = 0;
    return 1;
  }
  if( ioctl( i2cfd, I2C_SLAVE, device_address) < 0 ) {
    perror("Unable to set I2C slave device\n" );
    printf( "Address: %02x\n", device_address );
    return 1;
  }

  //////// read back the status register
  i2cbuf[0] = address;

  msg[0].addr = device_address;
  msg[0].flags = 0; // no flag means do a write
  msg[0].len = 1;
  msg[0].buf = i2cbuf;

  if( address < 0x40 && address >= 0x20 ) {
    // 16-bit transaction for data register readback buffer
    msg[1].addr = device_address;
    msg[1].flags = I2C_M_RD;
    msg[1].len = 2;
    msg[1].buf = data;
  } else {
    // 8-bit transaction for status register readback buffer
    msg[1].addr = device_address;
    msg[1].flags = I2C_M_RD;
    msg[1].len = 1;
    msg[1].buf = data;
  }

  msgst.msgs = msg;	
  msgst.nmsgs = 2;
  
  if (ioctl(i2cfd, I2C_RDWR, &msgst) < 0){
    perror("Transaction failed\n" );
    return -1;
  }
  
  return (0);
}

// helper function to read conversions
int adc128d818_read_conv(int channel_code, unsigned short *data) {
  unsigned char buf[2];
  int ret;
  int device_address;

  if( channel_code & 0x8 ) {
    device_address = ADC128_I2C_ADR_1;
  } else {
    device_address = ADC128_I2C_ADR_0;
  }
  
  ret = adc128d818_read_reg(device_address, ADC128_REG_IN((channel_code & 0xFF)), buf);

  *data = buf[1] | (buf[0] << 8);
  
  return ret;
}

int adc128d818_write_reg( int device_address, unsigned char address, unsigned char data ) {
  int i2cfd;
  unsigned char i2cbuf[2]; 

  struct i2c_msg msg[2];
		
  struct i2c_ioctl_rdwr_data {
    struct i2c_msg *msgs;  /* ptr to array of simple messages */              
    int nmsgs;             /* number of messages to exchange */ 
  } msgst;
  
  i2cfd = open("/dev/i2c-1", O_RDWR);
  if( i2cfd < 0 ) {
    perror("Unable to open /dev/i2c-1\n");
    i2cfd = 0;
    return 1;
  }
  if( ioctl( i2cfd, I2C_SLAVE, device_address) < 0 ) {
    perror("Unable to set I2C slave device\n" );
    printf( "Address: %02x\n", device_address );
    return 1;
  }

  //////// write data, address, and commit using multi-byte write
  i2cbuf[0] = address;
  i2cbuf[1] = data;

  msg[0].addr = device_address;
  msg[0].flags = 0; // no flag means do a write
  msg[0].len = 2;
  msg[0].buf = i2cbuf;
  
#ifdef DEBUG
  dump(i2cbuf, 2);
#endif

  msgst.msgs = msg;	
  msgst.nmsgs = 1;

  if (ioctl(i2cfd, I2C_RDWR, &msgst) < 0){
    perror("Transaction failed\n" );
    return -1;
  }

  close( i2cfd );
  
  return 0;
}

void default_adc128d818() {
  unsigned char data;
  
  adc128d818_read_reg( ADC128_I2C_ADR_0, ADC128_REG_MAN_ID, &data );
  if( data != 0x1 )
    fprintf( stderr, "ADC128 0 ID mismatch\n" );
  adc128d818_read_reg( ADC128_I2C_ADR_1, ADC128_REG_MAN_ID, &data );
  if( data != 0x1 )
    fprintf( stderr, "ADC128 1 ID mismatch\n" );
  
  adc128d818_read_reg( ADC128_I2C_ADR_0, ADC128_REG_DEV_ID, &data );
  if( data != 0x5 )
    fprintf( stderr, "ADC128 0 ID mismatch\n" );
  adc128d818_read_reg( ADC128_I2C_ADR_1, ADC128_REG_DEV_ID, &data );
  if( data != 0x5 )
    fprintf( stderr, "ADC128 1 ID mismatch\n" );
  
  // put device in shutdown to allow for programming
  adc128d818_write_reg( ADC128_I2C_ADR_0, ADC128_REG_CONFIG, 0x0 );
  adc128d818_write_reg( ADC128_I2C_ADR_1, ADC128_REG_CONFIG, 0x0 );

  // set conversion rate to continuous
  adc128d818_write_reg( ADC128_I2C_ADR_0, ADC128_REG_CONV_RATE, 0x1 );
  adc128d818_write_reg( ADC128_I2C_ADR_1, ADC128_REG_CONV_RATE, 0x1 );

  // select internal VREF, and "mode 1" (all single ended inputs active)
  adc128d818_write_reg( ADC128_I2C_ADR_0, ADC128_REG_CONFIG_ADV, 0x1 << 1 );
  adc128d818_write_reg( ADC128_I2C_ADR_1, ADC128_REG_CONFIG_ADV, 0x1 << 1 );
  
  // startup converter & enable interrupts
  adc128d818_write_reg( ADC128_I2C_ADR_0, ADC128_REG_CONFIG, 0x3 );
  adc128d818_write_reg( ADC128_I2C_ADR_1, ADC128_REG_CONFIG, 0x3 );

  // make sure both converters finish their power-up sequence before continuing
  do {
    adc128d818_read_reg( ADC128_I2C_ADR_0, ADC128_REG_BUSY_STATUS, &data );
  } while( data & 0x2 );
  
  do {
    adc128d818_read_reg( ADC128_I2C_ADR_1, ADC128_REG_BUSY_STATUS, &data );
  } while( data & 0x2 );
}

int main() {
  int i;
  unsigned short conv;

  printf( "Set defaults on converter...\n" );
  default_adc128d818();

  for( i = 0; i < 8; i++ ) {
    adc128d818_read_conv( i, &conv );
    printf( "%d: %d\n", i, conv );
  }
  for( i = 0; i < 8; i++ ) {
    adc128d818_read_conv( i | 0x100, &conv );
    printf( "%d: %d\n", i, conv );
  }

  adc128d818_read_conv( ADC128_P1p2VTT, &conv );
  printf( "+1.2VTT reading: %g2.2V\n", ((double)conv) * 0.000625 );
  adc128d818_read_conv( ADC128_P1p2VE, &conv );
  printf( "+1.2VE reading: %g2.2V\n", ((double)conv) * 0.000625 );
  
  adc128d818_read_conv( ADC128_P1p0V, &conv );
  printf( "+1.0V reading: %g2.2V\n", ((double)conv) * 0.000625 );

  // constant is computed by RL * RS * 200e-6A/V
  // at 16.2k RL, 0.47ohm RS. Gives ~1mA at 1 LSB, with about 3.9A full scale
  // at 10.0k RL, we get 0.9400 as the constant, 0.6mA @ 1LSB, with 2.4A full scale
  adc128d818_read_conv( ADC128_I12p0A, &conv );
  printf( "12V in-line current reading: %g1.3A\n", (((double)conv) * 0.000625) * 0.9400 );
  
}
