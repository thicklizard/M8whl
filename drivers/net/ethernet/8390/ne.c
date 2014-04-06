/*
    Written 1992-94 by Donald Becker.

    Copyright 1993 United States Government as represented by the
    Director, National Security Agency.

    This software may be used and distributed according to the terms
    of the GNU General Public License, incorporated herein by reference.

    The author may be reached as becker@scyld.com, or C/O
    Scyld Computing Corporation, 410 Severn Ave., Suite 210, Annapolis MD 21403

    This driver should work with many programmed-I/O 8390-based ethernet
    boards.  Currently it supports the NE1000, NE2000, many clones,
    and some Cabletron products.

    Changelog:

    Paul Gortmaker	: use ENISR_RDC to monitor Tx PIO uploads, made
			  sanity checks and bad clone support optional.
    Paul Gortmaker	: new reset code, reset card after probe at boot.
    Paul Gortmaker	: multiple card support for module users.
    Paul Gortmaker	: Support for PCI ne2k clones, similar to lance.c
    Paul Gortmaker	: Allow users with bad cards to avoid full probe.
    Paul Gortmaker	: PCI probe changes, more PCI cards supported.
    rjohnson@analogic.com : Changed init order so an interrupt will only
    occur after memory is allocated for dev->priv. Deallocated memory
    last in cleanup_modue()
    Richard Guenther    : Added support for ISAPnP cards
    Paul Gortmaker	: Discontinued PCI support - use ne2k-pci.c instead.
    Hayato Fujiwara	: Add m32r support.

*/


static const char version1[] =
"ne.c:v1.10 9/23/94 Donald Becker (becker@scyld.com)\n";
static const char version2[] =
"Last modified Nov 1, 2000 by Paul Gortmaker\n";


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/isapnp.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>

#include <asm/io.h>

#include "8390.h"

#define DRV_NAME "ne"


#define SUPPORT_NE_BAD_CLONES
#define BAD 0xbad

#define MAX_NE_CARDS	4	
static struct platform_device *pdev_ne[MAX_NE_CARDS];
static int io[MAX_NE_CARDS];
static int irq[MAX_NE_CARDS];
static int bad[MAX_NE_CARDS];

#ifdef MODULE
module_param_array(io, int, NULL, 0);
module_param_array(irq, int, NULL, 0);
module_param_array(bad, int, NULL, 0);
MODULE_PARM_DESC(io, "I/O base address(es),required");
MODULE_PARM_DESC(irq, "IRQ number(s)");
MODULE_PARM_DESC(bad, "Accept card(s) with bad signatures");
MODULE_DESCRIPTION("NE1000/NE2000 ISA/PnP Ethernet driver");
MODULE_LICENSE("GPL");
#endif 




#if !defined(MODULE) && (defined(CONFIG_ISA) || defined(CONFIG_M32R))
#define NEEDS_PORTLIST
#endif

#ifdef NEEDS_PORTLIST
static unsigned int netcard_portlist[] __initdata = {
	0x300, 0x280, 0x320, 0x340, 0x360, 0x380, 0
};
#endif

static struct isapnp_device_id isapnp_clone_list[] __initdata = {
	{	ISAPNP_CARD_ID('A','X','E',0x2011),
		ISAPNP_VENDOR('A','X','E'), ISAPNP_FUNCTION(0x2011),
		(long) "NetGear EA201" },
	{	ISAPNP_ANY_ID, ISAPNP_ANY_ID,
		ISAPNP_VENDOR('E','D','I'), ISAPNP_FUNCTION(0x0216),
		(long) "NN NE2000" },
	{	ISAPNP_ANY_ID, ISAPNP_ANY_ID,
		ISAPNP_VENDOR('P','N','P'), ISAPNP_FUNCTION(0x80d6),
		(long) "Generic PNP" },
	{ }	
};

MODULE_DEVICE_TABLE(isapnp, isapnp_clone_list);

