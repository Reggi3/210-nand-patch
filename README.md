README
-------
An opensource linux 16bit ecc mlc nand driver addon for the samsung s5pv210 based boards, currently 
only tested on friendlyarm mini210S boards  with 4GB of mlc nand marked K9GBG08U0A using 
friendlyarm's linux 3.0.8 kernel source code, block size = 1MB, page size = 8192b.  This code was 
built for the 3.0.8 kernel source from the Mini210s-20120913.iso but has been tested to work with 
the 2013 kernel sources available on baidu.  The s5p_nand_mlc.c driver was originally tested on the
 2.6.35 kernel sources, with the s5p_phy_init_ext.c file added for the 3.0.8 kernel sources.

this driver has only been tested with superboot.bin from the Mini210s-20120913.iso, it may or may 
not work with other versions of superboot although it should be fine.

*update* The driver has been updated to work on all versions of superboot, it's untested as of yet
but an error had crept into the driver via superboot, as this error has been corrected, any 
references to superboot from the mini210s-20120913.iso should be ignored!!!

This driver should allow us to start looking at using our own bootloaders and updated kernel 
versions in the future.

This driver replaces the s5p_mlc_nand.fo binary blob that is supplied in the kernel source code, 
the binary blob is called via the s5p_nand_mlc_probe and s5p_nand_ext_finit functions from 
drivers/mtd/nand/s3c_nand.c, the s5p_nand_mlc_probe function replaces all of the functions of the 
s3c_nand.c driver with it's own implementation of the read/write/ecc and helper functions which 
includes access to 16bit ecc, it also supplies the nand_ecclayout to read the K9GBG08U0A chips 
using 448 bytes ecc per 8KB page.

There is a function that is linked into the nand driver via s5p_phy_init_ext.c, it really doesn't 
have any reason to be in a nand driver but it breaks the kernel compilation without it. The 
function is s5p_phy_init_ext, this is a generic function used in 2 drivers, the s5p-echi driver and 
the s3c-sdhci driver.

the s5p_phy_init_ext function has now been broken down into 2 functions so it should be possible to 
put the code back into the respective drivers and forget s5p_phy_init_ext ever existed.

You may also wonder why some of the function names have 8bit in them, this is a misnomer, during 
the testing phase it was necessary to swap the functions in and out with the ones in the binary 
blob so the names just stuck, they are however capable of reading, writing and correcting upto and 
including 16bit ecc errors, maybe we'll rename them at some point in the future.

Usage
-----

place the s5p_nand_mlc.c, s5p_phy_init_ext.c and Makefile in the drivers/mtd/nand/ folder in your 
friendlyarm 3.0.8 kernel sources, this will overwrite the existing Makefile.  Next simply make your
 kernel, then using an SD card fused with the superboot.bin from the mini210s-20120913.iso and also 
 copied to the images folder, you can then burn the kernel image to nand.
 
You can also use this with the 2.6.35 kernel sources but you'll have to adjust the makefile 
yourself, you will only need the s5p_nand_mlc.c file in the drivers/mtd/nand/ folder for your
2.6.35 kernel sources.

Troubleshooting
---------------
The driver should work just fine if used with the correct superboot.bin, if you have the wrong 
superboot you will get a stream of 'uncorrectable nand ecc' errors, don't panic, this hasn't broken 
your nand chip, it's simply a mismatch between the driver and the bootloader, consequently the 
driver things all of the pages have ecc errors because the data isn't where it expected it.

To fix this issue simply fuse superboot.bin from the mini210s-20120913.iso file and make sure it's 
also in the images folder of your sd card's fat partition, then edit the images/FriendlyArm.ini 
file and set the LowFormat option:

LowFormat = Yes

This will wipe the nand, allowing superboot to flash to clean nand, once it's done you can reboot.

 
Please note, The first time you reboot after a LowFormat, the yaffs2 filesystem will scan the nand, 
if there are any real uncorrectable ecc errors then they will show here but will be mapped out, 
yaffs2 will create a checkpoint when the sync function is called, this is usually done 
automatically when the system is unmounted at reboot or shutdown, however, due to other system 
errors reboot doesn't always succeed (if you touch the touchscreen and set off a menu sound, a 
reboot will fail) and shutdown doesn't do anything yet.

There is a fix that will be published soon that deals with the reboot issue that also fixes an 
issue with connected usb devices failing to enumerate after a reboot.
