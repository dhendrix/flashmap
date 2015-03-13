Flashmap, or fmap for short, is a very simple specification to expose layout of firmware images which typically reside in a small flash memory chip in computers. It makes no assumption about the binary it is embedded in or its underlying technology (EFI, Coreboot, PC BIOS, etc). In theory, fmap could be modified to describe a myriad of binary blobs. For our purposes, we will describe things such as fields and flags in the context of an individual firmware image.

fmap is currently used to map out regions of firmware images with interesting properties. This allows one to easily determine which regions are volatile or static, which are compressed, which can be safely overwritten, etc.

Firmware ROM layout is implementation-defined, dependent on the firmware core in use, the size of the physical ROM, the needs of the system to organize data in ROM, etc. The fmap specification is merely a way to expose layout information and does not dictate any standard by which the layout should be formed.

Here is a verbose example of how a parser might display fmap content:
```
fmap_signature="0x5f5f50414d465f5f" fmap_ver_major="1" fmap_ver_minor="0"
fmap_base="0x00000000ffe00000" fmap_size="0x200000" fmap_name="System BIOS" fmap_nareas="4"
area_offset="0x00000000" area_size="0x00020000" area_name="NVRAM" area_flags=""
area_offset="0x00020000" area_size="0x0000ffff" area_name="EVENT_LOG" area_flags=""
area_offset="0x001c0000" area_size="0x00040000" area_name="FV_BB" area_flags="static"
area_offset="0x00040000" area_size="0x00180000" area_name="FV_MAIN" area_flags="static,compressed"
```
From this, we see that there are four regions mapped out in the "System BIOS." The regions "FV\_BB" and "FV\_MAIN" are listed as static, meaning that their contents will not change during operation of the computer. In practice, this allows us to do things such as calculate a checksum of regions expected to remain consistent and compare with those of the original golden image.