#ifdef SUPPORT_NE_BAD_CLONES
static struct { const char *name8, *name16; unsigned char SAprefix[4];}
bad_clone_list[] __initdata = {
    {"DE100", "DE200", {0x00, 0xDE, 0x01,}},
    {"DE120", "DE220", {0x00, 0x80, 0xc8,}},
    {"DFI1000", "DFI2000", {'D', 'F', 'I',}}, 
    {"EtherNext UTP8", "EtherNext UTP16", {0x00, 0x00, 0x79}},
    {"NE1000","NE2000-invalid", {0x00, 0x00, 0xd8}}, 
    {"NN1000", "NN2000",  {0x08, 0x03, 0x08}}, 
    {"4-DIM8","4-DIM16", {0x00,0x00,0x4d,}},  
    {"Con-Intl_8", "Con-Intl_16", {0x00, 0x00, 0x24}}, 
    {"ET-100","ET-200", {0x00, 0x45, 0x54}}, 
    {"COMPEX","COMPEX16",{0x00,0x80,0x48}}, 
    {"E-LAN100", "E-LAN200", {0x00, 0x00, 0x5d}}, 
    {"PCM-4823", "PCM-4823", {0x00, 0xc0, 0x6c}}, 
    {"REALTEK", "RTL8019", {0x00, 0x00, 0xe8}}, 
#ifdef CONFIG_MACH_TX49XX
    {"RBHMA4X00-RTL8019", "RBHMA4X00-RTL8019", {0x00, 0x60, 0x0a}},  
#endif
    {"LCS-8834", "LCS-8836", {0x04, 0x04, 0x37}}, 
    {NULL,}
};
#endif


#define NE_BASE	 (dev->base_addr)
#define NE_CMD	 	0x00
#define NE_DATAPORT	0x10	
#define NE_RESET	0x1f	
#define NE_IO_EXTENT	0x20

#define NE1SM_START_PG	0x20	
#define NE1SM_STOP_PG 	0x40	
#define NESM_START_PG	0x40	
#define NESM_STOP_PG	0x80	

#if defined(CONFIG_PLAT_MAPPI)
#  define DCR_VAL 0x4b
#elif defined(CONFIG_PLAT_OAKS32R)  || \
   defined(CONFIG_MACH_TX49XX)
#  define DCR_VAL 0x48		
#else
#  define DCR_VAL 0x49
#endif

static int ne_probe1(struct net_device *dev, unsigned long ioaddr);
static int ne_probe_isapnp(struct net_device *dev);

static void ne_reset_8390(struct net_device *dev);
static void ne_get_8390_hdr(struct net_device *dev, struct e8390_pkt_hdr *hdr,
			  int ring_page);
static void ne_block_input(struct net_device *dev, int count,
			  struct sk_buff *skb, int ring_offset);
static void ne_block_output(struct net_device *dev, const int count,
		const unsigned char *buf, const int start_page);



static int __init do_ne_probe(struct net_device *dev)
{
	unsigned long base_addr = dev->base_addr;
#ifdef NEEDS_PORTLIST
	int orig_irq = dev->irq;
#endif

	
	if (base_addr > 0x1ff) {	
		int ret = ne_probe1(dev, base_addr);
		if (ret)
			printk(KERN_WARNING "ne.c: No NE*000 card found at "
				"i/o = %#lx\n", base_addr);
		return ret;
	}
	else if (base_addr != 0)	
		return -ENXIO;

	
	if (isapnp_present() && (ne_probe_isapnp(dev) == 0))
		return 0;

#ifdef NEEDS_PORTLIST
	
	for (base_addr = 0; netcard_portlist[base_addr] != 0; base_addr++) {
		int ioaddr = netcard_portlist[base_addr];
		dev->irq = orig_irq;
		if (ne_probe1(dev, ioaddr) == 0)
			return 0;
	}
#endif

	return -ENODEV;
}

