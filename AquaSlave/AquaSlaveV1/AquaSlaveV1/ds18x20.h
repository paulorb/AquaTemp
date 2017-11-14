// vim:sw=8:ts=8:si:et:
#ifndef DS18X20_H_
#define DS18X20_H_

/* return values */
#define DS18X20_OK 0x00
#define DS18X20_ERROR 0x01
#define DS18X20_START_FAIL 0x02
#define DS18X20_ERROR_CRC 0x03

/* DS18X20 specific values (see datasheet) */
#define DS18S20_ID 0x10
#define DS18B20_ID 0x28

#define DS18X20_CONVERT_T 0x44
#define DS18X20_READ 0xBE
#define DS18X20_WRITE 0x4E
#define DS18X20_EE_WRITE 0x48
#define DS18X20_EE_RECALL 0xB8
#define DS18X20_READ_POWER_SUPPLY 0xB4

#define DS18B20_CONF_REG 4
#define DS18B20_9_BIT 0
#define DS18B20_10_BIT (1<<5)
#define DS18B20_11_BIT (1<<6)
#define DS18B20_12_BIT ((1<<6)|(1<<5))

// indefined bits in LSB if 18B20 != 12bit
#define DS18B20_9_BIT_UNDF ((1<<0)|(1<<1)|(1<<2))
#define DS18B20_10_BIT_UNDF ((1<<0)|(1<<1))
#define DS18B20_11_BIT_UNDF ((1<<0))
#define DS18B20_12_BIT_UNDF 0

// conversion times in ms
#define DS18B20_TCONV_12BIT 750
#define DS18B20_TCONV_11BIT DS18B20_TCONV_12_BIT/2
#define DS18B20_TCONV_10BIT DS18B20_TCONV_12_BIT/4
#define DS18B20_TCONV_9BIT DS18B20_TCONV_12_BIT/8
#define DS18S20_TCONV DS18B20_TCONV_12_BIT

// constant to convert the fraction bits to cel*(10^-4)
#define DS18X20_FRACCONV 625

#define DS18X20_SP_SIZE 9

/* for description of functions see ds18x20.c */
extern void DS18X20_find_sensor(uint8_t *diff, uint8_t *id);
extern uint8_t DS18X20_start_meas( uint8_t *id);
extern uint8_t DS18X20_read_meas(uint8_t *id, uint8_t *subzero, uint8_t *cel, uint8_t *cel_frac_bits);
extern void DS18X20_meas_to_cel( uint8_t fc, uint8_t *sp, uint8_t* subzero, uint8_t* cel, uint8_t* cel_frac_bits);
extern uint8_t DS18X20_frac_bits_decimal(uint8_t cel_frac_bits);
extern uint8_t DS18X20_read_scratchpad( uint8_t *id, uint8_t sp[]);
extern uint16_t DS18X20_show_id_print_buf( uint8_t *id, uint16_t plen,uint8_t *buf);

#endif
