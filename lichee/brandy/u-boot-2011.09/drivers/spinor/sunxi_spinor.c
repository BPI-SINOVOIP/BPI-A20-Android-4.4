/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <malloc.h>
#include <spi.h>
#include <asm/arch/spi.h>
#include <sunxi_mbr.h>

static int   spinor_flash_inited = 0;
static uint  total_write_bytes;
static char *spinor_store_buffer;
static char  spinor_mbr[SUNXI_MBR_SIZE];
static char *spinor_write_cache;
static int   spinor_cache_block = -1;

#define  SYSTEM_PAGE_SIZE        (512)
#define  SPINOR_PAGE_SIZE        (256)
#define  NPAGE_IN_1SYSPAGE       (SYSTEM_PAGE_SIZE/SPINOR_PAGE_SIZE)
#define  SPINOR_BLOCK_BYTES      (64 * 1024)
#define  SPINOR_BLOCK_SECTORS    (SPINOR_BLOCK_BYTES/512)

/* instruction definition */
#define SPINOR_READ        0x03
#define SPINOR_FREAD       0x0b
#define SPINOR_WREN        0x06
#define SPINOR_WRDI        0x04
#define SPINOR_RDSR        0x05
#define SPINOR_WRSR        0x01
#define SPINOR_PP          0x02
#define SPINOR_SE          0xd8
#define SPINOR_BE          0x60
#define SPINOR_RDID        0x9f