static int __init ne_probe_isapnp(struct net_device *dev)
{
	int i;

	for (i = 0; isapnp_clone_list[i].vendor != 0; i++) {
		struct pnp_dev *idev = NULL;

		while ((idev = pnp_find_dev(NULL,
					    isapnp_clone_list[i].vendor,
					    isapnp_clone_list[i].function,
					    idev))) {
			
			if (pnp_device_attach(idev) < 0)
				continue;
			if (pnp_activate_dev(idev) < 0) {
			      	pnp_device_detach(idev);
			      	continue;
			}
			
			if (!pnp_port_valid(idev, 0) || !pnp_irq_valid(idev, 0)) {
				pnp_device_detach(idev);
				continue;
			}
			
			dev->base_addr = pnp_port_start(idev, 0);
			dev->irq = pnp_irq(idev, 0);
			printk(KERN_INFO "ne.c: ISAPnP reports %s at i/o %#lx, irq %d.\n",
				(char *) isapnp_clone_list[i].driver_data,
				dev->base_addr, dev->irq);
			if (ne_probe1(dev, dev->base_addr) != 0) {	
				printk(KERN_ERR "ne.c: Probe of ISAPnP card at %#lx failed.\n", dev->base_addr);
				pnp_device_detach(idev);
				return -ENXIO;
			}
			ei_status.priv = (unsigned long)idev;
			break;
		}
		if (!idev)
			continue;
		return 0;
	}

	return -ENODEV;
}

