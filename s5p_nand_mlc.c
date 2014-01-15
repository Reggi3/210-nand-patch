/*
 *  linux/drivers/mtd/nand/s5p_nand_mlc.c - 16bit ecc driver for samsung s5pv210 chips
 *
 *  Copyright (C) 2013, FriendlyArm http://www.arm9home.net
 *
 *  Contributions by:
 *  Jeff Kent, jeff@jkent.net 2013
 *  I. Baker 2013
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Thanks to the following companies for their support:
 *
 *  http://www.andahammer.com
 *
 * this is an open source implementation of the 16bit ecc functions necessary to 
 * use samsung K9GBG08U0A 4GB nand flash chips on mini210S boards
 */

#include <asm/mach-types.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <linux/delay.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>

#include <plat/regs-nand.h>
#include <plat/nand.h>

#define S5P_NAND_WAIT_INTERVAL 25

#define S5P_NFECCREG(x)     (0x20000 + x)
#define S5P_NFECCCONF       S5P_NFECCREG(0x00)
#define S5P_NFECCCONT       S5P_NFECCREG(0x20)
#define S5P_NFECCSTAT       S5P_NFECCREG(0x30)
#define S5P_NFECCSECSTAT    S5P_NFECCREG(0x40)
#define S5P_NFECCPRGECC0    S5P_NFECCREG(0x90)
#define S5P_NFECCPRGECC1    S5P_NFECCREG(0x94)
#define S5P_NFECCPRGECC2    S5P_NFECCREG(0x98)
#define S5P_NFECCPRGECC3    S5P_NFECCREG(0x9C)
#define S5P_NFECCPRGECC4    S5P_NFECCREG(0xA0)
#define S5P_NFECCPRGECC5    S5P_NFECCREG(0xA4)
#define S5P_NFECCPRGECC6    S5P_NFECCREG(0xA8)
#define S5P_NFECCERL0       S5P_NFECCREG(0xC0)
#define S5P_NFECCERL1       S5P_NFECCREG(0xC4)
#define S5P_NFECCERL2       S5P_NFECCREG(0xC8)
#define S5P_NFECCERL3       S5P_NFECCREG(0xCC)
#define S5P_NFECCERL4       S5P_NFECCREG(0xD0)
#define S5P_NFECCERL5       S5P_NFECCREG(0xD4)
#define S5P_NFECCERL6       S5P_NFECCREG(0xD8)
#define S5P_NFECCERL7       S5P_NFECCREG(0xDC)
#define S5P_NFECCERP0       S5P_NFECCREG(0xF0)
#define S5P_NFECCERP1       S5P_NFECCREG(0xF4)
#define S5P_NFECCERP2       S5P_NFECCREG(0xF8)
#define S5P_NFECCERP3       S5P_NFECCREG(0xFC)
#define S5P_NFECCCONECC0    S5P_NFECCREG(0x110)
#define S5P_NFECCCONECC1    S5P_NFECCREG(0x114)
#define S5P_NFECCCONECC2    S5P_NFECCREG(0x118)
#define S5P_NFECCCONECC3    S5P_NFECCREG(0x11C)
#define S5P_NFECCCONECC4    S5P_NFECCREG(0x120)
#define S5P_NFECCCONECC5    S5P_NFECCREG(0x124)
#define S5P_NFECCCONECC6    S5P_NFECCREG(0x128)

#define S5P_NFCONT_nFCE2                (1<<23)
#define S5P_NFCONT_nFCE3                (1<<22)

#define S5P_NFECCCONF_ECCTYPE_16BIT     (5<< 0)
#define S5P_NFECCCONF_MSGLEN_SHIFT      16

#define S5P_NFECCCONT_ENCODE            (1<<16)
#define S5P_NFECCCONT_INITMECC          (1<< 2)

#define S5P_NFECCSTAT_ECCBUSY           (1<<31)
#define S5P_NFECCSTAT_ECCENCDONE        (1<<25)
#define S5P_NFECCSTAT_ECCDECDONE        (1<<24)

#define ECC_MODE_DECODE 0
#define ECC_MODE_ENCODE 1


