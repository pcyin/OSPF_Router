#ifndef COMMON_H
#define COMMON_H
#include <sys/types.h>
#include <netinet/in.h>

class Common
{
public:
	/*
	* from opensource.apple.com
	*/
	static uint16_t create_osi_cksum (const uint8_t *pptr, int checksum_offset, int length)
	{
		int x;
		int y;
		uint32_t mul;
		uint32_t c0;
		uint32_t c1;
		uint16_t checksum;
		int index;

		checksum = 0;

		c0 = 0;
		c1 = 0;

		for (index = 0; index < length; index++) {
			/*
			 * Ignore the contents of the checksum field.
			 */
			if (index == checksum_offset ||
				index == checksum_offset+1) {
				c1 += c0;
				pptr++;
			} else {
				c0 = c0 + *(pptr++);
				c1 += c0;
			} 
		}

		c0 = c0 % 255;
		c1 = c1 % 255;

		mul = (length - checksum_offset)*(c0);
  
		x = mul - c0 - c1;
		y = c1 - mul - 1;

		if ( y >= 0 ) y++;
		if ( x < 0 ) x--;

		x %= 255;
		y %= 255;


		if (x == 0) x = 255;
		if (y == 0) y = 255;

		y &= 0x00FF;
		checksum = ((x << 8) | y);
  
		return checksum;
	}
};

#endif