static int __init ne_probe1(struct net_device *dev, unsigned long ioaddr)
{
	int i;
	unsigned char SA_prom[32];
	int wordlength = 2;
	const char *name = NULL;
	int start_page, stop_page;
	int neX000, ctron, copam, bad_card;
	int reg0, ret;
	static unsigned version_printed;

	if (!request_region(ioaddr, NE_IO_EXTENT, DRV_NAME))
		return -EBUSY;

	reg0 = inb_p(ioaddr);
	if (reg0 == 0xFF) {
		ret = -ENODEV;
		goto err_out;
	}

	
	{
		int regd;
		outb_p(E8390_NODMA+E8390_PAGE1+E8390_STOP, ioaddr + E8390_CMD);
		regd = inb_p(ioaddr + 0x0d);
		outb_p(0xff, ioaddr + 0x0d);
		outb_p(E8390_NODMA+E8390_PAGE0, ioaddr + E8390_CMD);
		inb_p(ioaddr + EN0_COUNTER0); 
		if (inb_p(ioaddr + EN0_COUNTER0) != 0) {
			outb_p(reg0, ioaddr);
			outb_p(regd, ioaddr + 0x0d);	
			ret = -ENODEV;
			goto err_out;
		}
	}

	if (ei_debug  &&  version_printed++ == 0)
		printk(KERN_INFO "%s%s", version1, version2);

	printk(KERN_INFO "NE*000 ethercard probe at %#3lx:", ioaddr);


	bad_card = ((dev->base_addr != 0) && (dev->mem_end == BAD));

	

	{
		unsigned long reset_start_time = jiffies;

		
		outb(inb(ioaddr + NE_RESET), ioaddr + NE_RESET);

		while ((inb_p(ioaddr + EN0_ISR) & ENISR_RESET) == 0)
		if (time_after(jiffies, reset_start_time + 2*HZ/100)) {
			if (bad_card) {
				printk(" (warning: no reset ack)");
				break;
			} else {
				printk(" not found (no reset ack).\n");
				ret = -ENODEV;
				goto err_out;
			}
		}

		outb_p(0xff, ioaddr + EN0_ISR);		
	}

	{
		struct {unsigned char value, offset; } program_seq[] =
		{
			{E8390_NODMA+E8390_PAGE0+E8390_STOP, E8390_CMD}, 
			{0x48,	EN0_DCFG},	
			{0x00,	EN0_RCNTLO},	
			{0x00,	EN0_RCNTHI},
			{0x00,	EN0_IMR},	
			{0xFF,	EN0_ISR},
			{E8390_RXOFF, EN0_RXCR},	
			{E8390_TXOFF, EN0_TXCR},	
			{32,	EN0_RCNTLO},
			{0x00,	EN0_RCNTHI},
			{0x00,	EN0_RSARLO},	
			{0x00,	EN0_RSARHI},
			{E8390_RREAD+E8390_START, E8390_CMD},
		};

		for (i = 0; i < ARRAY_SIZE(program_seq); i++)
			outb_p(program_seq[i].value, ioaddr + program_seq[i].offset);

	}
	for(i = 0; i < 32 ; i+=2) {
		SA_prom[i] = inb(ioaddr + NE_DATAPORT);
		SA_prom[i+1] = inb(ioaddr + NE_DATAPORT);
		if (SA_prom[i] != SA_prom[i+1])
			wordlength = 1;
	}

	if (wordlength == 2)
	{
		for (i = 0; i < 16; i++)
			SA_prom[i] = SA_prom[i+i];
		
		outb_p(DCR_VAL, ioaddr + EN0_DCFG);
		start_page = NESM_START_PG;

		if ((DCR_VAL & 0x01) == 0 &&		
		    inb(ioaddr + EN0_RCNTLO) == 0x50 &&
		    inb(ioaddr + EN0_RCNTHI) == 0x70)
			stop_page = 0x60;
		else
			stop_page = NESM_STOP_PG;
	} else {
		start_page = NE1SM_START_PG;
		stop_page  = NE1SM_STOP_PG;
	}

#if  defined(CONFIG_PLAT_MAPPI) || defined(CONFIG_PLAT_OAKS32R)
	neX000 = ((SA_prom[14] == 0x57  &&  SA_prom[15] == 0x57)
		|| (SA_prom[14] == 0x42 && SA_prom[15] == 0x42));
#else
	neX000 = (SA_prom[14] == 0x57  &&  SA_prom[15] == 0x57);
#endif
	ctron =  (SA_prom[0] == 0x00 && SA_prom[1] == 0x00 && SA_prom[2] == 0x1d);
	copam =  (SA_prom[14] == 0x49 && SA_prom[15] == 0x00);

	
	if (neX000 || bad_card || copam) {
		name = (wordlength == 2) ? "NE2000" : "NE1000";
	}
	else if (ctron)
	{
		name = (wordlength == 2) ? "Ctron-8" : "Ctron-16";
		start_page = 0x01;
		stop_page = (wordlength == 2) ? 0x40 : 0x20;
	}
	else
	{
#ifdef SUPPORT_NE_BAD_CLONES
		for (i = 0; bad_clone_list[i].name8; i++)
		{
			if (SA_prom[0] == bad_clone_list[i].SAprefix[0] &&
				SA_prom[1] == bad_clone_list[i].SAprefix[1] &&
				SA_prom[2] == bad_clone_list[i].SAprefix[2])
			{
				if (wordlength == 2)
				{
					name = bad_clone_list[i].name16;
				} else {
					name = bad_clone_list[i].name8;
				}
				break;
			}
		}
		if (bad_clone_list[i].name8 == NULL)
		{
			printk(" not found (invalid signature %2.2x %2.2x).\n",
				SA_prom[14], SA_prom[15]);
			ret = -ENXIO;
			goto err_out;
		}
#else
		printk(" not found.\n");
		ret = -ENXIO;
		goto err_out;
#endif
	}

	if (dev->irq < 2)
	{
		unsigned long cookie = probe_irq_on();
		outb_p(0x50, ioaddr + EN0_IMR);	
		outb_p(0x00, ioaddr + EN0_RCNTLO);
		outb_p(0x00, ioaddr + EN0_RCNTHI);
		outb_p(E8390_RREAD+E8390_START, ioaddr); 
		mdelay(10);		
		outb_p(0x00, ioaddr + EN0_IMR); 		
		dev->irq = probe_irq_off(cookie);
		if (ei_debug > 2)
			printk(" autoirq is %d\n", dev->irq);
	} else if (dev->irq == 2)
		dev->irq = 9;

	if (! dev->irq) {
		printk(" failed to detect IRQ line.\n");
		ret = -EAGAIN;
		goto err_out;
	}

	ret = request_irq(dev->irq, eip_interrupt, 0, name, dev);
	if (ret) {
		printk (" unable to get IRQ %d (errno=%d).\n", dev->irq, ret);
		goto err_out;
	}

	dev->base_addr = ioaddr;

#ifdef CONFIG_PLAT_MAPPI
	outb_p(E8390_NODMA + E8390_PAGE1 + E8390_STOP,
		ioaddr + E8390_CMD); 
	for (i = 0; i < ETH_ALEN; i++) {
		dev->dev_addr[i] = SA_prom[i]
			= inb_p(ioaddr + EN1_PHYS_SHIFT(i));
	}
#else
	for (i = 0; i < ETH_ALEN; i++) {
		dev->dev_addr[i] = SA_prom[i];
	}
#endif

	printk("%pM\n", dev->dev_addr);

	ei_status.name = name;
	ei_status.tx_start_page = start_page;
	ei_status.stop_page = stop_page;

	
	ei_status.word16 = (wordlength == 2 && (DCR_VAL & 0x01));

	ei_status.rx_start_page = start_page + TX_PAGES;
#ifdef PACKETBUF_MEMSIZE
	 
	ei_status.stop_page = ei_status.tx_start_page + PACKETBUF_MEMSIZE;
#endif

	ei_status.reset_8390 = &ne_reset_8390;
	ei_status.block_input = &ne_block_input;
	ei_status.block_output = &ne_block_output;
	ei_status.get_8390_hdr = &ne_get_8390_hdr;
	ei_status.priv = 0;

	dev->netdev_ops = &eip_netdev_ops;
	NS8390p_init(dev, 0);

	ret = register_netdev(dev);
	if (ret)
		goto out_irq;
	printk(KERN_INFO "%s: %s found at %#lx, using IRQ %d.\n",
	       dev->name, name, ioaddr, dev->irq);
	return 0;

out_irq:
	free_irq(dev->irq, dev);
err_out:
	release_region(ioaddr, NE_IO_EXTENT);
	return ret;
}