extern unsigned int __machine_arch_type;

static unsigned int ecc_msg_len = 511;
static int cur_ecc_mode;

static void __iomem *regs;


static struct nand_ecclayout s5p_nand_oob_mlc_448 = {
	.eccbytes = 448,
	.eccpos = {
		 40,  41,  42,  43,  44,  45,  46,  47,
		 48,  49,  50,  51,  52,  53,  54,  55,
		 56,  57,  58,  59,  60,  61,  62,  63,
		 64,  65,  66,  67,  68,  69,  70,  71,
		 72,  73,  74,  75,  76,  77,  78,  79,
		 80,  81,  82,  83,  84,  85,  86,  87,
		 88,  89,  90,  91,  92,  93,  94,  95,
		 96,  97,  98,  99, 100, 101, 102, 103,
		104, 105, 106, 107, 108, 109, 110, 111,
		112, 113, 114, 115, 116, 117, 118, 119,
		120, 121, 122, 123, 124, 125, 126, 127,
		128, 129, 130, 131, 132, 133, 134, 135,
		136, 137, 138, 139, 140, 141, 142, 143,
		144, 145, 146, 147, 148, 149, 150, 151,
		152, 153, 154, 155, 156, 157, 158, 159,
		160, 161, 162, 163, 164, 165, 166, 167,
		168, 169, 170, 171, 172, 173, 174, 175,
		176, 177, 178, 179, 180, 181, 182, 183,
		184, 185, 186, 187, 188, 189, 190, 191,
		192, 193, 194, 195, 196, 197, 198, 199,
		200, 201, 202, 203, 204, 205, 206, 207,
		208, 209, 210, 211, 212, 213, 214, 215,
		216, 217, 218, 219, 220, 221, 222, 223,
		224, 225, 226, 227, 228, 229, 230, 231,
		232, 233, 234, 235, 236, 237, 238, 239,
		240, 241, 242, 243, 244, 245, 246, 247,
		248, 249, 250, 251, 252, 253, 254, 255,
		256, 257, 258, 259, 260, 261, 262, 263,
		264, 265, 266, 267, 268, 269, 270, 271,
		272, 273, 274, 275, 276, 277, 278, 279,
		280, 281, 282, 283, 284, 285, 286, 287,
		288, 289, 290, 291, 292, 293, 294, 295,
		296, 297, 298, 299, 300, 301, 302, 303,
		304, 305, 306, 307, 308, 309, 310, 311,
		312, 313, 314, 315, 316, 317, 318, 319,
		320, 321, 322, 323, 324, 325, 326, 327,
		328, 329, 330, 331, 332, 333, 334, 335,
		336, 337, 338, 339, 340, 341, 342, 343,
		344, 345, 346, 347, 348, 349, 350, 351,
		352, 353, 354, 355, 356, 357, 358, 359,
		360, 361, 362, 363, 364, 365, 366, 367,
		368, 369, 370, 371, 372, 373, 374, 375,
		376, 377, 378, 379, 380, 381, 382, 383,
		384, 385, 386, 387, 388, 389, 390, 391,
		392, 393, 394, 395, 396, 397, 398, 399,
		400, 401, 402, 403, 404, 405, 406, 407,
		408, 409, 410, 411, 412, 413, 414, 415,
		416, 417, 418, 419, 420, 421, 422, 423,
		424, 425, 426, 427, 428, 429, 430, 431,
		432, 433, 434, 435, 436, 437, 438, 439,
		440, 441, 442, 443, 444, 445, 446, 447,
		448, 449, 450, 451, 452, 453, 454, 455,
		456, 457, 458, 459, 460, 461, 462, 463,
		464, 465, 466, 467, 468, 469, 470, 471,
		472, 473, 474, 475, 476, 477, 478, 479,
		480, 481, 482, 483, 484, 485, 486, 487 },
	.oobfree = {
		{.offset = 4,
		 .length = 32 } }
};

static void s5p_nand_fast_iow(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	void *addr = (void *) regs + S3C_NFDATA;
	writesl(addr, buf, len >> 2);
	writesb(addr, buf + (len & ~0x3), len & 3);
}

