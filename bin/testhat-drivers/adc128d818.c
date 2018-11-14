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

/*
  post-compilation:
    sudo chown root tester-driver
    sudo chmod u+s tester-driver
 */

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
// not suitable for reading conversion results, use the adc128d818_read_conv function instead
int adc128d818_read_reg(int device_address, unsigned char address, unsigned char *data) {
  int i2cfd;
  unsigned char i2cbuf[1]; 

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

  i2cbuf[0] = address;

  msg[0].addr = device_address;
  msg[0].flags = 0; // no flag means do a write
  msg[0].len = 1;
  msg[0].buf = i2cbuf;

  // 8-bit transaction for status register readback buffer
  msg[1].addr = device_address;
  msg[1].flags = I2C_M_RD;
  msg[1].len = 1;
  msg[1].buf = data;

  msgst.msgs = msg;	
  msgst.nmsgs = 2;
  
  if (ioctl(i2cfd, I2C_RDWR, &msgst) < 0){
    perror("Transaction failed\n" );
    return -1;
  }

  close(i2cfd);
  
  return (0);
}

int open_i2c_dev(int i2cbus, char *filename, size_t size, int quiet)
{
	int file;

	snprintf(filename, size, "/dev/i2c/%d", i2cbus);
	filename[size - 1] = '\0';
	file = open(filename, O_RDWR);

	if (file < 0 && (errno == ENOENT || errno == ENOTDIR)) {
		sprintf(filename, "/dev/i2c-%d", i2cbus);
		file = open(filename, O_RDWR);
	}

	if (file < 0 && !quiet) {
		if (errno == ENOENT) {
			fprintf(stderr, "Error: Could not open file "
				"`/dev/i2c-%d' or `/dev/i2c/%d': %s\n",
				i2cbus, i2cbus, strerror(ENOENT));
		} else {
			fprintf(stderr, "Error: Could not open file "
				"`%s': %s\n", filename, strerror(errno));
			if (errno == EACCES)
				fprintf(stderr, "Run as root?\n");
		}
	}

	return file;
}

int set_slave_addr(int file, int address, int force)
{
	/* With force, let the user read from/write to the registers
	   even when a driver is also running */
	if (ioctl(file, force ? I2C_SLAVE_FORCE : I2C_SLAVE, address) < 0) {
		fprintf(stderr,
			"Error: Could not set address to 0x%02x: %s\n",
			address, strerror(errno));
		return -errno;
	}

	return 0;
}

