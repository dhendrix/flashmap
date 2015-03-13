Version 1.1 (draft)

Original FMAP spec is attributed to Tim Hockin @ Google, Inc, 2007

Documentation License:

[![](http://i.creativecommons.org/l/by/3.0/us/88x31.png)](http://creativecommons.org/licenses/by/3.0/us/)  [Creative Commons Attribution 3.0 United States License](http://creativecommons.org/licenses/by/3.0/us/)

# Introduction #

The objective of this document is to define a mechanism by which the firmware can expose a map of the flash chip to the OS and userland tools.  This will enable tools to better interact with firmware images.

# Details #

Traditionally, a firmware update meant an image is loaded into the flash chip, completely replacing the old flash image.  How do you know that a flash succeeded?  To checksum the flash chip, one could simply run a tool such as md5sum on the entire flash device. Firmware for modern platforms are introducing features like a "rolling BIOS" (a fallback image in flash), flash-based event logs, and other persistent non-volatile storage in flash.

We no longer want to treat the firmware as one single binary image.  Instead we need to know which areas of the firmware image are checksum-able, and which areas are being used for things that can change over time.  These areas will obviously be different across firmware vendors and platforms, but can also be different across firmware revisions on the same platform.  There is currently no way to know, for a given firmware image, what is the **static checksum**.  Therefore, we need a way for the firmware to expose a map to the OS and other interested parties, describing exactly how the flash is laid out.

# Requirements #

Fmap has one hard requirement, in two parts. It must provide enough information to generate a reliable static checksum of a firmware image.  The information must be available both on a running firmware and in a golden image.  Anything else is gravy.

# Terminology #

There are several terms which must be explained in order to understand the concepts presented in this design document.

## Firmware ##

Firmware is the the code run first when a computer is powered on. There are many places firmware may exist in a system -- on the motherboard, on add-in devices (e.g. option ROMs), and even the CPU die itself. It is commonly stored in a flash memory device often referred to as ROM, which is usually shorthand for writable EEPROM.

The fundamental purpose of firmware is to prepare the system to execute an operating system kernel. Thus, it may consist of hardware initialization routines, tables to expose hardware-specific details to the OS, etc. Some implementations make a distinction between the firmware and bootloader, the latter being the part which is responsible for fetching the kernel from storage, loading it into memory and executing it. Historically in the PC world firmware is known as the BIOS.

## Static Checksum ##

Typically a checksum of the flash covers the entire flash chip.  Since the firmware is starting to use the flash chip for more dynamic data, the checksum of the entire flash chip is no longer constant.  We use the checksum to verify a successful flash update, as well as to verify version matches when investigating problems.  We will use the term **static checksum** to mean the checksum of the flash chip areas which should not ever change at runtime.

## ACPI ##

Advanced Configuration and Power Interface.  ACPI is an industry standard format for (among other things) tables of structured data that the firmware wants to expose to the OS.

## SMBIOS ##


System Management BIOS.  SMBIOS is an open specification that defines an in-memory table of data structures which describe the running system. http://www.dmtf.org/standards/smbios/

# Overview #

The firmware will expose a map of the flash chip which describes the location of the chip in the physical memory map, as well as some other data about the flash chip, such as size and access modes.  The map will detail the layout of the data in the flash chip by describing regions and details about those regions.  Higher-level tools, such as a flash checksum utility, can then process this map and make more intelligent decisions about their behavior.  A checksum tool can skip volatile areas, such as the event log, and could then produce a verifiable static checksum of the flash.

# Detailed Design #

## Finding the FMAP table ##

There are three main ways to export structured data from firmware to the OS: ACPI, SMBIOS, and custom.  ACPI tables are slightly complicated and have some fixed overhead, but have the advantages of being easy to find in memory and having a relatively well-defined header. SMBIOS is well defined and very amenable to structured data.  Custom tables are simple and easy to implement (mostly because there is no imposed spec) but are non-standard.

Any one of these would be sufficient for describing the running firmware.  ACPI and SMBIOS, however, are strictly in-memory, and are usually generated at run time.  This does not solve the requirement of being able to checksum an on-disk ROM image.  For that reason, we are defining FMAP in terms of a custom table, which can be found in the firmware image both at run time and on disk.

The FMAP table can be found by scanning the firmware image for a 64-byte aligned signature, "`__FMAP__`".

## The FMAP structure ##

Once the table has been found, the reader can interpret the data as a packed structure of the following format:

```
        struct {
                uint8_t fmap_signature[8];       /* "__FMAP__" (0x5F5F50414D465F5F) */
                uint8_t  fmap_ver_major;       /* major version number of this structure */
                uint8_t  fmap_ver_minor;       /* minor version of this structure */
                uint64_t fmap_base;            /* physical memory-mapped address of the flash chip */
                uint32_t fmap_size;            /* size of the flash chip in bytes */
                uint8_t  fmap_name[32];        /* descriptive name of this flash device, 0 terminated */
                uint16_t fmap_nareas;          /* number of areas described by fmap_areas[] below */
                struct {
                        uint32_t area_offset;  /* offset of this area in the flash device */
                        uint32_t area_size;    /* size of this area in bytes */
                        uint8_t  area_name[32];/* descriptive name of this area, 0 terminated */
                        uint16_t area_flags;   /* flags for this area */
                } fmap_areas[0];
        } __attribute__((packed));

```

The **fmap\_signature** defines the start of this structure.  The signature is "`__FMAP__`" or 0x5F5F464D41505F5F when read as a single 64-bit value in little endian format.

The **fmap\_ver\_major** field defines the major revision number of this structure.  Within a major revision, any change to the structure must be backwards compatible.  The current major revision is 1.

The **fmap\_ver\_minor** field defines the minor revision number of this structure.  Minor revisions are always backwards compatible, within a major revision.  The current minor revision is 0.

The **fmap\_base** field defines the base address of the flash chip in the physical memory map.  For the system firmware this is typically immediately below the 4 GB address mark.

The **fmap\_size** field defines the total size of the flash chip in bytes.  The entire area of the flash chip must be accounted for in the **fmap\_areas[.md](.md)** array.

The **fmap\_name[.md](.md)** field is an optional descriptive name for this FMAP instance.  The array is treated as a zero-terminated ASCII string.

The **fmap\_nareas** field defines the number of entries in the **fmap\_areas[.md](.md)** array.  The areas described by **fmap\_areas[.md](.md)** are assumed to be contiguous areas starting at offset 0 of the flash chip.  An **fmap\_area** can start at any address and can be any size up to **fmap\_size**.

The **area\_size** field defines the size of an **fmap\_area**.  Because they are contiguous, the offset of an **fmap\_area** can be calculated by accumulating the sizes of the **fmap\_area** structures before the structure in question.

The **area\_name[.md](.md)** field is an optional descriptive name for this **fmap\_area**.  The array is treated as a zero-terminated ASCII string

The **area\_flags** field defines attributes of an **fmap\_area**.  These attributes are detailed below.

## Area flags ##

Each **fmap\_area** can have a combination of several flags.  These area\_flags are intended to act as a hint to higher level tools on how to handle the fmap\_area.  The following are the known area\_flags values:

```
    #define FMAP_AREA_STATIC      (1 << 0)       /* area's contents will not change */
    #define FMAP_AREA_COMPRESSED  (1 << 1)       /* area holds potentially compressed data 
    #define FMAP_AREA_RO          (1 << 2)       /* area is considered read-only */
```

Areas marked with FMAP\_AREA\_STATIC can be checksummed.  The checksum of all such areas constitutes the static checksum of the flash image.  Areas that do not have the FMAP\_AREA\_STATIC flag set must not be included as part of the static checksum.

Areas marked with FMAP\_AREA\_COMPRESSED hold data that is partially or fully compressed.

Areas marked with FMAP\_AREA\_RO are to be considered "read-only". Write operations are not guaranteed to work.

# Revision history #
## 1.00 ##
First public version

## 1.01 ##
- Change alignment requirement from 4-byte to 64-byte.

- Add new area type (FMAP\_AREA\_RO) to indicate region is read-only and clarified the meaning of "static."