/*
 * Copyright (c) 2002 Vladimir Popov <jumbo@narod.ru>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * $Id$
 * PCI autoconfiguration code
 */

#ifndef PCI_H__
#define PCI_H__

#define CONFIG_ADDRESS			0x0CF8
#define CONFIG_DATA			0x0CFC

#define PCI_CYCLE_ENABLE_BIT		(1 << 31)

#define PCI_BUS_NO(x)			(((x) & 0xFF) << 16)
#define PCI_DEV_NO(x)			(((x) & 0x1F) << 11)
#define PCI_FUN_NO(x)			(((x) & 7) << 8)
#define PCI_REG_ADDR(x)			((((x) >> 2) & 0x3F) << 2)

#if 0
	/*
	 * PCI_MAX_BUS == 0xFF requires too much time to look up a card.
	 * Majority of the cards are on buses 0, 1 and 2, so
	 * reduce the search time by setting PCI_MAX_BUS = 0xF.
	 */
#define PCI_MAX_BUS			0xFF
#else
#define PCI_MAX_BUS			0x0F
#endif /* 0 */
#define PCI_MAX_DEV			0x1F
#define PCI_MAX_FUN			0x07

#define PCI_VENDOR_SHIFT		0
#define PCI_VENDOR_MASK			0xffff
#define PCI_VENDOR(id) (((id) >> PCI_VENDOR_SHIFT) & PCI_VENDOR_MASK)

#define PCI_PRODUCT_SHIFT		16
#define PCI_PRODUCT_MASK		0xffff
#define PCI_PRODUCT(id) (((id) >> PCI_PRODUCT_SHIFT) & PCI_PRODUCT_MASK)

#define PCI_CLASS_SHIFT			24
#define PCI_CLASS_MASK			0xff
#define PCI_CLASS(cr) (((cr) >> PCI_CLASS_SHIFT) & PCI_CLASS_MASK)

/* We're interested only in these */
#define PCI_CLASS_MULTIMEDIA		0x04

#define PCI_SUBCLASS_SHIFT		16
#define PCI_SUBCLASS_MASK		0xff
#define PCI_SUBCLASS(cr) (((cr) >> PCI_SUBCLASS_SHIFT) & PCI_SUBCLASS_MASK)

#define PCI_INTERFACE_SHIFT		8
#define PCI_INTERFACE_MASK		0xff
#define PCI_INTERFACE(cr) (((cr) >> PCI_INTERFACE_SHIFT) & PCI_INTERFACE_MASK)

#define PCI_REVISION_SHIFT		0
#define PCI_REVISION_MASK		0xff
#define PCI_REVISION(cr) (((cr) >> PCI_REVISION_SHIFT) & PCI_REVISION_MASK)

#define PCI_BASEADDR_IO_TYPE		(1 << 0)
#define PCI_BASEADDR_MEM_TYPE		(0 << 0)
#define PCI_BASEADDR(x)			((x) & ~0x03)

#define	PCI_ID_REG			0x00
#define	PCI_COMMAND_STATUS_REG		0x04
#define	PCI_CLASS_REG			0x08
#define	PCI_BHLC_REG			0x0c
#define PCI_BASEADDR_0			0x10
#define PCI_BASEADDR_1			0x14
#define PCI_BASEADDR_2			0x18
#define PCI_BASEADDR_3			0x1c
#define PCI_BASEADDR_4			0x20
#define PCI_BASEADDR_5			0x24
#define PCI_CARDBUS_CIS_REG 		0x28
#define PCI_SUBSYSVEND_REG		0x2c
#define PCI_CARDBUSCIS_REG		0x28
#define PCI_CAPLISTPTR_REG		0x34
#define	PCI_INTERRUPT_REG		0x3c

struct pci_entry_t {
	u_int8_t bus;
	u_int8_t dev;
	u_int8_t fun;
};

#endif /* PCI_H__ */
