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
 * $Id: pci.c,v 1.7 2002/01/11 10:32:47 pva Exp $
 * PCI autoconfiguration code
 */

#include "ostypes.h"

#ifdef DEBUG
#include <stdio.h>
#endif

#include "pci.h"
#include "radio_drv.h"

u_int16_t pci_base_addr(struct pci_entry_t *);
u_int32_t pci_read_reg(struct pci_entry_t *, u_int8_t);
int pci_device_match(struct pci_entry_t *, struct pci_dev_t *);

/*
 * PCI specification prior to 2.2 has two configuration modes.
 * The specification 2.2 has only the configuration mode #1.
 * This file uses the configuration mode #1. On boxes where only
 * the configuration mode #2 is used these procedures won't work.
 */

/*
 * Scan all PCI buses, devices and device functions until
 * required device is found. Return base address of a first entry.
 */
u_int16_t
pci_bus_locate(struct pci_dev_t *card) {
	struct pci_entry_t e;
	u_int32_t data;

	for (e.bus = 0; e.bus <= PCI_MAX_BUS; e.bus++) {
		for (e.dev = 0; e.dev <= PCI_MAX_DEV; e.dev++) {
			for (e.fun = 0; e.fun <= PCI_MAX_FUN; e.fun++) {
				if (pci_device_match(&e, card)) {
					data = pci_base_addr(&e);
					/* We don't need mem address */
					if (PCI_BASEADDR_IO_TYPE & data)
						return PCI_BASEADDR(data);
				}
			}
		}
	}

	return 0u;
}

/*
 * Return base address of a PCI device specified in the struct pci_entry_t
 */
u_int16_t
pci_base_addr(struct pci_entry_t *c) {
	return pci_read_reg(c, PCI_BASEADDR_0);
}

/*
 * Read PCI configuration register of device and return its content
 */
u_int32_t
pci_read_reg(struct pci_entry_t *c, u_int8_t reg) {
	u_int32_t data;
	data  = PCI_CYCLE_ENABLE_BIT;
	data |= PCI_BUS_NO(c->bus) | PCI_DEV_NO(c->dev) | PCI_FUN_NO(c->fun);
	data |= PCI_REG_ADDR(reg);
	OUTL(CONFIG_ADDRESS, data);
	return inl(CONFIG_DATA);
}

/*
 * Compare PCI device parameters (vendor id, device id, subsystem id, etc)
 * with those specified in the struct pci_dev_t. vid and did of the struct
 * must be defined.
 *
 * Return TRUE on match, FALSE if else.
 */
int
pci_device_match(struct pci_entry_t *e, struct pci_dev_t *c) {
	u_int32_t data;

	/* These must be defined */
	data = pci_read_reg(e, PCI_ID_REG);
#ifdef DEBUG
	if (data != 0xFFFFFFFF) {
		printf("bus %u, dev %u, fun %u:", e->bus, e->dev, e->fun);
		printf("\tvendor id 0x%x, product id 0x%x\n", PCI_VENDOR(data), PCI_PRODUCT(data));
	}
#endif /* DEBUG */

	if (PCI_PRODUCT(data) != c->did)
		return 0;
	if (PCI_VENDOR(data) != c->vid)
		return 0;

#ifdef DEBUG
	printf("bus %u, dev %u, fun %u:", e->bus, e->dev, e->fun);
	printf("\tvendor id 0x%x, product id 0x%x\n", PCI_VENDOR(data), PCI_PRODUCT(data));
	data = pci_read_reg(e, PCI_CLASS_REG);
	printf("\tclass multimedia, subclass 0x%x, revision 0x%x\n",
			PCI_SUBCLASS(data), PCI_REVISION(data));
	data = pci_read_reg(e, PCI_SUBSYSVEND_REG);
	printf("\tsubsystem vendor id 0x%x, subsystem id 0x%x\n", PCI_VENDOR(data), PCI_PRODUCT(data));
#endif /* DEBUG */

	data = pci_read_reg(e, PCI_CLASS_REG);
	if (PCI_CLASS(data) != PCI_CLASS_MULTIMEDIA)
		return 0;
	if (c->subclass != PCI_SUBCLASS_ANY)
		if (c->subclass != PCI_SUBCLASS(data))
			return 0;
	if (c->rev != PCI_REVISION_ANY)
		if (c->rev != PCI_REVISION(data))
			return 0;

	data = pci_read_reg(e, PCI_SUBSYSVEND_REG);
	if (c->subvid != PCI_SUBSYS_ID_ANY)
		if (c->subvid != PCI_VENDOR(data))
			return 0;
	if (c->subdid != PCI_SUBSYS_ID_ANY)
		if (c->subdid != PCI_PRODUCT(data))
			return 0;

	return 1;
}