static void s5p_nand_fast_ior(struct mtd_info *mtd, uint8_t *buf, int len)
{
	void *addr = (void *) regs + S3C_NFDATA;
	readsl(addr, buf, len >> 2);
	readsb(addr, buf + (len & ~0x3), len & 3);
}

static int s5p_nand_block_bad(struct mtd_info *mtd, loff_t ofs, int getchip)
{
	int page, res = 0;
	struct nand_chip *chip = mtd->priv;
	u8 bad;

	/* check bad block marker of first page */
	page = ofs >> chip->page_shift;
	chip->cmdfunc(mtd, NAND_CMD_READOOB, chip->badblockpos, page);
	bad = chip->read_byte(mtd);
	if (likely(chip->badblockbits == 8))
		res = (bad != 0xFF);
	else
		res = hweight8(bad) < chip->badblockbits;

	/* check bad block marker of last page */
	if (!res) {
		page += (mtd->erasesize - mtd->writesize) >> chip->page_shift;
		chip->cmdfunc(mtd, NAND_CMD_READOOB, chip->badblockpos, page);
		bad = chip->read_byte(mtd);
		if (likely(chip->badblockbits == 8))
			res = (bad != 0xFF);
		else
			res = hweight8(bad) < chip->badblockbits;
	}

	return res;	
}

static int s5p_nand_read_page_8bit(struct mtd_info *mtd, struct nand_chip *chip, uint8_t *buf, int page)
{
	int i, stat, col;
	const int msg_bytes = chip->ecc.size;
	const int ecc_bytes = chip->ecc.bytes;
	const int ecc_steps = chip->ecc.steps;
	const int ecc_total = chip->ecc.total;
	uint8_t *data;
	const uint8_t *ecc;
	uint32_t *mecc_pos = chip->ecc.layout->eccpos;

	/* read redundant area into posion buffer */
	chip->cmdfunc(mtd, NAND_CMD_RNDOUT, mtd->writesize, -1);
	chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);

	/* read and correct spare area */
	ecc_msg_len = 31;

	data = chip->oob_poi + 4;
	col = mtd->writesize + 4;
	ecc = chip->oob_poi + mecc_pos[0] + ecc_total;

	chip->cmdfunc(mtd, NAND_CMD_RNDOUT, col, -1);
	chip->ecc.hwctl(mtd, ECC_MODE_DECODE);
	chip->read_buf(mtd, data, 32);
	/* push spare ecc through hardware */
	chip->write_buf(mtd, ecc, ecc_bytes);
	chip->ecc.calculate(mtd, NULL, NULL);
	stat = chip->ecc.correct(mtd, data, NULL, NULL);
	if (stat == -1) {
		mtd->ecc_stats.failed++;
	}

	/* read and correct data area by page */
	ecc_msg_len = 511;

	data = buf;
	col = 0;
	ecc = chip->oob_poi + mecc_pos[0];

	for (i = 0; i < ecc_steps; i++) {
		chip->cmdfunc(mtd, NAND_CMD_RNDOUT, col, -1);
		chip->ecc.hwctl(mtd, ECC_MODE_DECODE);
		chip->read_buf(mtd, data + col, msg_bytes);
		chip->write_buf(mtd, ecc, ecc_bytes);
		chip->ecc.calculate(mtd, NULL, NULL);
		stat = chip->ecc.correct(mtd, data + col, NULL, NULL);
		if (stat == -1) {
			mtd->ecc_stats.failed++;
		}
		ecc += ecc_bytes;
		col += msg_bytes;
	}

	return 0;
}