static void ne_reset_8390(struct net_device *dev)
{
	unsigned long reset_start_time = jiffies;

	if (ei_debug > 1)
		printk(KERN_DEBUG "resetting the 8390 t=%ld...", jiffies);

	
	outb(inb(NE_BASE + NE_RESET), NE_BASE + NE_RESET);

	ei_status.txing = 0;
	ei_status.dmaing = 0;

	
	while ((inb_p(NE_BASE+EN0_ISR) & ENISR_RESET) == 0)
		if (time_after(jiffies, reset_start_time + 2*HZ/100)) {
			printk(KERN_WARNING "%s: ne_reset_8390() did not complete.\n", dev->name);
			break;
		}
	outb_p(ENISR_RESET, NE_BASE + EN0_ISR);	
}


static void ne_get_8390_hdr(struct net_device *dev, struct e8390_pkt_hdr *hdr, int ring_page)
{
	int nic_base = dev->base_addr;

	

	if (ei_status.dmaing)
	{
		printk(KERN_EMERG "%s: DMAing conflict in ne_get_8390_hdr "
			"[DMAstat:%d][irqlock:%d].\n",
			dev->name, ei_status.dmaing, ei_status.irqlock);
		return;
	}

	ei_status.dmaing |= 0x01;
	outb_p(E8390_NODMA+E8390_PAGE0+E8390_START, nic_base+ NE_CMD);
	outb_p(sizeof(struct e8390_pkt_hdr), nic_base + EN0_RCNTLO);
	outb_p(0, nic_base + EN0_RCNTHI);
	outb_p(0, nic_base + EN0_RSARLO);		
	outb_p(ring_page, nic_base + EN0_RSARHI);
	outb_p(E8390_RREAD+E8390_START, nic_base + NE_CMD);

	if (ei_status.word16)
		insw(NE_BASE + NE_DATAPORT, hdr, sizeof(struct e8390_pkt_hdr)>>1);
	else
		insb(NE_BASE + NE_DATAPORT, hdr, sizeof(struct e8390_pkt_hdr));

	outb_p(ENISR_RDC, nic_base + EN0_ISR);	
	ei_status.dmaing &= ~0x01;

	le16_to_cpus(&hdr->count);
}