// helper function to read conversions
int adc128d818_read_conv(int channel_code, unsigned short *ret) {
  int i2cfd;
  int device_address;

  struct i2c_smbus_ioctl_data args;
  union i2c_smbus_data data;
  __s32 err;
  char filename[20];
  int pec = 0;
  unsigned short temp;
  
  if( channel_code & 0x100 ) {
    device_address = ADC128_I2C_ADR_1;
  } else {
    device_address = ADC128_I2C_ADR_0;
  }

  i2cfd = open_i2c_dev(1, filename, sizeof(filename), 0);
  
  set_slave_addr(i2cfd, device_address, 1);

  if (pec && ioctl(i2cfd, I2C_PEC, 1) < 0) {
    fprintf(stderr, "Error: Could not set PEC: %s\n",
	    strerror(errno));
    close(i2cfd);
    exit(1);
  }

  args.read_write = I2C_SMBUS_READ;
  args.command = ADC128_REG_IN((channel_code & 0xFF));
  args.size = I2C_SMBUS_WORD_DATA;
  args.data = &data;

  err = ioctl(i2cfd, I2C_SMBUS, &args);
  close( i2cfd );

  temp = 0x0FFFF & data.word;
  *ret = (((temp >> 8) & 0xff) | ((temp & 0xff) << 8)) >>  4;
  //  *ret = 0x0ffff & data.word;
  
  return err;
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
    fprintf( stderr, "ADC128 0 mfg ID mismatch: %x\n", data );
  adc128d818_read_reg( ADC128_I2C_ADR_1, ADC128_REG_MAN_ID, &data );
  if( data != 0x1 )
    fprintf( stderr, "ADC128 1 mfg ID mismatch: %x\n", data );
  
  adc128d818_read_reg( ADC128_I2C_ADR_0, ADC128_REG_DEV_ID, &data );
  if( data != 0x9 )
    fprintf( stderr, "ADC128 0 rev ID mismatch: %x\n", data );
  adc128d818_read_reg( ADC128_I2C_ADR_1, ADC128_REG_DEV_ID, &data );
  if( data != 0x9 )
    fprintf( stderr, "ADC128 1 rev ID mismatch: %x\n", data );
  
  // make sure both converters finish their power-up sequence before continuing
  do {
    adc128d818_read_reg( ADC128_I2C_ADR_0, ADC128_REG_BUSY_STATUS, &data );
  } while( data & 0x2 );
  
  do {
    adc128d818_read_reg( ADC128_I2C_ADR_1, ADC128_REG_BUSY_STATUS, &data );
  } while( data & 0x2 );

  // put device in shutdown to allow for programming
  adc128d818_write_reg( ADC128_I2C_ADR_0, ADC128_REG_CONFIG, 0x0 );
  adc128d818_write_reg( ADC128_I2C_ADR_1, ADC128_REG_CONFIG, 0x0 );

  // select internal VREF, and "mode 1" (all single ended inputs active)
  adc128d818_write_reg( ADC128_I2C_ADR_0, ADC128_REG_CONFIG_ADV, 0x1 << 1 );
  adc128d818_write_reg( ADC128_I2C_ADR_1, ADC128_REG_CONFIG_ADV, 0x1 << 1 );
  
  // set conversion rate to continuous
  adc128d818_write_reg( ADC128_I2C_ADR_0, ADC128_REG_CONV_RATE, 0x1 );
  adc128d818_write_reg( ADC128_I2C_ADR_1, ADC128_REG_CONV_RATE, 0x1 );

  // sit disable to 0 on used channels
  adc128d818_write_reg( ADC128_I2C_ADR_0, ADC128_REG_DISABLE, 0x0 );
  adc128d818_write_reg( ADC128_I2C_ADR_1, ADC128_REG_DISABLE, 0x7c ); // 0111 1100

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

  usleep(250000);  // for some reason we still have to wait even though busy status is clear...
}

static void help(void) __attribute__ ((noreturn));

static void help(void)
{
	fprintf(stderr,
		"Usage: tester-driver [-i] [-v] [-j] [-t] [-s]\n"
		"  -v  report 12V DUT voltage in mV\n"
		"  -i  report DUT current in mA\n"
		"  -j  all parameters in JSON\n"
		"  -t  pass/fail for all voltage/current quicktest\n"
		"  -s  pass/fail for just 12V and current test\n"
		"  -q  12V/IDUT in json\n"
		"  default: report current/voltage in loop forever\n");
	exit(1);
}

// tolerance is a fraction, e.g. 10% tolerance is 0.1
// returns 0 if in tolerance, 1 if out of tolerance
int out_of_tolerance(double value, double nominal, double tolerance) {
  if( (value < (nominal * (1 + tolerance))) && (value > (nominal * (1 - tolerance))) ) {
    return 0;
  } else {
    return 1;
  }
  
}
#define DEFAULT_TOLERANCE 0.045  // 4.5% tolerance on all internal supplies -- 0.5% slop due to test jig itself for total 5% guaranteed range
#define ETH_TOLERANCE 0.065  // 7% tolerance on 1.2VE, the LDO is sloppy
#define ADAPTER_TOLERANCE 0.1  // allow 10% tolerance on power adapter, this supply is expected to be messy

