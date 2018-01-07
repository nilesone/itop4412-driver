#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>


#include "lcdfont.h"


#define OLED_CLEAR_ALL		0x100001
#define	OLED_CLEAR_PAGE		0x100002
#define OLED_SET_POS		0x100003

/* usage:
 * ./app_spi_oled clear_all
 * ./app_spi_oled clear_page  page
 * ./app_spi_oled write		  page 	col  string		 
 */

unsigned int fd;

void Usage(char *file)
{
	printf("%s clear_all\n", file);
	printf("%s clear_page page\n", file);
	printf("%s write      page col string\n", file);
}

/* 根据字模输出一个字符 */
void gpio_spi_write_char(unsigned int page, unsigned int col, unsigned char data_char)
{
	int i;
	unsigned int buff = data_char - ' ';
	ioctl(fd, OLED_SET_POS, page | (col<<8));
	write(fd, &oled_asc2_8x16[buff][0], 8);

	ioctl(fd, OLED_SET_POS, (page + 1) | (col<<8));
	write(fd, &oled_asc2_8x16[buff][0 + 8], 8);
}

/* 遍历每个字符 */
int lcd_spi_write(unsigned int page, unsigned int col, unsigned char *data, unsigned int count)
{
	int i;

	for(i=0; i<count; i++)
	{
		if((col + 8) > 128)
		{
			page += 2;
			col = 0;
		}
		
		if((page + 1) > 7)
			return -1;
		
		gpio_spi_write_char(page, col, data[i]);
		col  += 8; 
	}

	return 0;
}

int main(unsigned int argc, unsigned char **argv)
{
	if((argc != 2) && (argc != 3) && (argc != 5))
	{
		Usage(argv[0]);
		return -1;
	}

	fd = open("/dev/oled", O_RDWR);
	if (fd < 0)
	{
		printf("open /dev/oled fail!\n");
		return -1;
	}

	if (strcmp("clear_all", argv[1]) == 0)
	{
		ioctl(fd, OLED_CLEAR_ALL, 0);
	}	
	else if (strcmp("clear_page", argv[1]) == 0)
	{
		ioctl(fd, OLED_CLEAR_PAGE, strtoul(argv[2], NULL, 0));
	}
	else if (strcmp("write", argv[1]) == 0)
	{
		lcd_spi_write(strtoul(argv[2], NULL, 0), strtoul(argv[3], NULL, 0), argv[4], strlen(argv[4]));
	}
	else
	{
		Usage(argv[0]);		
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}
