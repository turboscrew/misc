/*
 * rom_table.c
 *
 *  Created on: 15.4.2019
 *      Author: Juha Aaltonen
 */
#include <stdio.h>

typedef struct {
	uint16_t code:8;
	uint16_t flag:4;
	uint16_t cont:4;
} designer_code_t;

typedef struct {
	union {
		uint16_t code;
		designer_code_t designer;
	} code;
	char *name;
} designer_t;

typedef union {
	struct {
		uint16_t designer_index;
		uint16_t partnum;
	} codes;
	uint32_t key;
} comp_id_t;

typedef struct {
	comp_id_t id;
	char *descr;
} component_data_t;

char buffer[2048];
uint16_t lastbuff = 0;

/* table of designers */
const designer_t designers[] =
{
	{{.designer.code = 0x3B, .designer.flag = 1, .designer.cont = 4},
		"ARM"
	},
	{{.designer.code = 0x41, .designer.flag = 0, .designer.cont = 0},
		"ARM (legacy)"
	},
	{{.designer.code = 0x15, .designer.flag = 1, .designer.cont = 0},
		"NXP"
	},
	{{.designer.code = 0x17, .designer.flag = 1, .designer.cont = 0},
		"TI"
	},
	{{.designer.code = 0x20, .designer.flag = 1, .designer.cont = 0},
		"STMicroelectronics"
	},
};
uint32_t num_designers = sizeof(designers)/sizeof(designer_t);

/* table of components */
const component_data_t component_data[] =
{
	{{.codes.designer_index=0, .codes.partnum=0x925}, "ETM-M4"},
	{{.codes.designer_index=0, .codes.partnum=0x924}, "ETM-M3/M4"},
	{{.codes.designer_index=0, .codes.partnum=0x927}, "ETM-M7"},
	{{.codes.designer_index=0, .codes.partnum=0x956}, "ETM-A7"},
	{{.codes.designer_index=0, .codes.partnum=0x962}, "STM"},
	{{.codes.designer_index=0, .codes.partnum=0x950}, "PTM-A9"},
	{{.codes.designer_index=0, .codes.partnum=0x961}, "Trace Memory Controller"},
	{{.codes.designer_index=0, .codes.partnum=0x903}, "CTI"},
	{{.codes.designer_index=0, .codes.partnum=0x950}, "TPIU"},
	{{.codes.designer_index=0, .codes.partnum=0x950}, "PTM-A9"},
	{{.codes.designer_index=0, .codes.partnum=0x002}, "DWT"},
	{{.codes.designer_index=0, .codes.partnum=0x006}, "DWT"},
	{{.codes.designer_index=0, .codes.partnum=0x00A}, "DWT"},
	{{.codes.designer_index=0, .codes.partnum=0x003}, "FPB"},
	{{.codes.designer_index=0, .codes.partnum=0x007}, "FPB"},
	{{.codes.designer_index=0, .codes.partnum=0x00E}, "FPB"},
	{{.codes.designer_index=0, .codes.partnum=0xD20}, "DWT/FPB/ETM"},
	{{.codes.designer_index=0, .codes.partnum=0xD21}, "DWT/FPB"},
	{{.codes.designer_index=0, .codes.partnum=0xD30}, "DWT/FPB"},
	{{.codes.designer_index=0, .codes.partnum=0x00C}, "SCS"},
	{{.codes.designer_index=0, .codes.partnum=0x001}, "ITM"},
	{{.codes.designer_index=0, .codes.partnum=0x9A1}, "TPIU (V7-M)"},
	{{.codes.designer_index=4, .codes.partnum=0x411}, "ROM TABLE"},
};
uint32_t num_components = sizeof(component_data)/sizeof(component_data_t);

/* CODE */


void handle_component_class(uint8_t cclass)
{
	lastbuff += sprintf(&buffer[lastbuff], "component class:");
	switch (cclass)
	{
	case 1:
		lastbuff += sprintf(&buffer[lastbuff], "Generic verification component");
		break;
	case 9:
		lastbuff += sprintf(&buffer[lastbuff], "Debug component");
		break;
	case 11:
		lastbuff += sprintf(&buffer[lastbuff], "Peripheral Test Block (PTB)");
		break;
	case 13:
		lastbuff += sprintf(&buffer[lastbuff], "OptimoDE Data Engine SubSystem (DESS) component");
		break;
	case 14:
		lastbuff += sprintf(&buffer[lastbuff], "Generic IP component");
		break;
	case 15:
		lastbuff += sprintf(&buffer[lastbuff], "PrimeCell peripheral");
		break;
	default:
		lastbuff += sprintf(&buffer[lastbuff], "RESERVED");
		break;
	}
	lastbuff += sprintf(&buffer[lastbuff], " (%d)\n", (uint32_t)cclass);
}