static void ne_block_input(struct net_device *dev, int count, struct sk_buff *skb, int ring_offset)
{
#ifdef NE_SANITY_CHECK
	int xfer_count = count;
#endif
	int nic_base = dev->base_addr;
	char *buf = skb->data;

	
	if (ei_status.dmaing)
	{
		printk(KERN_EMERG "%s: DMAing conflict in ne_block_input "
			"[DMAstat:%d][irqlock:%d].\n",
			dev->name, ei_status.dmaing, ei_status.irqlock);
		return;
	}
	ei_status.dmaing |= 0x01;
	outb_p(E8390_NODMA+E8390_PAGE0+E8390_START, nic_base+ NE_CMD);
	outb_p(count & 0xff, nic_base + EN0_RCNTLO);
	outb_p(count >> 8, nic_base + EN0_RCNTHI);
	outb_p(ring_offset & 0xff, nic_base + EN0_RSARLO);
	outb_p(ring_offset >> 8, nic_base + EN0_RSARHI);
	outb_p(E8390_RREAD+E8390_START, nic_base + NE_CMD);
	if (ei_status.word16)
	{
		insw(NE_BASE + NE_DATAPORT,buf,count>>1);
		if (count & 0x01)
		{
			buf[count-1] = inb(NE_BASE + NE_DATAPORT);
#ifdef NE_SANITY_CHECK
			xfer_count++;
#endif
		}
	} else {
		insb(NE_BASE + NE_DATAPORT, buf, count);
	}

#ifdef NE_SANITY_CHECK

	if (ei_debug > 1)
	{
		
		int addr, tries = 20;
		do {
			int high = inb_p(nic_base + EN0_RSARHI);
			int low = inb_p(nic_base + EN0_RSARLO);
			addr = (high << 8) + low;
			if (((ring_offset + xfer_count) & 0xff) == low)
				break;
		} while (--tries > 0);
	 	if (tries <= 0)
			printk(KERN_WARNING "%s: RX transfer address mismatch,"
				"%#4.4x (expected) vs. %#4.4x (actual).\n",
				dev->name, ring_offset + xfer_count, addr);
	}
#endif
	outb_p(ENISR_RDC, nic_base + EN0_ISR);	
	ei_status.dmaing &= ~0x01;
}