/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static int __spinor_wren(void)
{
    uint sdata = SPINOR_WREN;
    int ret = -1;

	ret = spic_rw(1, (void *)&sdata, 0, NULL);

	return ret;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static int __spinor_rdsr(u8 *reg)
{
	uint sdata = SPINOR_RDSR;
    int ret = -1;

	ret = spic_rw(1, (void *)&sdata, 1, (void *)reg);

	return ret;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static int __spinor_erase_block(uint block_index)
{
    uint  blk_addr = block_index * SPINOR_BLOCK_BYTES;
    u8    sdata[4] = {0};
    int   ret = -1;
    u8    status = 0;
    uint  i = 0;
    uint  txnum, rxnum;

	ret = __spinor_wren();
	if (-1 == ret)
	{
	    return -1;
	}

	txnum = 4;
	rxnum = 0;

	sdata[0] = SPINOR_SE;
	sdata[1] = (blk_addr >> 16) & 0xff;
	sdata[2] = (blk_addr >> 8 ) & 0xff;
	sdata[3] =  blk_addr        & 0xff;

	ret = spic_rw(txnum, (void *)sdata, rxnum, 0);
	if (-1 == ret)
    {
        return -1;
    }

	do
	{
	    ret = __spinor_rdsr(&status);
	    if (-1 == ret)
	    {
	        return -1;
	    }

	    for (i = 0; i < 100; i++);
	} while (status & 0x01);

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static int __spinor_erase_all(void)
{
    uint  sdata = 0;
    int   ret = -1;
    u8    status = 0;
    uint  i = 0;
    uint  txnum, rxnum;

	ret = __spinor_wren();
	if (-1 == ret)
	{
	    return -1;
	}

	txnum = 1;
	rxnum = 0;

	sdata = SPINOR_BE;

	ret = spic_rw(txnum, (void*)&sdata, rxnum, 0);
	if (ret==-1)
    {
        return -1;
    }

	do
	{
	    ret = __spinor_rdsr(&status);
	    if (-1 == ret)
	    {
	        return -1;
	    }

	    for (i = 0; i < 100; i++);
	} while (status & 0x01);

	return 0;
}

/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static int __spinor_pp(uint page_addr, void *buf, uint len)
{
    u8   sdata[260];
    u8   status = 0;
    uint i = 0;
    int  ret = -1;
    uint txnum, rxnum;

    if (len > 256)
    {
        return -1;
    }

	ret = __spinor_wren();
	if (ret < 0)
	{
	    return -1;
	}

	txnum = len+4;
	rxnum = 0;

	memset((void *)sdata, 0xff, 260);
    sdata[0] = SPINOR_PP;
	sdata[1] = (page_addr >> 16) & 0xff;
	sdata[2] = (page_addr >> 8 ) & 0xff;
	sdata[3] =  page_addr        & 0xff;

	memcpy((void *)(sdata+4), buf, len);
    ret = spic_rw(txnum, (void *)sdata, rxnum, 0);
	if (-1 == ret)
	{
        return -1;
    }

	do
	{
	    ret = __spinor_rdsr(&status);
	    if (ret < 0)
	    {
	        return -1;
	    }

	    for (i=0; i < 100; i++);
	} while (status & 0x01);

    return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static int __spinor_read_id(uint *id)
{
    uint sdata = SPINOR_RDID;

	return spic_rw(1, (void *)&sdata, 3, (void *)id);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static int __spinor_sector_read(uint start, uint sector_cnt, void *buf)
{
    uint page_addr;
    uint rbyte_cnt;
    u8   sdata[4] = {0};
    int ret = 0;
    uint tmp_cnt, tmp_offset = 0;
    void  *tmp_buf;
    uint txnum, rxnum;

    txnum = 4;

    while (sector_cnt)
    {
        if (sector_cnt > 127)
        {
            tmp_cnt = 127;
        }
        else
        {
            tmp_cnt = sector_cnt;
        }

        page_addr = (start + tmp_offset) * SYSTEM_PAGE_SIZE;
        rbyte_cnt = tmp_cnt * SYSTEM_PAGE_SIZE;
        sdata[0]  =  SPINOR_READ;
    	sdata[1]  = (page_addr >> 16) & 0xff;
    	sdata[2]  = (page_addr >> 8 ) & 0xff;
    	sdata[3]  =  page_addr        & 0xff;

        rxnum   = rbyte_cnt;
        tmp_buf = (u8 *)buf + (tmp_offset << 9);
        if (spic_rw(txnum, (void *)sdata, rxnum, tmp_buf))
        {
            ret = -1;
            break;
        }

        sector_cnt -= tmp_cnt;
        tmp_offset += tmp_cnt;
    }

    return ret;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static int __spinor_sector_write(uint sector_start, uint sector_cnt, void *buf)
{
    uint page_addr = sector_start * SYSTEM_PAGE_SIZE;
    uint nor_page_cnt = sector_cnt * (NPAGE_IN_1SYSPAGE);
    uint i = 0;
    int ret = -1;

	printf("start = 0x%x, cnt=0x%x\n", sector_start, sector_cnt);
	printf("nor_page_cnt=%d\n", nor_page_cnt);
    for (i = 0; i < nor_page_cnt; i++)
    {
    	//printf("spinor program page : 0x%x\n", page_addr + SPINOR_PAGE_SIZE * i);
		if((i & 0x1ff) == 0x1ff)
		{
			printf("nor_page_cnt=%d\n", i);
		}
        ret = __spinor_pp(page_addr + SPINOR_PAGE_SIZE * i, (void *)((uint)buf + SPINOR_PAGE_SIZE * i), SPINOR_PAGE_SIZE);
        if (-1 == ret)
        {
            return -1;
        }
    }

    return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int spinor_init(int stage)
{
	if(spinor_flash_inited)
	{
		puts("sunxi spinor is already inited\n");
	}
	else
	{
		puts("sunxi spinor is initing...");
		if(spic_init(0))
		{
			puts("Fail\n");

			return -1;
		}
		else
		{
			puts("OK\n");
		}

	}
	spinor_flash_inited ++;

	if(uboot_spare_head.boot_data.work_mode != WORK_MODE_BOOT)
	{
		spinor_store_buffer  = (char *)malloc(16 * 1024 * 1024);
		if(!spinor_store_buffer)
		{
			puts("memory malloced fail for store buffer\n");

			return -1;
		}
	}
	spinor_write_cache  = (char *)malloc(64 * 1024);
	if(!spinor_write_cache)
	{
		puts("memory malloced fail for spinor data buffer\n");

		if(spinor_store_buffer)
		{
			free(spinor_store_buffer);

			return -1;
		}
	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int spinor_exit(int force)
{
	int need_exit = 0;
	int ret = 0;

	if(!spinor_flash_inited)
	{
		printf("sunxi spinor has not inited\n");

		return -1;
	}
	if(force == 1)
	{
		if(spinor_flash_inited)
		{
			printf("force sunxi spinor exit\n");

			spinor_flash_inited = 0;
			need_exit = 1;
		}
	}
	else
	{
		if(spinor_flash_inited)
		{
			spinor_flash_inited --;
		}
		if(!spinor_flash_inited)
		{
			printf("sunxi spinor is exiting\n");
			need_exit = 1;
		}
	}
	if(need_exit)
	{
		if((spinor_cache_block >= 0) && (spinor_write_cache))
		{
			if(__spinor_sector_write(spinor_cache_block * SPINOR_BLOCK_SECTORS, SPINOR_BLOCK_SECTORS, spinor_write_cache))
			{
				printf("spinor write cache block fail\n");

				ret = -1;
			}
		}
		//这里关闭spinor控制器
		spic_exit(0);
	}

	return ret;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int spinor_read(uint start, uint nblock, void *buffer)
{
	int tmp_block_index;

	printf("spinor read: start 0x%x, sector 0x%x\n", start, nblock);

	if(spinor_cache_block < 0)
	{
		printf("%s %d\n", __FILE__, __LINE__);
		if(__spinor_sector_read(start, nblock, buffer))
		{
			printf("spinor read fail no buffer\n");

			return 0;
		}
	}
	else
	{
		tmp_block_index = start/SPINOR_BLOCK_SECTORS;
		if(spinor_cache_block == tmp_block_index)
		{
			memcpy(buffer, spinor_write_cache + (start % SPINOR_BLOCK_SECTORS) * 512, nblock * 512);
		}
		else
		{
			if(__spinor_sector_read(start, nblock, buffer))
			{
				printf("spinor read fail with buffer\n");

				return 0;
			}
		}
	}

	return nblock;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int spinor_write(uint start, uint nblock, void *buffer)
{
	int tmp_block_index;

	printf("spinor write: start 0x%x, sector 0x%x\n", start, nblock);

	if(spinor_cache_block < 0)
	{
		spinor_cache_block = start/SPINOR_BLOCK_SECTORS;
		if(__spinor_sector_read(spinor_cache_block * SPINOR_BLOCK_SECTORS, SPINOR_BLOCK_SECTORS, spinor_write_cache))
		{
			printf("spinor read cache block fail\n");

			return 0;
		}
		__spinor_erase_block(spinor_cache_block);
		memcpy(spinor_write_cache + (start % SPINOR_BLOCK_SECTORS) * 512, buffer, nblock * 512);
	}
	else
	{
		tmp_block_index = start/SPINOR_BLOCK_SECTORS;
		if(spinor_cache_block == tmp_block_index)
		{
			memcpy(spinor_write_cache + (start % SPINOR_BLOCK_SECTORS) * 512, buffer, nblock * 512);
		}
		else
		{
			if(__spinor_sector_write(spinor_cache_block * SPINOR_BLOCK_SECTORS, SPINOR_BLOCK_SECTORS, spinor_write_cache))
			{
				printf("spinor write cache block fail\n");

				return 0;
			}
			spinor_cache_block = tmp_block_index;
			if(__spinor_sector_read(spinor_cache_block * SPINOR_BLOCK_SECTORS, SPINOR_BLOCK_SECTORS, spinor_write_cache))
			{
				printf("spinor read cache block fail\n");

				return 0;
			}
			__spinor_erase_block(spinor_cache_block);
			memcpy(spinor_write_cache + (start % SPINOR_BLOCK_SECTORS) * 512, buffer, nblock * 512);
		}
	}

	return nblock;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int spinor_flush_cache(void)
{
	if((spinor_cache_block >= 0) && (spinor_write_cache))
	{
		if(__spinor_sector_write(spinor_cache_block * SPINOR_BLOCK_SECTORS, SPINOR_BLOCK_SECTORS, spinor_write_cache))
		{
			printf("spinor write cache block fail\n");

			return -1;
		}
	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :  erase:  0:
*
*							  1: erase the all flash
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int spinor_erase_all_blocks(int erase)
{
	if(erase)		//当参数为0，表示根据情况，自动判断是否需要进入擦除
	{
		__spinor_erase_all();
	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int spinor_size(void)
{
	return 8 * 1024 * 1024/512;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int spinor_sprite_write(uint start, uint nblock, void *buffer)
{
	printf("spinor burn write: start 0x%x, sector 0x%x\n", start, nblock);

	memcpy(spinor_store_buffer + start*512, buffer, nblock*512);

	total_write_bytes += nblock * 512;

	return nblock;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
//static void spinor_dump(void *buffer, int len)
//{
//	char *addr;
//	int  i;
//
//	addr = (char *)buffer;
//	for(i=0;i<len;i++)
//	{
//		printf("%02x  ", addr[i]);
//		if((i & 0x07) == 0x07)
//		{
//			puts("\n");
//		}
//	}
//}

int spinor_datafinish(void)
{
	//char  test_buffer[512];
	//int   i,j;
	uint  id = 0xff;
	//char  *buffer;
	int   ret;

	printf("spinor_datafinish\n");
	spinor_get_private_data((void *)spinor_mbr, 1);

//	while((*(volatile unsigned int *)0) != 0x1234);
	__spinor_read_id(&id);

	printf("spinor id = 0x%x\n", id);

	__spinor_erase_all();

	printf("spinor has erasered\n");

	printf("total write bytes = %d\n", total_write_bytes);
	ret = __spinor_sector_write(0, total_write_bytes/512, spinor_store_buffer);
	if(ret)
	{
		printf("spinor write img fail\n");

		return -1;
	}
//	buffer = (char *)malloc(8 * 1024 * 1024);
//	memset(buffer, 0xff, 8 * 1024 * 1024);
//	ret = __spinor_sector_read(0, total_write_bytes/512, buffer);
//	if(ret)
//	{
//		printf("spinor read img fail\n");
//
//		return -1;
//	}
//	printf("spinor read data ok\n");
//	for(i=0;i<total_write_bytes;i++)
//	{
//		if(buffer[i] != spinor_store_buffer[i])
//		{
//			printf("compare spinor read and write error\n");
//			printf("offset %d\n", i);
//
//			return -1;
//		}
//	}

	printf("spinor download data ok\n");

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int nand_get_mbr(char *buffer, int mbr_size)
{
	memcpy(spinor_mbr, buffer, SUNXI_MBR_SIZE);

	return 0;
}