void handle_part(uint8_t jep_id, uint8_t jepflag, uint8_t jep_cont, uint16_t partnum)
{
	designer_t designer;
	comp_id_t comp_id;
	uint16_t i, des_ind = 0xFFFF;

	lastbuff += sprintf(&buffer[lastbuff], "JEDEC JEP 106 ID: %d, cont flag %d, continuation code %d\n",
		(int)jep_id, (int)jepflag, (int)jep_cont);

	lastbuff += sprintf(&buffer[lastbuff], "Part number %d (0x%04X)\n", (int)partnum);
	lastbuff += sprintf(&buffer[lastbuff], "\t");
	// Initialize as legacy
	designer.code.designer.code = jep_id;
	designer.code.designer.flag = 0;
	designer.code.designer.cont = 0;

	if (jepflag) // not legacy
	{
		designer.code.designer.flag = jepflag;
		designer.code.designer.cont = jep_cont;
	}
	for (i=0; i<num_designers; i++)
	{
		if (designer.code.code == designers[i].code.code)
		{
			lastbuff += sprintf(&buffer[lastbuff], "%s ", designers[i].name);
			des_ind = i;
		}
	}
	if (des_ind != 0xFFFF)
	{
		comp_id.codes.designer_index = des_ind;
		comp_id.codes.partnum = partnum;

		for (i=0; i<num_components; i++)
		{
			if (component_data[i].id.key == comp_id.key)
			{
				lastbuff += sprintf(&buffer[lastbuff], " %s", component_data[i].descr);
				break;
			}
		}
		if (i >= num_components)
		{
			lastbuff += sprintf(&buffer[lastbuff], "Unknown part");
		}
	} else {
		lastbuff += sprintf(&buffer[lastbuff], "Unknown designer");
	}
	lastbuff += sprintf(&buffer[lastbuff], "\n");
}

void handle_component_generic(uint32_t *ptr)
{
	uint32_t *ptr1;
	uint32_t tmp1, tmp2, csize, caddress;
	uint32_t cid = 0;
	uint32_t pid0, pid1;
	uint16_t partnum;
	uint8_t jepid, jepcont, jepflag, revand, custmod, rev, cclass, i;

	tmp1 = (uint32_t)ptr;
	tmp2 = tmp1 + 0xFD0; // start of IDs
	ptr1 = (uint32_t *) tmp2;
	pid0 = 0;
	pid1 = 0;
	// get PID high
	for (i=0; i<4; i++) {
		// only LSB of a PID-register is used
		pid1 |= (((*ptr1) & 0xFF) <<8*i);
		ptr1++;
	}
	// get PID low
	for (i=0; i<4; i++) {
		// only LSB of a PID-register is used
		pid0 |= (((*ptr1) & 0xFF) <<8*i);
		ptr1++;
	}
	// get CID
	for (i=0; i<4; i++) {
		// only LSB of a CID-register is used
		cid |= (((*ptr1) & 0xFF) <<8*i);
		ptr1++;
	}

	cclass = (cid >> 12) & 0x0F;

	// 3 upper bytes of pid high are zeroes
	jepcont = (uint8_t)(pid1 & 0x0000000F);
	tmp2 = (pid1 & 0x000000F0) >> 4;
	csize = 4*(tmp2 + 1); // in kbytes
	caddress = tmp1 - (4096 * tmp2);

	// pid low
	revand = (pid0 >> 28) & 0x0F;
	custmod = (pid0 >> 24) & 0x0F;
	rev = (pid0 >> 20) & 0x0F;
	jepflag = (pid0 >> 19) & 0x01;
	jepid = (pid0 >> 12) & 0x7F;
	partnum = pid0 & 0xFFF;

	// check preamble
	if ((cid & 0xFFFF0FFF) != 0xB105000D) {
		lastbuff += sprintf(&buffer[lastbuff], "Bad CID: 0x%08X\n", cid);
	}

	handle_component_class(cclass);
	handle_part(jepid, jepflag, jepcont, partnum);

	lastbuff += sprintf(&buffer[lastbuff], "start address 0x%08X, size %d\n", caddress, csize);
	lastbuff += sprintf(&buffer[lastbuff], "RevAnd %d\n", (uint32_t)revand);
	lastbuff += sprintf(&buffer[lastbuff], "Custmod %d\n", (uint32_t)custmod);
	lastbuff += sprintf(&buffer[lastbuff], "Revision %d\n", (uint32_t)rev);
}

void handle_rom_table(uint32_t address) {
	uint32_t *ptr1, ptr2;
	uint32_t tmp, tmp2;
	int i;

	if (address == 0) return;

	ptr1 = (uint32_t *) address;
	handle_component_generic(ptr1);

	/* go through the component entries */
	for (i=0; *ptr1 != 0; i++, ptr1++) {
		tmp = *ptr1; // the entry
		if (tmp == 0) {
			break; // null entry is end marker
		}
		tmp2 = tmp & 0xFFFFF000; // component offset
		ptr2 = (uint32_t *)(tmp2 + address); // pointer to component
		lastbuff += sprintf(&buffer[lastbuff], "Component address 0x%08X\n", (unsigned int)ptr2);
		if (!(tmp & 1)) {
			lastbuff += sprintf(&buffer[lastbuff], "Component %d not present!\n", i);
		} else {
			handle_component_generic(ptr2);
		}
	}
}

// ARMv7-M: ROM table address = 0xE00FF000, CPACR = 0xE000ED88
void read_resources(uint32_t address, uint32_t coprocs)
{
	uint32_t tmp;
	uint32_t *cpacr = (uint32_t *)coprocs;
	int i;

	// check coprocessors
	if (coprocs != 0)
	{
		*cpacr = 0xFFFFFFFF;
		tmp = *cpacr;
		for (i=0; i<15; i++) {
			if ((tmp & (3 << (i*2))) != 0) {
				lastbuff += sprintf(&buffer[lastbuff], "CP%i exists\n", i);
			}
		}
	}
	// we know this is the (master) ROM table
	if (address != 0) {
		handle_rom_table(address);
	}
}
