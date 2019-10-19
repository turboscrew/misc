/*
 * watchpoint.c
 *
 *  Created on: 29.3.2019
 *      Author: Juha Aaltonen
 */

#include "hal.h"
#include <stdint.h>
#include "watchpoint.h"

#define DWT_CTRL		(*((uint32_t *)DWT_ADDRESS))
#define DWT_COMP_0		(*((uint32_t *)(DWT_ADDRESS + 0x20)))
#define DWT_MASK_0		(*((uint32_t *)(DWT_ADDRESS + 0x24)))
#define DWT_FUNCTION_0	(*((uint32_t *)(DWT_ADDRESS + 0x28)))
#define DWT_COMP_1		(*((uint32_t *)(DWT_ADDRESS + 0x20 + 1*16)))
#define DWT_MASK_1		(*((uint32_t *)(DWT_ADDRESS + 0x24 + 1*16)))
#define DWT_FUNCTION_1	(*((uint32_t *)(DWT_ADDRESS + 0x28 + 1*16)))
#define DWT_CYCCNT		(*((uint32_t *)(DWT_ADDRESS + 0x04)))

//#define DWT_LAR		(*((uint32_t *)0xE0001FB0))
//#define DWT_LAR		(*((uint32_t *)0xE0000FB0))

#define DWT_DHCSR (*((uint32_t *)DHCSR_ADDR))
#define DWT_DEMCR (*((uint32_t *)(DHCSR_ADDR + 12)))

/*
	The topmost hex digit of DWT_CTRL shows the number of implemented comparators
	DWT_COMPn (address or value to match)
	DWT_MASKn (How many low order bits are masked: maximum can be queried by writing 0x1F and reading back)

	DWT_FUNCTIONn & 0x01000000 = match (r/o, reading clears)
	DWT_FUNCTIONn & 0x000F0000 = datavaddr1 (linked comparator number)
	DWT_FUNCTIONn & 0x0000F000 = datavaddr0 (linked comparator number)
	DWT_FUNCTIONn & 0x00000C00 = datavsize (data value size: 0=byte, 1=half word, 2=word)
	DWT_FUNCTIONn & 0x00000200 = lnk1ena (use 2nd linked comparator)
	DWT_FUNCTIONn & 0x00000100 = datavmatch (1 if data match, 0 if address match)
	DWT_FUNCTIONn & 0x00000080 = cycmatch (1 if cycle match)
	DWT_FUNCTIONn & 0x00000040 = emitrange (1 = enable trace packets)
	DWT_FUNCTIONn & 0x0000000F = function: 5 = read access, 6 = write access, 7 = r/w access
 */

/* The region size should be power of two and aligned accordingly, because the check is done by just masking
the low order address bits; the address is masked the same way, so it's enough that the address is inside the region */
/* address = (start) address of the region, region_sz = region size (2^n), data = data value, it's used as a condition,
   datasize = required access size (byte half word, word), accessmode = read, write or read/write */
/* if datasize = 0, data is not used as a condition, 1 = byte, 2 = half word, 3 = word */
/* if accessmode = 0, the watch region is removed, 1 = read, 2 = write, 3 = read/write */

void set_watchpoint(uint32_t address, uint32_t region_sz, uint32_t data, uint8_t datasize, uint8_t accessmode)
{
	uint32_t mask = 0;
	uint32_t tmp;

#if 1
	asm volatile (
			"mrs %[var_reg], CONTROL\n\t"
			:[var_reg] "=r" (tmp)::
			);
#endif
	//DWT_LAR = 0xC5ACCE55;

	tmp = DWT_CTRL;
	tmp = DWT_DHCSR;
	tmp = DWT_DEMCR;
	if ((tmp & (1<<24)) == 0) // if TRACENA
	{
		DWT_DEMCR |= (1<<24); // DWT disabled, enable it
	}
	tmp = DWT_DEMCR;

	while(region_sz) {
		region_sz >>= 1;
		mask++;
	}
	/* address region to watch */
	DWT_COMP_0 = address;
	DWT_MASK_0 = mask;
	if (datasize !=0) {
		/* address and data match used */
		datasize -=1;
		DWT_FUNCTION_0 = 0; // DWT_COMP_0 is linked address comparator
		tmp = 0x0100; // data match + linked comparator 1
		tmp |= (datasize << 10);
		DWT_COMP_1 = data; // DWT_COMP_1 is the data value comparator
		DWT_MASK_1 = 0;
		if (accessmode == 0) DWT_FUNCTION_1 = 0;
		else DWT_FUNCTION_1 = tmp | (4 + accessmode);
	} else {
		/* only address match used */
		if (accessmode == 0) DWT_FUNCTION_0 = 0;
		else DWT_FUNCTION_0 = 4 + accessmode;
	}
	if (accessmode == 0)
	{
		mask = DWT_FUNCTION_0;
	}
}
/*
void DebugMon_Handler(void)
{
	while(1);
}
*/