static void s5p_nand_write_page_8bit(struct mtd_info *mtd, struct nand_chip *chip, const uint8_t *buf)
{
	int i;
	const int msg_bytes = chip->ecc.size;
	const int ecc_bytes = chip->ecc.bytes;
	const int ecc_steps = chip->ecc.steps;
	const int ecc_total = chip->ecc.total;
	int badoffs = mtd->writesize == 512 ? NAND_SMALL_BADBLOCK_POS : \
		NAND_LARGE_BADBLOCK_POS;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	uint32_t *mecc_pos = chip->ecc.layout->eccpos;
	const uint8_t *data = buf;
	uint8_t *ecc = ecc_calc;
	uint8_t spare_ecc[ecc_bytes];

	for (i = 0; i < ecc_steps; i++) {
		chip->ecc.hwctl(mtd, ECC_MODE_ENCODE);
		chip->write_buf(mtd, data, msg_bytes);
		chip->ecc.calculate(mtd, NULL, ecc);
		data += msg_bytes;
		ecc += ecc_bytes;
	}
	chip->oob_poi[badoffs] = 0xFF;
	memcpy(chip->oob_poi + mecc_pos[0], ecc_calc, ecc_total);
	chip->write_buf(mtd, chip->oob_poi, 4);

	ecc_msg_len = 31;
	chip->ecc.hwctl(mtd, ECC_MODE_ENCODE);
	chip->write_buf(mtd, chip->oob_poi + 4, 32);
	chip->ecc.calculate(mtd, NULL, spare_ecc);
	memcpy(chip->oob_poi + mecc_pos[0] + ecc_total, spare_ecc, sizeof(spare_ecc));
	chip->write_buf(mtd, chip->oob_poi + 4 + 32, mtd->oobsize - 4 - 32);

	ecc_msg_len = 511;
}

static int s5p_nand_read_oob_8bit(struct mtd_info *mtd, struct nand_chip *chip, int page, int sndcmd)
{
	int stat, bad, col;
	int eccbytes = chip->ecc.bytes;
	uint32_t *mecc_pos = chip->ecc.layout->eccpos;
	uint8_t *data;
	uint8_t spare_ecc[28];

	if (sndcmd)
		chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, page);
	chip->read_buf(mtd, chip->oob_poi, 4);

	if (likely(chip->badblockbits == 8))
		bad = chip->oob_poi[0] != 0xFF;
	else
		bad = hweight8(chip->oob_poi[0]) < chip->badblockbits;

	if (!bad) {
		/* read spare ecc */
		col = mtd->writesize + mecc_pos[0] + chip->ecc.total;

		chip->cmdfunc(mtd, NAND_CMD_RNDOUT, col, -1);
		chip->read_buf(mtd, spare_ecc, eccbytes);

		/* read and correct spare area */
		ecc_msg_len = 31;
		col = mtd->writesize + 4;
		data = chip->oob_poi + 4;

		chip->cmdfunc(mtd, NAND_CMD_RNDOUT, col, -1);
		chip->ecc.hwctl(mtd, ECC_MODE_DECODE);
		chip->read_buf(mtd, data, 32);
		/* push spare ecc through hardware */
		chip->write_buf(mtd, spare_ecc, eccbytes);
		chip->ecc.calculate(mtd, NULL, NULL);
		stat = chip->ecc.correct(mtd, data, NULL, NULL);

		if (stat < 0)
			mtd->ecc_stats.failed++;

		ecc_msg_len = 511;
	}
	return 0;
}

static int s5p_nand_write_oob_8bit(struct mtd_info *mtd, struct nand_chip *chip, int page)
{
	printk("s5p_nand: write page oob %8d\n", page);
	return -1;
}