static void ne_block_output(struct net_device *dev, int count,
		const unsigned char *buf, const int start_page)
{
	int nic_base = NE_BASE;
	unsigned long dma_start;
#ifdef NE_SANITY_CHECK
	int retries = 0;
#endif


	if (ei_status.word16 && (count & 0x01))
		count++;

	
	if (ei_status.dmaing)
	{
		printk(KERN_EMERG "%s: DMAing conflict in ne_block_output."
			"[DMAstat:%d][irqlock:%d]\n",
			dev->name, ei_status.dmaing, ei_status.irqlock);
		return;
	}
	ei_status.dmaing |= 0x01;
	
	outb_p(E8390_PAGE0+E8390_START+E8390_NODMA, nic_base + NE_CMD);

#ifdef NE_SANITY_CHECK
retry:
#endif

#ifdef NE8390_RW_BUGFIX

	outb_p(0x42, nic_base + EN0_RCNTLO);
	outb_p(0x00,   nic_base + EN0_RCNTHI);
	outb_p(0x42, nic_base + EN0_RSARLO);
	outb_p(0x00, nic_base + EN0_RSARHI);
	outb_p(E8390_RREAD+E8390_START, nic_base + NE_CMD);
	
	udelay(6);
#endif

	outb_p(ENISR_RDC, nic_base + EN0_ISR);

	
	outb_p(count & 0xff, nic_base + EN0_RCNTLO);
	outb_p(count >> 8,   nic_base + EN0_RCNTHI);
	outb_p(0x00, nic_base + EN0_RSARLO);
	outb_p(start_page, nic_base + EN0_RSARHI);

	outb_p(E8390_RWRITE+E8390_START, nic_base + NE_CMD);
	if (ei_status.word16) {
		outsw(NE_BASE + NE_DATAPORT, buf, count>>1);
	} else {
		outsb(NE_BASE + NE_DATAPORT, buf, count);
	}

	dma_start = jiffies;

#ifdef NE_SANITY_CHECK

	if (ei_debug > 1)
	{
		
		int addr, tries = 20;
		do {
			int high = inb_p(nic_base + EN0_RSARHI);
			int low = inb_p(nic_base + EN0_RSARLO);
			addr = (high << 8) + low;
			if ((start_page << 8) + count == addr)
				break;
		} while (--tries > 0);

		if (tries <= 0)
		{
			printk(KERN_WARNING "%s: Tx packet transfer address mismatch,"
				"%#4.4x (expected) vs. %#4.4x (actual).\n",
				dev->name, (start_page << 8) + count, addr);
			if (retries++ == 0)
				goto retry;
		}
	}
#endif

	while ((inb_p(nic_base + EN0_ISR) & ENISR_RDC) == 0)
		if (time_after(jiffies, dma_start + 2*HZ/100)) {		
			printk(KERN_WARNING "%s: timeout waiting for Tx RDC.\n", dev->name);
			ne_reset_8390(dev);
			NS8390p_init(dev, 1);
			break;
		}

	outb_p(ENISR_RDC, nic_base + EN0_ISR);	
	ei_status.dmaing &= ~0x01;
}

static int __init ne_drv_probe(struct platform_device *pdev)
{
	struct net_device *dev;
	int err, this_dev = pdev->id;
	struct resource *res;

	dev = alloc_eip_netdev();
	if (!dev)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_IO, 0);
	if (res) {
		dev->base_addr = res->start;
		dev->irq = platform_get_irq(pdev, 0);
	} else {
		if (this_dev < 0 || this_dev >= MAX_NE_CARDS) {
			free_netdev(dev);
			return -EINVAL;
		}
		dev->base_addr = io[this_dev];
		dev->irq = irq[this_dev];
		dev->mem_end = bad[this_dev];
	}
	err = do_ne_probe(dev);
	if (err) {
		free_netdev(dev);
		return err;
	}
	platform_set_drvdata(pdev, dev);

	if (!res) {
		io[this_dev] = dev->base_addr;
		irq[this_dev] = dev->irq;
	}
	return 0;
}

static int ne_drv_remove(struct platform_device *pdev)
{
	struct net_device *dev = platform_get_drvdata(pdev);

	if (dev) {
		struct pnp_dev *idev = (struct pnp_dev *)ei_status.priv;
		netif_device_detach(dev);
		unregister_netdev(dev);
		if (idev)
			pnp_device_detach(idev);
		ei_status.priv = 0;
		free_irq(dev->irq, dev);
		release_region(dev->base_addr, NE_IO_EXTENT);
		free_netdev(dev);
		platform_set_drvdata(pdev, NULL);
	}
	return 0;
}

