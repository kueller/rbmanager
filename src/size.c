#include <sys/ioctl.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <stdint.h>

#include "size.h"

/*
 * Why does this ioctl call work properly in C but not C++?
 * I have no idea...
 * So here's a token C file.
 */
uint64_t device_file_size(const char *devname)
{
	uint64_t size = 0;
	
	int fd = open(devname, O_RDONLY);
	if (fd != 0) size = 0;

	int r = ioctl(fd, BLKGETSIZE, &size);
	if (r != 0) size = 0;

	return size * 512;
}