static void s5p_nand_enable_hwecc_8bit(struct mtd_info *mtd, int mode)
{
	uint32_t tmp;

	cur_ecc_mode = mode;

	/* set length and type of ecc */
	tmp = (ecc_msg_len << S5P_NFECCCONF_MSGLEN_SHIFT) | S5P_NFECCCONF_ECCTYPE_16BIT;
	writel(tmp, regs + S5P_NFECCCONF);

	/* set encode/decode mode */
	tmp = readl(regs + S5P_NFECCCONT);
	if (cur_ecc_mode == ECC_MODE_ENCODE) {
		tmp |= S5P_NFECCCONT_ENCODE;
	} else { /* ECC_MODE_DECODE */
		tmp &= ~S5P_NFECCCONT_ENCODE;
	}
	writel(tmp, regs + S5P_NFECCCONT);

	/* clear RnB transition and illegal access bit */
	tmp = readl(regs + S3C_NFSTAT);
	tmp |= S3C_NFSTAT_RnB_CHANGE | S3C_NFSTAT_ILEGL_ACC;
	writel(tmp, regs + S3C_NFSTAT);

	/* clear encode and decode done flags */
	tmp = readl(regs + S5P_NFECCSTAT);
	tmp |= S5P_NFECCSTAT_ECCDECDONE | S5P_NFECCSTAT_ECCENCDONE;
	writel(tmp, regs + S5P_NFECCSTAT);

	/* unlock ecc encode/decode */
	tmp = readl(regs + S3C_NFCONT);
	tmp &= ~S3C_NFCONT_MECCLOCK;
	writel(tmp, regs + S3C_NFCONT);

	/* initialize ecc encoder/decoder */
	tmp = readl(regs + S5P_NFECCCONT);
	tmp |= S5P_NFECCCONT_INITMECC;
	writel(tmp, regs + S5P_NFECCCONT);
}

static inline void s5p_nand_wait_ecc_status(void)
{
	unsigned long timeo = jiffies;
	uint32_t tmp;
	uint32_t flag = (cur_ecc_mode == ECC_MODE_DECODE) ?
		S5P_NFECCSTAT_ECCDECDONE : S5P_NFECCSTAT_ECCENCDONE;

	timeo += S5P_NAND_WAIT_INTERVAL;

	for (;;) {
		if (time_after_eq(jiffies, timeo)) {
			printk("s5p-nand: ECC status error\n");
			break;
		}

		tmp = readl(regs + S5P_NFECCSTAT);
		if (tmp & flag) {
			break;
		}
		cond_resched();
	}
	tmp = readl(regs + S5P_NFECCSTAT);
	tmp |= flag;
	writel(tmp, regs + S5P_NFECCSTAT);
}

static inline void s5p_nand_wait_ecc_busy(void)
{
	unsigned long timeo = jiffies;
	uint32_t tmp;

	timeo += S5P_NAND_WAIT_INTERVAL;
	for (;;) {
		if (time_after_eq(jiffies, timeo)) {
			printk("s5p-nand: ECC busy\n");
			break;
		}
		tmp = readl(regs + S5P_NFECCSTAT);
		if (!(tmp & S5P_NFECCSTAT_ECCBUSY)) {
			break;
		}
		cond_resched();
	}
}

static int s5p_nand_calculate_ecc_8bit(struct mtd_info *mtd, const uint8_t *dat, uint8_t *ecc_code)
{
	uint32_t buf[7];
	uint32_t tmp;

	tmp = readl(regs + S3C_NFCONT);
	tmp |= S3C_NFCONT_MECCLOCK;
	writel(tmp, regs + S3C_NFCONT);

	s5p_nand_wait_ecc_status();

	if (cur_ecc_mode == ECC_MODE_ENCODE) {
		buf[0] = readl(regs + S5P_NFECCPRGECC0);
		buf[1] = readl(regs + S5P_NFECCPRGECC1);
		buf[2] = readl(regs + S5P_NFECCPRGECC2);
		buf[3] = readl(regs + S5P_NFECCPRGECC3);
		buf[4] = readl(regs + S5P_NFECCPRGECC4);
		buf[5] = readl(regs + S5P_NFECCPRGECC5);
		buf[6] = readl(regs + S5P_NFECCPRGECC6) | 0xFFFF0000;
		memcpy(ecc_code, buf, sizeof(buf));
	}
	return 0;
}