static void ne_loop_rm_unreg(int all)
{
	int this_dev;
	struct platform_device *pdev;
	for (this_dev = 0; this_dev < MAX_NE_CARDS; this_dev++) {
		pdev = pdev_ne[this_dev];
		
		if (pdev && (!platform_get_drvdata(pdev) || all)) {
			ne_drv_remove(pdev);
			platform_device_unregister(pdev);
			pdev_ne[this_dev] = NULL;
		}
	}
}

#ifdef CONFIG_PM
static int ne_drv_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct net_device *dev = platform_get_drvdata(pdev);

	if (netif_running(dev)) {
		struct pnp_dev *idev = (struct pnp_dev *)ei_status.priv;
		netif_device_detach(dev);
		if (idev)
			pnp_stop_dev(idev);
	}
	return 0;
}

static int ne_drv_resume(struct platform_device *pdev)
{
	struct net_device *dev = platform_get_drvdata(pdev);

	if (netif_running(dev)) {
		struct pnp_dev *idev = (struct pnp_dev *)ei_status.priv;
		if (idev)
			pnp_start_dev(idev);
		ne_reset_8390(dev);
		NS8390p_init(dev, 1);
		netif_device_attach(dev);
	}
	return 0;
}
#else
#define ne_drv_suspend NULL
#define ne_drv_resume NULL
#endif

static struct platform_driver ne_driver = {
	.remove		= ne_drv_remove,
	.suspend	= ne_drv_suspend,
	.resume		= ne_drv_resume,
	.driver		= {
		.name	= DRV_NAME,
		.owner	= THIS_MODULE,
	},
};

static void __init ne_add_devices(void)
{
	int this_dev;
	struct platform_device *pdev;

	for (this_dev = 0; this_dev < MAX_NE_CARDS; this_dev++) {
		if (pdev_ne[this_dev])
			continue;
		pdev = platform_device_register_simple(
			DRV_NAME, this_dev, NULL, 0);
		if (IS_ERR(pdev))
			continue;
		pdev_ne[this_dev] = pdev;
	}
}

#ifdef MODULE
int __init init_module(void)
{
	int retval;
	ne_add_devices();
	retval = platform_driver_probe(&ne_driver, ne_drv_probe);
	if (retval) {
		if (io[0] == 0)
			printk(KERN_NOTICE "ne.c: You must supply \"io=0xNNN\""
				" value(s) for ISA cards.\n");
		ne_loop_rm_unreg(1);
		return retval;
	}

	
	ne_loop_rm_unreg(0);
	return retval;
}
#else 
static int __init ne_init(void)
{
	int retval = platform_driver_probe(&ne_driver, ne_drv_probe);

	
	ne_loop_rm_unreg(0);
	return retval;
}
module_init(ne_init);

struct net_device * __init ne_probe(int unit)
{
	int this_dev;
	struct net_device *dev;

	
	this_dev = 0;
	while ((pdev_ne[this_dev] && platform_get_drvdata(pdev_ne[this_dev])) ||
		io[this_dev]) {
		if (++this_dev == MAX_NE_CARDS)
			return ERR_PTR(-ENOMEM);
	}

	
	dev = alloc_eip_netdev();
	if (!dev)
		return ERR_PTR(-ENOMEM);

	sprintf(dev->name, "eth%d", unit);
	netdev_boot_setup_check(dev);

	io[this_dev] = dev->base_addr;
	irq[this_dev] = dev->irq;
	bad[this_dev] = dev->mem_end;

	free_netdev(dev);

	ne_add_devices();

	
	for (this_dev = 0; this_dev < MAX_NE_CARDS; this_dev++) {
		if (pdev_ne[this_dev]) {
			dev = platform_get_drvdata(pdev_ne[this_dev]);
			if (dev)
				return dev;
		}
	}

	return ERR_PTR(-ENODEV);
}
#endif 

static void __exit ne_exit(void)
{
	platform_driver_unregister(&ne_driver);
	ne_loop_rm_unreg(1);
}
module_exit(ne_exit);