int comprehensive_check() {
  double value;
  unsigned short conv;
  int ret = 0;
  
  adc128d818_read_conv( ADC128_P1p2VTT, &conv );
  value = (((double)conv) * 0.000625);
  ret += out_of_tolerance( value, 1.2, DEFAULT_TOLERANCE );
  
  adc128d818_read_conv( ADC128_P1p2VE, &conv );
  value = (((double)conv) * 0.000625);
  ret += out_of_tolerance( value, 1.2, ETH_TOLERANCE );
  
  adc128d818_read_conv( ADC128_P1p8V, &conv );
  value = (((double)conv) * 0.000625);
  ret += out_of_tolerance( value, 1.8, DEFAULT_TOLERANCE );
  
  adc128d818_read_conv( ADC128_P1p5V, &conv );
  value = (((double)conv) * 0.000625);
  ret += out_of_tolerance( value, 1.5, DEFAULT_TOLERANCE );
  
  adc128d818_read_conv( ADC128_P1p5V, &conv );
  value = (((double)conv) * 0.000625);
  ret += out_of_tolerance( value, 1.5, DEFAULT_TOLERANCE );
  
  adc128d818_read_conv( ADC128_P3p3V, &conv );
  value = (((double)conv) * 0.000625) / 0.5;
  ret += out_of_tolerance( value, 3.3, DEFAULT_TOLERANCE );
  
  adc128d818_read_conv( ADC128_P3p3VE, &conv );
  value = (((double)conv) * 0.000625) / 0.5;
  ret += out_of_tolerance( value, 3.3, DEFAULT_TOLERANCE );
  
  adc128d818_read_conv( ADC128_P5p0V, &conv );
  value = (((double)conv) * 0.000625) / 0.2325;
  ret += out_of_tolerance( value, 5.0, DEFAULT_TOLERANCE );
  
  adc128d818_read_conv( ADC128_P12p0V, &conv );
  value = (((double)conv) * 0.000625) / 0.0909;
  ret += out_of_tolerance( value, 12.0, ADAPTER_TOLERANCE );
  
  adc128d818_read_conv( ADC128_P1p0V, &conv );
  value = (((double)conv) * 0.000625);
  ret += out_of_tolerance( value, 1.0, DEFAULT_TOLERANCE );
  
  adc128d818_read_conv( ADC128_P0p75VTT, &conv );
  value = (((double)conv) * 0.000625);
  ret += out_of_tolerance( value, 0.75, DEFAULT_TOLERANCE );

  
  adc128d818_read_conv( ADC128_I12p0A, &conv );
  value = (((double)conv) * 0.000625) * 0.5;
  // coarsely, current should be >8mA, and less than 0.6A
  // 14mA is roughly the idle/leakage of just the board
  // and we see maybe 0.5A peak consumption during active periods
  if( value < 0.008 )
    ret += 1;
  if( value > 0.6 )
    ret += 1;

  return ret;
}

int quick_check() {
  double value;
  unsigned short conv;
  int ret = 0;
  
  adc128d818_read_conv( ADC128_P12p0V, &conv );
  value = (((double)conv) * 0.000625) / 0.0909;
  ret += out_of_tolerance( value, 12.0, ADAPTER_TOLERANCE );
  
  adc128d818_read_conv( ADC128_I12p0A, &conv );
  value = (((double)conv) * 0.000625) * 0.5;
  // coarsely, current should be >8mA, and less than 0.6A
  // 14mA is roughly the idle/leakage of just the board
  // and we see maybe 0.5A peak consumption during active periods
  if( value < 0.008 )
    ret += 1;
  if( value > 0.6 )
    ret += 1;

  return ret;
}