static int s5p_nand_correct_data_8bit(struct mtd_info *mtd, uint8_t *dat, uint8_t *read_ecc, uint8_t *calc_ecc)
{
	uint32_t err_info;
	int err_count;
	int i;
	uint32_t nfeccerl[8];
	uint32_t nfeccerp[4];
	uint16_t *loc;
	uint8_t *pat;

	s5p_nand_wait_ecc_busy();

	err_info = readl(regs + S5P_NFECCSECSTAT);
	if (err_info & 0x20000000) { /* unknown/undocumented bit */
		printk("s5p-nand: NFEECCSECSTAT = %08x\n", err_info);
		return 0;
	}

	err_count = err_info & 0x1f;
	if (!err_count) {
		return 0;
	}

	if (err_count > 16) {
		printk("s5p-nand: ECC uncorrectable error(s) detected\n");
		return -1;
	}

	//printk("s5p-nand: ECC correcting %d error(s)\n", err_count);

	nfeccerl[0] = readl(regs + S5P_NFECCERL0);
	nfeccerl[1] = readl(regs + S5P_NFECCERL1);
	nfeccerl[2] = readl(regs + S5P_NFECCERL2);
	nfeccerl[3] = readl(regs + S5P_NFECCERL3);
	nfeccerl[4] = readl(regs + S5P_NFECCERL4);
	nfeccerl[5] = readl(regs + S5P_NFECCERL5);
	nfeccerl[6] = readl(regs + S5P_NFECCERL6);
	nfeccerl[7] = readl(regs + S5P_NFECCERL7);

	nfeccerp[0] = readl(regs + S5P_NFECCERP0);
	nfeccerp[1] = readl(regs + S5P_NFECCERP1);
	nfeccerp[2] = readl(regs + S5P_NFECCERP2);
	nfeccerp[3] = readl(regs + S5P_NFECCERP3);

	loc = (uint16_t *) nfeccerl;
	pat = (uint8_t *) nfeccerp;

	for (i = 0; i < err_count; i++) {
		dat[loc[i]] ^= pat[i];
	}

	return err_count;
}

static void s5p_nand_chip_reset(int chipnr)
{
	const unsigned char cs_bit[] = {1, 2, 22, 23};
	int i;
	uint32_t tmp;

	if (chipnr > 3)
		return;

	/* assert chip select */
	tmp = readl(regs + S3C_NFCONT);
	tmp &= ~(1 << cs_bit[chipnr]);
	writel(tmp, regs + S3C_NFCONT);

	/* clear RnB transition event */
	/*writel(readl(regs + S3C_NFCONT) | S3C_NFCONT_INITSECC, regs + S3C_NFCONT);*/
	writeb(NAND_CMD_RESET, regs + S3C_NFCMMD);

	for (i = 0; i <= 10000; i++) {
		/* wait for a busy/idle transition */
		tmp = readl(regs + S3C_NFSTAT);
		if ((tmp & S3C_NFSTAT_READY) && (tmp & S3C_NFSTAT_RnB_CHANGE)) {
			break;
		}
		udelay(10);
	}

	/* deassert chip select */
	tmp = readl(regs + S3C_NFCONT);
	tmp |= (1 << cs_bit[chipnr]);
	writel(tmp, regs + S3C_NFCONT);
}

int s5p_nand_ext_finit(struct nand_chip *nand, void __iomem *nandregs)
{
	if (!nandregs) {
		return -1;
	}

	regs = nandregs;
	nand->read_buf = s5p_nand_fast_ior;
	nand->write_buf = s5p_nand_fast_iow;

	s5p_nand_chip_reset(0);
	/* s5p_nand_chip_reset(1); */

	return 0;
}

int s5p_nand_mlc_probe(struct nand_chip *nand, void __iomem *nandregs, u_char *ids)
{
	if (nandregs && !regs) {
		regs = nandregs;
	}

	/* data */
	nand->ecc.layout = &s5p_nand_oob_mlc_448;
	nand->ecc.size = 512;
	nand->ecc.bytes = 28;

	/* function pointers */
	nand->block_bad = s5p_nand_block_bad;
	nand->ecc.read_page = s5p_nand_read_page_8bit;
	nand->ecc.write_page = s5p_nand_write_page_8bit;
	nand->ecc.read_oob = s5p_nand_read_oob_8bit;
	nand->ecc.write_oob = s5p_nand_write_oob_8bit;
	nand->ecc.hwctl = s5p_nand_enable_hwecc_8bit;
	nand->ecc.calculate = s5p_nand_calculate_ecc_8bit;
	nand->ecc.correct = s5p_nand_correct_data_8bit;
	
	return 0;
}

