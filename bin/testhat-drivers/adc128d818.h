int adc128d818_read_reg(int device_address, unsigned char address, unsigned char *data);
int adc128d818_read_conv(int channel_code, unsigned short *data);
int adc128d818_write_reg( int device_address, unsigned char address, unsigned char data );
void default_adc128d818();

#define ADC128_I2C_ADR_0  0x1D
#define ADC128_I2C_ADR_1  0x37

/* registers */
#define ADC128_REG_IN_MAX(nr)		(0x2a + (nr) * 2)
#define ADC128_REG_IN_MIN(nr)		(0x2b + (nr) * 2)
#define ADC128_REG_IN(nr)		(0x20 + (nr))

#define ADC128_REG_TEMP			0x27
#define ADC128_REG_TEMP_MAX		0x38
#define ADC128_REG_TEMP_HYST		0x39

#define ADC128_REG_CONFIG		0x00
#define ADC128_REG_ALARM		0x01
#define ADC128_REG_MASK			0x03
#define ADC128_REG_CONV_RATE		0x07
#define ADC128_REG_DISABLE		0x08
#define ADC128_REG_ONESHOT		0x09
#define ADC128_REG_SHUTDOWN		0x0a
#define ADC128_REG_CONFIG_ADV		0x0b
#define ADC128_REG_BUSY_STATUS		0x0c

#define ADC128_REG_MAN_ID		0x3e
#define ADC128_REG_DEV_ID		0x3f

// format: ADC number | ADC channel
#define ADC128_P1p2VTT    (0x0 << 8 | 0x0)
#define ADC128_P1p2VE     (0x0 << 8 | 0x1)
#define ADC128_P1p8V      (0x0 << 8 | 0x2)
#define ADC128_P1p5V      (0x0 << 8 | 0x3)
#define ADC128_P3p3V      (0x0 << 8 | 0x4)
#define ADC128_P3p3VE     (0x0 << 8 | 0x5)
#define ADC128_P5p0V      (0x0 << 8 | 0x6)
#define ADC128_P12p0V     (0x0 << 8 | 0x7)
  
#define ADC128_P1p0V      (0x1 << 8 | 0x0)
#define ADC128_P0p75VTT   (0x1 << 8 | 0x1)
#define ADC128_I12p0A     (0x1 << 8 | 0x7)