int main(int argc, char *argv[]) {
  unsigned short conv;
  int flags = 0;
  int test_v = 0, test_i = 0, test_j = 0, test_t = 0, test_s = 0, test_q = 0;
  int errcnt = 0;

  while (1+flags < argc && argv[1+flags][0] == '-') {
    switch (argv[1+flags][1]) {
    case 'v': test_v = 1; break;
    case 'i': test_i = 1; break;
    case 'j': test_j = 1; break;
    case 't': test_t = 1; break;
    case 's': test_s = 1; break;
    case 'q': test_q = 1; break;
    default:
      fprintf(stderr, "Error: Unsupported option "
	      "\"%s\"!\n", argv[1+flags]);
      help();
      exit(1);
    }
    flags++;
  }

  default_adc128d818();

  if( test_v ) {
    adc128d818_read_conv( ADC128_P12p0V, &conv );
    printf( "%d\n", (int) (((double)conv) * 0.625 / 0.0909) );
    return 0;
  }

  if( test_i ) {
    adc128d818_read_conv( ADC128_I12p0A, &conv );
    printf( "%d\n", (int) (((double)conv) * 0.625 * 0.5) );
    return 0;
  }

  if( test_t ) {
    return comprehensive_check();
  }

  if( test_s ) {
    return quick_check();
  }

  if( test_q ) {
    printf( "{ \"dna\":-1, \"subtest\":\"power\", \"msg\":[{" );
    
    printf( "\"12.0V\":" );
    adc128d818_read_conv( ADC128_P12p0V, &conv );
    printf( "%d, ", (int) (((double)conv) * 0.625 / 0.0909)  );

    printf( "\"IDUT\":" );
    adc128d818_read_conv( ADC128_I12p0A, &conv );
    printf( "%d ", (int) (((double)conv) * 0.625 * 0.5) );
    
    printf( "}] }\n" );

    errcnt = quick_check();
    printf( "{ \"dna\":-1, \"subtest\":\"power\", \"msg\":[{\"errcnt\":%d}]}\n", errcnt );
    return errcnt;
  }
  
  if( test_j ) {
    printf( "{ \"dna\":-1, \"subtest\":\"voltage\", \"msg\":[{" );
    
    printf( "\"1.2VTT\":" );
    adc128d818_read_conv( ADC128_P1p2VTT, &conv );
    printf( "%d, ", (int) (((double)conv) * 0.625) );

    printf( "\"1.2VE\":" );
    adc128d818_read_conv( ADC128_P1p2VE, &conv );
    printf( "%d, ", (int) (((double)conv) * 0.625) );

    printf( "\"1.8V\":" );
    adc128d818_read_conv( ADC128_P1p8V, &conv );
    printf( "%d, ", (int) (((double)conv) * 0.625) );
    
    printf( "\"1.5V\":" );
    adc128d818_read_conv( ADC128_P1p5V, &conv );
    printf( "%d, ", (int) (((double)conv) * 0.625) );
    
    printf( "\"3.3V\":" );
    adc128d818_read_conv( ADC128_P3p3V, &conv );
    printf( "%d, ", (int) (((double)conv) * 0.625 / 0.5) );
    
    printf( "\"3.3VE\":" );
    adc128d818_read_conv( ADC128_P3p3VE, &conv );
    printf( "%d, ", (int) (((double)conv) * 0.625 / 0.5) );
    
    printf( "\"5.0V\":" );
    adc128d818_read_conv( ADC128_P5p0V, &conv );
    printf( "%d, ", (int) (((double)conv) * 0.625 / 0.2325)  );
    
    printf( "\"1.0V\":" );
    adc128d818_read_conv( ADC128_P1p0V, &conv );
    printf( "%d, ", (int) (((double)conv) * 0.625) );
    
    printf( "\"0.75VTT\":" );
    adc128d818_read_conv( ADC128_P0p75VTT, &conv );
    printf( "%d, ", (int) (((double)conv) * 0.625) );
    
    printf( "\"12.0V\":" );
    adc128d818_read_conv( ADC128_P12p0V, &conv );
    printf( "%d, ", (int) (((double)conv) * 0.625 / 0.0909)  );

    printf( "\"IDUT\":" );
    adc128d818_read_conv( ADC128_I12p0A, &conv );
    printf( "%d ", (int) (((double)conv) * 0.625 * 0.5) );
    
    printf( "}] }\n" );

    errcnt = comprehensive_check();
    printf( "{ \"dna\":-1, \"subtest\":\"voltage\", \"msg\":[{\"errcnt\":%d}]}\n", errcnt );
    return errcnt;
  }
  


  while(1) {
    adc128d818_read_conv( ADC128_P12p0V, &conv );
    printf( "+12.0V reading: %2.4gV code %d\n", ((double)conv) * 0.000625 / 0.0909, conv );

    // constant is computed by RL * RS * 200e-6A/V
    // at 16.2k RL, 0.47ohm RS. Gives ~1mA at 1 LSB, with about 3.9A full scale
    // at 10.0k RL, we get 0.9400 as the constant, 0.6mA @ 1LSB, with 2.4A full scale
    // at 0.1ohm RS, 100k RL gives 2.0V at 1A, with 1.28A full scale
    adc128d818_read_conv( ADC128_I12p0A, &conv );
    printf( "12V in-line current reading: %1.4gA, code %d\n", (((double)conv) * 0.000625) * 0.5, conv );
    usleep(500000);
  }
  return 0;
  
}
