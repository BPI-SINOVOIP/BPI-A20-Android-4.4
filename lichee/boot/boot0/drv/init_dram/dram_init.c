/*
*********************************************************************************************************
* File    : dram_init.c
* By      : Berg.Xing
* Date    : 2011-12-07
* Descript: dram for AW1625 chipset
* Update  : date      auther      ver     notes
*			2011-12-07			Berg        1.0     create file from aw1623
*			2012-01-11			Berg		    1.1		  kill bug for 1/2 rank decision
*	    2012-01-31			Berg		    1.2		  kill bug for clock frequency > 600MHz
*     2012-05-18      Daniel      1.3     Change itm_disable to Add Delay to CKE after Clock Stable
*     2012-06-12      Daniel      1.4     Function "mctl_enable_dllx", Add DQS Phase Adjust Option
*     2012-06-15      Daniel      1.5     Adjust Initial Delay(including relation among RST/CKE/CLK)
*     2012-06-28      Daniel      1.6     Add Three Times DRAM Initial for CARD Boot (DRAMC_init_EX)
*     2012-06-28      Daniel      1.7     Add DLL Parameter Scan Function      
*     2012-09-07      Daniel      1.8     Add more delay to all delay_us
*	  2013-03-04	  CPL		  1.9	  modify for A20
*	  2013-03-06	  CPL		  1.10	  use dram_tpr4 bit1 as DQ & CA mode setup
*	  2013-03-08	  CPL		  1.11	  fix the problem [EFR DRAM can't not exit from super selfrefresh]
*     2013-03-15      CPL         1.12    modify mbus source to PLL6*2 /4 = 300MHz
*     2013-03-15      CPL         1.13    read zq value from config para to fix superstandby bug.
i
*     2013-04-10      CPL         1.14    modify code for super standby
*			2013-06-24			YeShaozhen	1.15		add spread spectrum funciton, use dram_tpr4[2] configure
*			2013-06-28			YeShaozhen	1.16		DRAM frequency jump, use dram_tpr4[3] configure
*********************************************************************************************************
*/
#include "boot0_i.h"
#include "dram_i.h"

char dram_driver_version[8]={'1','.','1','5'};
typedef  boot_dram_para_t		__dram_para_t;
__u32 super_standby_flag = 0;

#pragma arm section rwdata="RW_KRNL_CLK",code="CODE_KRNL_CLK",rodata="RO_KRNL_CLK",zidata="ZI_KRNL_CLK"

extern __u32 get_cyclecount (void);
void mctl_delay(__u32 dly)
{
	__u32	i;
	for(i=0; i<dly; i++){};
}
/*
*********************************************************************************************************
*                                   DRAM INIT
*
* Description: dram init function
*
* Arguments  : para     dram config parameter
*
*
* Returns    : result
*
* Note       :
*********************************************************************************************************
*/
void mctl_ddr3_reset(void)
{
	__u32 reg_val;

	reg_val = mctl_read_w(SDR_CR);
	reg_val &= ~(0x1<<12);
	mctl_write_w(SDR_CR, reg_val);
	mctl_delay(0x100);
	reg_val = mctl_read_w(SDR_CR);
	reg_val |= (0x1<<12);
	mctl_write_w(SDR_CR, reg_val);
}

void mctl_set_drive(void)
{
	__u32 reg_val;

	reg_val = mctl_read_w(SDR_CR);
	reg_val |= (0x6<<12);
	reg_val |= 0xFFC;
	reg_val &= ~0x3;
	reg_val &= ~(0x3<<28);
//	reg_val |= 0x7<<20;
	mctl_write_w(SDR_CR, reg_val);
}

void mctl_itm_disable(void)
{
	__u32 reg_val = 0x0;

	reg_val = mctl_read_w(SDR_CCR);
	reg_val |= 0x1<<28;
	reg_val &= ~(0x1U<<31);          //danielwang, 2012-05-18
	mctl_write_w(SDR_CCR, reg_val);
}

void mctl_itm_enable(void)
{
	__u32 reg_val = 0x0;

	reg_val = mctl_read_w(SDR_CCR);
	reg_val &= ~(0x1<<28);
	mctl_write_w(SDR_CCR, reg_val);
}

void mctl_enable_dll0(__u32 phase)
{
    mctl_write_w(SDR_DLLCR0, (mctl_read_w(SDR_DLLCR0) & ~(0x3f<<6)) | (((phase>>16)&0x3f)<<6));
	mctl_write_w(SDR_DLLCR0, (mctl_read_w(SDR_DLLCR0) & ~0x40000000) | 0x80000000);

	//mctl_delay(0x100);
	delay_us(10);

	mctl_write_w(SDR_DLLCR0, mctl_read_w(SDR_DLLCR0) & ~0xC0000000);

	//mctl_delay(0x1000);
	delay_us(10);

	mctl_write_w(SDR_DLLCR0, (mctl_read_w(SDR_DLLCR0) & ~0x80000000) | 0x40000000);
	//mctl_delay(0x1000);
	delay_us(100);
}

void mctl_enable_dllx(__u32 phase)
{
	__u32 i = 0;
    __u32 reg_val;
    __u32 dll_num;
    __u32	dqs_phase = phase;

	reg_val = mctl_read_w(SDR_DCR);
	reg_val >>=6;
	reg_val &= 0x7;
	if(reg_val == 3)
		dll_num = 5;
	else
		dll_num = 3;

    for(i=1; i<dll_num; i++)
	{
		mctl_write_w(SDR_DLLCR0+(i<<2), (mctl_read_w(SDR_DLLCR0+(i<<2)) & ~(0xf<<14)) | ((dqs_phase&0xf)<<14));
		mctl_write_w(SDR_DLLCR0+(i<<2), (mctl_read_w(SDR_DLLCR0+(i<<2)) & ~0x40000000) | 0x80000000);
		dqs_phase = dqs_phase>>4;
	}

	//mctl_delay(0x100);
	delay_us(10);

    for(i=1; i<dll_num; i++)
	{
		mctl_write_w(SDR_DLLCR0+(i<<2), mctl_read_w(SDR_DLLCR0+(i<<2)) & ~0xC0000000);
	}

	//mctl_delay(0x1000);
	delay_us(10);

    for(i=1; i<dll_num; i++)
	{
		mctl_write_w(SDR_DLLCR0+(i<<2), (mctl_read_w(SDR_DLLCR0+(i<<2)) & ~0x80000000) | 0x40000000);
	}
	//mctl_delay(0x1000);
	delay_us(100);
}

void mctl_disable_dll(void)
{
	__u32 reg_val;

	reg_val = mctl_read_w(SDR_DLLCR0);
	reg_val &= ~(0x1<<30);
	reg_val |= 0x1U<<31;
	mctl_write_w(SDR_DLLCR0, reg_val);

	reg_val = mctl_read_w(SDR_DLLCR1);
	reg_val &= ~(0x1<<30);
	reg_val |= 0x1U<<31;
	mctl_write_w(SDR_DLLCR1, reg_val);

	reg_val = mctl_read_w(SDR_DLLCR2);
	reg_val &= ~(0x1<<30);
	reg_val |= 0x1U<<31;
	mctl_write_w(SDR_DLLCR2, reg_val);

	reg_val = mctl_read_w(SDR_DLLCR3);
	reg_val &= ~(0x1<<30);
	reg_val |= 0x1U<<31;
	mctl_write_w(SDR_DLLCR3, reg_val);

	reg_val = mctl_read_w(SDR_DLLCR4);
	reg_val &= ~(0x1<<30);
	reg_val |= 0x1U<<31;
	mctl_write_w(SDR_DLLCR4, reg_val);
}

void mctl_configure_hostport(void)
{
	__u32 i;
	__u32 hpcr_value[32] = {
		0x00000301,0x00000301,0x00000301,0x00000301,
		0x00000301,0x00000301,0x00000301,0x00000301,
		0x0,       0x0,       0x0,       0x0,
		0x0,       0x0,       0x0,       0x0,
		0x00001031,0x00001031,0x00000735,0x00001035,
		0x00001035,0x00000731,0x00001031,0x00000735,
		0x00001035,0x00001031,0x00000731,0x00001035,
		0x00001031,0x00000301,0x00000301,0x00000731,
	};

	for(i=0; i<8; i++)
	{
		mctl_write_w(SDR_HPCR + (i<<2), hpcr_value[i]);
	}
	
	for(i=16; i<28; i++)
	{
		mctl_write_w(SDR_HPCR + (i<<2), hpcr_value[i]);
	}	
	
	mctl_write_w(SDR_HPCR + (29<<2), hpcr_value[i]);
	mctl_write_w(SDR_HPCR + (31<<2), hpcr_value[i]);
}

void mctl_setup_dram_clock(__u32 clk, __u32 pll_tun_en)
{
    __u32 reg_val;
    
    //DISABLE PLL before configuration by yeshaozhen at 20130711
    reg_val = mctl_read_w(DRAM_CCM_SDRAM_PLL_REG);
    reg_val &= ~(0x1<<31);  	//PLL disable before configure
    mctl_write_w(DRAM_CCM_SDRAM_PLL_REG, reg_val);
    delay_us(20);
    //20130711 end
    
    if(pll_tun_en)
    {
	      //spread spectrum
		    reg_val	= 0x00;
		    reg_val	|= (0x0<<17);	//spectrum freq
		    reg_val	|= 0x3333;	//BOT
		    reg_val	|= (0x113<<20);	//STEP
		    reg_val	|= (0x2<<29);	//MODE Triangular
		    mctl_write_w(DRAM_CCM_SDRAM_PLL_TUN2, reg_val);
		    reg_val	|= (0x1<<31);	//enable spread spectrum
		    mctl_write_w(DRAM_CCM_SDRAM_PLL_TUN2, reg_val);
  	}
  	else
  	{
  	    mctl_write_w(DRAM_CCM_SDRAM_PLL_TUN2, 0x00000000);
  	}

    //setup DRAM PLL
    reg_val = mctl_read_w(DRAM_CCM_SDRAM_PLL_REG);
    reg_val &= ~0x3;
    reg_val |= 0x1;                                             //m factor
    reg_val &= ~(0x3<<4);
    reg_val |= 0x1<<4;                                          //k factor
    reg_val &= ~(0x1f<<8);
    reg_val |= ((clk/24)&0x1f)<<8;                              //n factor
    reg_val &= ~(0x3<<16);
    reg_val |= 0x1<<16;                                         //p factor
    reg_val &= ~(0x1<<29);                                      //clock output disable
    reg_val |= (__u32)0x1<<31;                                  //PLL En
    mctl_write_w(DRAM_CCM_SDRAM_PLL_REG, reg_val);
	//mctl_delay(0x100000);
	delay_us(10000);
	reg_val = mctl_read_w(DRAM_CCM_SDRAM_PLL_REG);
	reg_val |= 0x1<<29;
    mctl_write_w(DRAM_CCM_SDRAM_PLL_REG, reg_val);

	//setup MBUS clock
    reg_val = (0x1U<<31) | (0x1<<24) | (0x1<<0) | (0x1<<16);
    mctl_write_w(DRAM_CCM_MUS_CLK_REG, reg_val);

    //open DRAMC AHB & DLL register clock
    //close it first
    reg_val = mctl_read_w(DRAM_CCM_AHB_GATE_REG);
    reg_val &= ~(0x3<<14);
    mctl_write_w(DRAM_CCM_AHB_GATE_REG, reg_val);
	delay_us(10);
    //then open it
    reg_val |= 0x3<<14;
    mctl_write_w(DRAM_CCM_AHB_GATE_REG, reg_val);
	delay_us(10);
	
}

#define RTC_BASE			0x01c20c00
#define SDR_GP_REG0		    (RTC_BASE + 0x120)
#define SDR_GP_REG1         (RTC_BASE + 0x124)
#define SDR_GP_REG2         (RTC_BASE + 0x128)


__s32 DRAMC_init(__dram_para_t *para)
{
    __u32 reg_val;
	__u32 hold_flag = 0;
    __u8  reg_value;
    __s32 ret_val;  
    __u32 i;
    
//    hold_flag = mctl_read_w(SDR_DPCR);

	//check input dram parameter structure
    if(!para)
	{
    	//dram parameter is invalid
    	return 0;
	}
    
    //end PLL close 20130710
    //close AHB clock for DRAMC,--added by yeshaozhen at 20130708
    reg_val = mctl_read_w(DRAM_CCM_AHB_GATE_REG);
    reg_val &= ~(0x3<<14);
    mctl_write_w(DRAM_CCM_AHB_GATE_REG, reg_val);
	delay_us(10);
	//20130708 AHB close end
    
	//setup DRAM relative clock
	if(para->dram_tpr4 & 0x4)
		mctl_setup_dram_clock(para->dram_clk,1);	//PLL tunning enable
	else
		mctl_setup_dram_clock(para->dram_clk,0);	


	mctl_set_drive();

	//dram clock off
	DRAMC_clock_output_en(0);


	mctl_itm_disable();
	
	//if( (para->dram_clk) > 300)
		mctl_enable_dll0(para->dram_tpr3);
	//else
	//	mctl_disable_dll();

	//configure external DRAM
	reg_val = 0;
	if(para->dram_type == 3)
		reg_val |= 0x1;
	reg_val |= (para->dram_io_width>>3) <<1;

	if(para->dram_chip_density == 256)
		reg_val |= 0x0<<3;
	else if(para->dram_chip_density == 512)
		reg_val |= 0x1<<3;
	else if(para->dram_chip_density == 1024)
		reg_val |= 0x2<<3;
	else if(para->dram_chip_density == 2048)
		reg_val |= 0x3<<3;
	else if(para->dram_chip_density == 4096)
		reg_val |= 0x4<<3;
	else if(para->dram_chip_density == 8192)
		reg_val |= 0x5<<3;
	else
		reg_val |= 0x0<<3;
	reg_val |= ((para->dram_bus_width>>3) - 1)<<6;
	reg_val |= (para->dram_rank_num -1)<<10;
	reg_val |= 0x1<<12;
	reg_val |= ((0x1)&0x3)<<13;
	mctl_write_w(SDR_DCR, reg_val);

    //SDR_ZQCR1 set bit24 to 1
    reg_val  = mctl_read_w(SDR_ZQCR1);
    reg_val |= (0x1<<24) | (0x1<<1);
    if(para->dram_tpr4 & 0x2)
    {
    	reg_val &= ~((0x1<<24) | (0x1<<1));
    }    
    mctl_write_w(SDR_ZQCR1, reg_val);

  	//dram clock on
  	DRAMC_clock_output_en(1);
  	
    hold_flag = mctl_read_w(SDR_DPCR);
    if(hold_flag == 0) //normal branch
    {
	    //set odt impendance divide ratio
	    reg_val=((para->dram_zq)>>8)&0xfffff;
	    reg_val |= ((para->dram_zq)&0xff)<<20;
	    reg_val |= (para->dram_zq)&0xf0000000;
	    reg_val |= (0x1u<<31);
	    mctl_write_w(SDR_ZQCR0, reg_val);
	    
	    while( !((mctl_read_w(SDR_ZQSR)&(0x1u<<31))) );

	}
		    
	    //Set CKE Delay to about 1ms
	    reg_val = mctl_read_w(SDR_IDCR);
	    reg_val |= 0x1ffff;
	    mctl_write_w(SDR_IDCR, reg_val);
	
//      //dram clock on
//      DRAMC_clock_output_en(1);
    //reset external DRAM when CKE is Low
    //reg_val = mctl_read_w(SDR_DPCR);
    if(hold_flag == 0) //normal branch
	{
		//reset ddr3
		mctl_ddr3_reset();
	}
	else
	{
		//setup the DDR3 reset pin to high level
		reg_val = mctl_read_w(SDR_CR);
		reg_val |= (0x1<<12);
		mctl_write_w(SDR_CR, reg_val);
	}
	
	mctl_delay(0x10);
    while(mctl_read_w(SDR_CCR) & (0x1U<<31)) {};
    
	//if( (para->dram_clk) > 300)
    mctl_enable_dllx(para->dram_tpr3);
    //set I/O configure register
//    reg_val = 0x00cc0000;
//   reg_val |= (para->dram_odt_en)&0x3;
//  reg_val |= ((para->dram_odt_en)&0x3)<<30;
//    mctl_write_w(SDR_IOCR, reg_val);

	//set refresh period
	DRAMC_set_autorefresh_cycle(para->dram_clk);

	//set timing parameters
	mctl_write_w(SDR_TPR0, para->dram_tpr0);
	mctl_write_w(SDR_TPR1, para->dram_tpr1);
	mctl_write_w(SDR_TPR2, para->dram_tpr2);

	//set mode register
	if(para->dram_type==3)							//ddr3
	{
		reg_val = 0x1<<12;
		reg_val |= (para->dram_cas - 4)<<4;
		reg_val |= 0x5<<9;
	}
	else if(para->dram_type==2)					//ddr2
	{
		reg_val = 0x2;
		reg_val |= para->dram_cas<<4;
		reg_val |= 0x5<<9;
	}
	mctl_write_w(SDR_MR, reg_val);

    mctl_write_w(SDR_EMR, para->dram_emr1);
	mctl_write_w(SDR_EMR2, para->dram_emr2);
	mctl_write_w(SDR_EMR3, para->dram_emr3);

	//set DQS window mode
	reg_val = mctl_read_w(SDR_CCR);
	reg_val |= 0x1U<<14;
	reg_val &= ~(0x1U<<17);
		//2T & 1T mode 
	if(para->dram_tpr4 & 0x1)
	{
		reg_val |= 0x1<<5;
	}
	mctl_write_w(SDR_CCR, reg_val);

	//initial external DRAM
	reg_val = mctl_read_w(SDR_CCR);
	reg_val |= 0x1U<<31;
	mctl_write_w(SDR_CCR, reg_val);

	while(mctl_read_w(SDR_CCR) & (0x1U<<31)) {};
	
    //setup zq calibration manual
    //reg_val = mctl_read_w(SDR_DPCR);
    if(hold_flag == 1)
	{
        
		super_standby_flag = 1;
		
		    	//disable auto-fresh			//by cpl 2013-5-6
    	reg_val = mctl_read_w(SDR_DRR);
    	reg_val |= 0x1U<<31;
    	mctl_write_w(SDR_DRR, reg_val);

        reg_val = mctl_read_w(SDR_GP_REG0);
        reg_val &= 0x000fffff;
       // reg_val |= 0x17b00000;
        reg_val |= (0x1<<28) | (para->dram_zq<<20);
		mctl_write_w(SDR_ZQCR0, reg_val);
		
		//03-08
		reg_val = mctl_read_w(SDR_DCR);
		reg_val &= ~(0x1fU<<27);
		reg_val |= 0x12U<<27;
		mctl_write_w(SDR_DCR, reg_val);
		while( mctl_read_w(SDR_DCR)& (0x1U<<31) );
		
		mctl_delay(0x100);

		//dram pad hold off
		mctl_write_w(SDR_DPCR, 0x16510000);
		
		while(mctl_read_w(SDR_DPCR) & 0x1){}		
				
		//exit self-refresh state
		reg_val = mctl_read_w(SDR_DCR);
		reg_val &= ~(0x1fU<<27);
		reg_val |= 0x17U<<27;
		mctl_write_w(SDR_DCR, reg_val);
	
		//check whether command has been executed
		while( mctl_read_w(SDR_DCR)& (0x1U<<31) );
    	//disable auto-fresh			//by cpl 2013-5-6
    	reg_val = mctl_read_w(SDR_DRR);
    	reg_val &= ~(0x1U<<31);
    	mctl_write_w(SDR_DRR, reg_val);
		mctl_delay(0x100);;
	
//        //issue a refresh command
//        reg_val = mctl_read_w(SDR_DCR);
//        reg_val &= ~(0x1fU<<27);
//        reg_val |= 0x13U<<27;
//        mctl_write_w(SDR_DCR, reg_val);
//        
//        while( mctl_read_w(SDR_DCR)& (0x1U<<31) );
//
//        mem_delay(0x100);
	}

	//scan read pipe value
	mctl_itm_enable();

	if(hold_flag == 0)//normal branch
    {
		if(para->dram_tpr3 & (0x1U<<31))
		{
			ret_val = DRAMC_scan_dll_para();
			if(ret_val == 0)
			{	
				para->dram_tpr3 = 0x0;
				para->dram_tpr3 |= (((mctl_read_w(SDR_DLLCR0)>>6)&0x3f)<<16);
				para->dram_tpr3 |= (((mctl_read_w(SDR_DLLCR1)>>14)&0xf)<<0);
				para->dram_tpr3 |= (((mctl_read_w(SDR_DLLCR2)>>14)&0xf)<<4);
				para->dram_tpr3 |= (((mctl_read_w(SDR_DLLCR3)>>14)&0xf)<<8);
				para->dram_tpr3 |= (((mctl_read_w(SDR_DLLCR4)>>14)&0xf)<<12);
			}		
		}
		else
		{
			ret_val = DRAMC_scan_readpipe();
		}
	
		if(ret_val < 0)
		{
			return 0;
		}
	}
	else
	{
		//write back dqs gating value
        reg_val = mctl_read_w(SDR_GP_REG1);
        mctl_write_w(SDR_RSLR0, reg_val);

        reg_val = mctl_read_w(SDR_GP_REG2);
        mctl_write_w(SDR_RDQSGR, reg_val);
	}
	//configure all host port
	mctl_configure_hostport();

	return DRAMC_get_dram_size();
}

__s32 DRAMC_init_EX(__dram_para_t *para)
{
	__u32 i = 0;
	__s32 ret_val = 0;
	
	for(i=0; i<3; i++)
	{
	  ret_val = DRAMC_init(para);
	  if(ret_val) break;
	}
	
	return ret_val;
}

/*
*********************************************************************************************************
*                                   DRAM EXIT
*
* Description: dram exit;
*
* Arguments  : none;
*
* Returns    : result;
*
* Note       :
*********************************************************************************************************
*/
__s32 DRAMC_exit(void)
{
    return 0;
}

/*
*********************************************************************************************************
*                                   CHECK DDR READPIPE
*
* Description: check ddr readpipe;
*
* Arguments  : none
*
* Returns    : result, 0:fail, 1:success;
*
* Note       :
*********************************************************************************************************
*/
__s32 DRAMC_scan_readpipe(void)
{
	__u32 reg_val;

	//Clear Error Flag
	reg_val = mctl_read_w(SDR_CSR);
	reg_val &= ~(0x1<<20);
	mctl_write_w(SDR_CSR, reg_val);
	
	//data training trigger
	reg_val = mctl_read_w(SDR_CCR);
	reg_val |= 0x1<<30;
	mctl_write_w(SDR_CCR, reg_val);

	//check whether data training process is end
	while(mctl_read_w(SDR_CCR) & (0x1<<30)) {};

	//check data training result
	reg_val = mctl_read_w(SDR_CSR);
	if(reg_val & (0x1<<20))
	{
		return -1;
	}

	return (0);
}


/*
*********************************************************************************************************
*                                   SCAN DDR DLL Parameters
*
* Description: Scan DDR DLL Parameters;
*
* Arguments  : __dram_para_t
*
* Returns    : result, -1:fail, 0:success;
*
* Note       :
*********************************************************************************************************
*/
#define MCTL_DQS_DLY_COUNT     7
#define MCTL_CLK_DLY_COUNT     15
#define MCTL_SCAN_PASS		   0

__s32 DRAMC_scan_dll_para(void)
{
    const __u32 DQS_DLY[MCTL_DQS_DLY_COUNT] = {0x3, 0x2, 0x1, 0x0, 0xE, 0xD, 0xC};  //36~144
    const __u32 CLK_DLY[MCTL_CLK_DLY_COUNT] = {0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38};
    __u32 clk_dqs_count[MCTL_CLK_DLY_COUNT>MCTL_DQS_DLY_COUNT?MCTL_CLK_DLY_COUNT:MCTL_DQS_DLY_COUNT];
    __u32 dqs_i, clk_i;
    __u32 max_val, min_val;
    __u32 dqs_index, clk_index;

    //Find DQS_DLY Pass Count for every CLK_DLY
    for(clk_i = 0; clk_i < MCTL_CLK_DLY_COUNT; clk_i ++)
    {
    	clk_dqs_count[clk_i] = 0;
    	mctl_write_w(SDR_DLLCR0, (mctl_read_w(SDR_DLLCR0) & ~(0x3f<<6)) | ((CLK_DLY[clk_i] & 0x3f) << 6));
    	for(dqs_i = 0; dqs_i < MCTL_DQS_DLY_COUNT; dqs_i ++)
    	{
    		mctl_write_w(SDR_DLLCR1, (mctl_read_w(SDR_DLLCR1) & ~(0xf<<14)) | ((DQS_DLY[dqs_i] & 0xf) << 14));
    		mctl_write_w(SDR_DLLCR2, (mctl_read_w(SDR_DLLCR2) & ~(0xf<<14)) | ((DQS_DLY[dqs_i] & 0xf) << 14));
    		mctl_write_w(SDR_DLLCR3, (mctl_read_w(SDR_DLLCR3) & ~(0xf<<14)) | ((DQS_DLY[dqs_i] & 0xf) << 14));
    		mctl_write_w(SDR_DLLCR4, (mctl_read_w(SDR_DLLCR4) & ~(0xf<<14)) | ((DQS_DLY[dqs_i] & 0xf) << 14));
    		delay_us(1000);
			if(DRAMC_scan_readpipe()==MCTL_SCAN_PASS)
    		{
    			clk_dqs_count[clk_i] ++;
    		}
    	}
    }

    //Test DQS_DLY Pass Count for every CLK_DLY from up to down
    for(dqs_i = MCTL_DQS_DLY_COUNT; dqs_i > 0; dqs_i--)
    {
    	max_val = MCTL_CLK_DLY_COUNT;
    	min_val = MCTL_CLK_DLY_COUNT;
    	for(clk_i=0; clk_i<MCTL_CLK_DLY_COUNT; clk_i++)
    	{
    		if(clk_dqs_count[clk_i]==dqs_i)
    		{
    			max_val = clk_i;
    			if(min_val==MCTL_CLK_DLY_COUNT)
    			{
    				min_val = clk_i;
    			}
    		}
    	}
    	if(max_val < MCTL_CLK_DLY_COUNT)
    	{
    		break;
    	}
    }
    if(!dqs_i)
    {
    	//Fail to Find a CLK_DLY
    	mctl_write_w(SDR_DLLCR0, mctl_read_w(SDR_DLLCR0) & ~(0x3f<<6));
    	mctl_write_w(SDR_DLLCR1, mctl_read_w(SDR_DLLCR1) & ~(0xf<<14));
    	mctl_write_w(SDR_DLLCR2, mctl_read_w(SDR_DLLCR2) & ~(0xf<<14));
    	mctl_write_w(SDR_DLLCR3, mctl_read_w(SDR_DLLCR3) & ~(0xf<<14));
    	mctl_write_w(SDR_DLLCR4, mctl_read_w(SDR_DLLCR4) & ~(0xf<<14));
		delay_us(1000);
    	return DRAMC_scan_readpipe();
    }

    //Find the middle index of CLK_DLY
    clk_index = (max_val + min_val) >> 1;
    if((max_val == (MCTL_CLK_DLY_COUNT-1))&&(min_val>0))
    {
    	//if CLK_DLY[MCTL_CLK_DLY_COUNT] is very good, then the middle value can be more close to the max_val
    	clk_index = (MCTL_CLK_DLY_COUNT + clk_index) >> 1;
    }
    else if((max_val < (MCTL_CLK_DLY_COUNT-1))&&(min_val==0))
    {
    	//if CLK_DLY[0] is very good, then the middle value can be more close to the min_val
    	clk_index >>= 1;
    }
    if(clk_dqs_count[clk_index] < dqs_i)
    {
    	//if the middle value is not very good, can take any value of min_val/max_val
    	clk_index = min_val;
    }

    //Find the middle index of DQS_DLY for the CLK_DLY got above, and Scan read pipe again
    mctl_write_w(SDR_DLLCR0, (mctl_read_w(SDR_DLLCR0) & ~(0x3f<<6)) | ((CLK_DLY[clk_index] & 0x3f) << 6));
    max_val = MCTL_DQS_DLY_COUNT;
    min_val = MCTL_DQS_DLY_COUNT;
    for(dqs_i = 0; dqs_i < MCTL_DQS_DLY_COUNT; dqs_i++)
    {
    	clk_dqs_count[dqs_i] = 0;
    	mctl_write_w(SDR_DLLCR1, (mctl_read_w(SDR_DLLCR1) & ~(0xf<<14)) | ((DQS_DLY[dqs_i] & 0xf) << 14));
    	mctl_write_w(SDR_DLLCR2, (mctl_read_w(SDR_DLLCR2) & ~(0xf<<14)) | ((DQS_DLY[dqs_i] & 0xf) << 14));
    	mctl_write_w(SDR_DLLCR3, (mctl_read_w(SDR_DLLCR3) & ~(0xf<<14)) | ((DQS_DLY[dqs_i] & 0xf) << 14));
    	mctl_write_w(SDR_DLLCR4, (mctl_read_w(SDR_DLLCR4) & ~(0xf<<14)) | ((DQS_DLY[dqs_i] & 0xf) << 14));
		delay_us(1000);
    	if(DRAMC_scan_readpipe()==MCTL_SCAN_PASS)
    	{
    		clk_dqs_count[dqs_i] = 1;
    		max_val = dqs_i;
    		if(min_val == MCTL_DQS_DLY_COUNT) min_val = dqs_i;
    	}
    }
    if(max_val < MCTL_DQS_DLY_COUNT)
    {
    	dqs_index = (max_val + min_val) >> 1;
    	if((max_val == (MCTL_DQS_DLY_COUNT-1))&&(min_val>0))
    	{
    	   	//if DQS_DLY[MCTL_DQS_DLY_COUNT-1] is very good, then the middle value can be more close to the max_val
    		dqs_index = (MCTL_DQS_DLY_COUNT + dqs_index) >> 1;
    	}
    	else if((max_val < (MCTL_DQS_DLY_COUNT-1))&&(min_val==0))
    	{
    	   	//if DQS_DLY[0] is very good, then the middle value can be more close to the min_val
    		dqs_index >>= 1;
    	}
    	if(!clk_dqs_count[dqs_index])
    	{
    		//if the middle value is not very good, can take any value of min_val/max_val
    		dqs_index = min_val;
    	}
    	mctl_write_w(SDR_DLLCR1, (mctl_read_w(SDR_DLLCR1) & ~(0xf<<14)) | ((DQS_DLY[dqs_index] & 0xf) << 14));
    	mctl_write_w(SDR_DLLCR2, (mctl_read_w(SDR_DLLCR2) & ~(0xf<<14)) | ((DQS_DLY[dqs_index] & 0xf) << 14));
    	mctl_write_w(SDR_DLLCR3, (mctl_read_w(SDR_DLLCR3) & ~(0xf<<14)) | ((DQS_DLY[dqs_index] & 0xf) << 14));
    	mctl_write_w(SDR_DLLCR4, (mctl_read_w(SDR_DLLCR4) & ~(0xf<<14)) | ((DQS_DLY[dqs_index] & 0xf) << 14));
		delay_us(1000);
    	return DRAMC_scan_readpipe();
    }
    else
    {
    	//Fail to Find a DQS_DLY
    	mctl_write_w(SDR_DLLCR0, mctl_read_w(SDR_DLLCR0) & ~(0x3f<<6));
    	mctl_write_w(SDR_DLLCR1, mctl_read_w(SDR_DLLCR1) & ~(0xf<<14));
    	mctl_write_w(SDR_DLLCR2, mctl_read_w(SDR_DLLCR2) & ~(0xf<<14));
    	mctl_write_w(SDR_DLLCR3, mctl_read_w(SDR_DLLCR3) & ~(0xf<<14));
    	mctl_write_w(SDR_DLLCR4, mctl_read_w(SDR_DLLCR4) & ~(0xf<<14));
		delay_us(1000);
    	return DRAMC_scan_readpipe();
    }	
}

/*
*********************************************************************************************************
*                                   DRAM SCAN READ PIPE
*
* Description: dram scan read pipe
*
* Arguments  : none
*
* Returns    : result, 0:fail, 1:success;
*
* Note       :
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                   DRAM CLOCK CONTROL
*
* Description: dram get clock
*
* Arguments  : on   dram clock output (0: disable, 1: enable)
*
* Returns    : none
*
* Note       :
*********************************************************************************************************
*/
void DRAMC_clock_output_en(__u32 on)
{
    __u32 reg_val;

    reg_val = mctl_read_w(SDR_CR);

    if(on)
        reg_val |= 0x1<<16;
    else
        reg_val &= ~(0x1<<16);

    mctl_write_w(SDR_CR, reg_val);
}
/*
*********************************************************************************************************
* Description: Set autorefresh cycle
*
* Arguments  : clock value in MHz unit
*
* Returns    : none
*
* Note       :
*********************************************************************************************************
*/
void DRAMC_set_autorefresh_cycle(__u32 clk)
{
	__u32 reg_val;
//	__u32 dram_size;
	__u32 tmp_val;

//	dram_size = mctl_read_w(SDR_DCR);
//	dram_size >>=3;
//	dram_size &= 0x7;

//	if(clk < 600)
	{
//		if(dram_size<=0x2)
//			tmp_val = (131*clk)>>10;
//		else
//			tmp_val = (336*clk)>>10;
		reg_val = 0x83;
		tmp_val = (7987*clk)>>10;
		tmp_val = tmp_val*9 - 200;
		reg_val |= tmp_val<<8;
		reg_val |= 0x8<<24;
		mctl_write_w(SDR_DRR, reg_val);
	}
//	else
//   {
//		mctl_write_w(SDR_DRR, 0x0);
//   }
}


/*
**********************************************************************************************************************
*                                               GET DRAM SIZE
*
* Description: Get DRAM Size in MB unit;
*
* Arguments  : None
*
* Returns    : 32/64/128
*
* Notes      :
*
**********************************************************************************************************************
*/
__u32 DRAMC_get_dram_size(void)
{
    __u32 reg_val;
    __u32 dram_size;
    __u32 chip_den;

    reg_val = mctl_read_w(SDR_DCR);
    chip_den = (reg_val>>3)&0x7;
    if(chip_den == 0)
    	dram_size = 32;
    else if(chip_den == 1)
    	dram_size = 64;
    else if(chip_den == 2)
    	dram_size = 128;
    else if(chip_den == 3)
    	dram_size = 256;
    else if(chip_den == 4)
    	dram_size = 512;
    else
    	dram_size = 1024;

    if( ((reg_val>>1)&0x3) == 0x1)
    	dram_size<<=1;
    if( ((reg_val>>6)&0x7) == 0x3)
    	dram_size<<=1;
    if( ((reg_val>>10)&0x3) == 0x1)
    	dram_size<<=1;

    return dram_size;
}
#if SYS_STORAGE_MEDIA_TYPE == SYS_STORAGE_MEDIA_SD_CARD
__s32 DRAMC_to_card_init(__dram_para_t *para)
{
	__u32 i, m;
	__u32 dram_den;
	__u32 err_flag = 0;
	__u32 start_adr, end_adr;
	__u32 bonding_id;
	__u32 reg_val;
	
	
	//reset the controller logic------------------added by YeShaozhen at 20130707 for custonmer problem
	//close it first
	reg_val = mctl_read_w(DRAM_CCM_AHB_GATE_REG);
	reg_val &= ~(0x3<<14);
	mctl_write_w(DRAM_CCM_AHB_GATE_REG, reg_val);
	delay_us(10);
	//then open it
	reg_val |= 0x3<<14;
	mctl_write_w(DRAM_CCM_AHB_GATE_REG, reg_val);
	delay_us(10);
	//reset controller logic end

	//check whether scan dram size
	//below scan dram parameter

	//get bonding ID
	reg_val = mctl_read_w(DRAM_CCM_AHB_GATE_REG);
	reg_val |= 0x1<<5;
	mctl_write_w(DRAM_CCM_AHB_GATE_REG, reg_val);
	mctl_write_w(DRAM_CCM_SS_CLK_REG, 0x80000000);

	reg_val = mctl_read_w(DRAM_SS_CTRL_REG);
	bonding_id = (reg_val >>16)&0x7;

	//try 16/32 bus width
	para->dram_type = 3;
	para->dram_rank_num = 1;
	para->dram_chip_density = 4096;
	para->dram_io_width	= 16;
	para->dram_bus_width = 32;
	if(!DRAMC_init_EX(para))
	{
		para->dram_bus_width = 16;
	}

	//try 1/2 rank
	if(bonding_id == 0x1)
	{
		para->dram_rank_num = 1;
	}
	else
	{
		para->dram_rank_num = 2;
	    if(!DRAMC_init_EX(para))
		{
			para->dram_rank_num = 1;
		}
	}
	//try 2048/4096 chip density
	para->dram_chip_density = 8192;
//	dram_den = para->dram_chip_density<<17;
//	if(para->dram_rank_num == 2)
//		dram_den<<=1;
//	if(para->dram_bus_width == 32)
//		dram_den<<=1;
	if(!DRAMC_init_EX(para))
	{
		return 0;
	}
	
#if 0
	//write preset value at special address
	start_adr = 0x40000000;
	end_adr = 0x40000000 + dram_den;
	for(m = start_adr; m< end_adr; m+=0x4000000)
	{
		for(i=0; i<16; i++)
		{
			*((__u32 *)(m + i*4)) = m + i;
		}
	}
	//read and check value at special address
	err_flag = 0;
	for(m = start_adr; m< end_adr; m+=0x4000000)
	{
		for(i=0; i<16; i++)
		{
			if( *((__u32 *)(m + i*4)) != (m + i) )
			{
				err_flag = 1;
				break;
			}

		}
	}
	if(!err_flag)
	{
	    para->dram_size = dram_den>>20;
		return para->dram_size;
	}

	para->dram_chip_density = 2048;
	if(!DRAMC_init_EX(para))
	{
		return 0;
	}

	para->dram_size = dram_den>>21;
#else
	for(m =0x50000000; m<0xC0000000; m+=0x10000000)
	{
		for(i=0; i<32; i++)
		{
			if(mctl_read_w(m + i*4) != mctl_read_w(0x40000000 + i*4))
				break;
		}
		if(i == 32)
		{
			err_flag = 1;			
			para->dram_size = (m - 0x40000000)>>20;		
			para->dram_chip_density = (m - 0x40000000)>>17;	
			if(para->dram_bus_width == 32)
				para->dram_chip_density>>=1;
			break;
			
		}
	}
	
	if(m == 0xC0000000)
	{
		para->dram_size = 2048;
		para->dram_chip_density = 8192;
		
	}	
	
	reg_val = mctl_read_w(SDR_DCR);
	reg_val &= ~(0x7<<3);
	if(para->dram_chip_density == 256)
		reg_val |= 0x0<<3;
	else if(para->dram_chip_density == 512)
		reg_val |= 0x1<<3;
	else if(para->dram_chip_density == 1024)
		reg_val |= 0x2<<3;
	else if(para->dram_chip_density == 2048)
		reg_val |= 0x3<<3;
	else if(para->dram_chip_density == 4096)
		reg_val |= 0x4<<3;
	else if(para->dram_chip_density == 8192)
		reg_val |= 0x5<<3;
	else
		reg_val |= 0x0<<3;
	mctl_write_w(SDR_DCR, reg_val);
	
	mctl_configure_hostport();
#endif
	return para->dram_size;
}
#endif

//*****************************************************************************add for freq jump at 20120628
void mem_delay(unsigned int count)
{
	unsigned int i=0;

	for(i=0; i<count; i++)
		continue;
}

//*****************************************************************************
//	void mctl_self_refresh_entry()
//  Description:	Enter into self refresh state
//
//	Arguments:		None
//
//	Return Value:	None
//*****************************************************************************
void mctl_self_refresh_entry(void)
{
	__u32 reg_val;

	//disable auto refresh
	reg_val = mctl_read_w(SDR_DRR);
	reg_val |= 0x1U<<31;
	mctl_write_w(SDR_DRR, reg_val);

	reg_val = mctl_read_w(SDR_DCR);
	reg_val &= ~(0x1fU<<27);
	reg_val |= 0x12U<<27;
	mctl_write_w(SDR_DCR, reg_val);

	//check whether command has been executed
	while( mctl_read_w(SDR_DCR)& (0x1U<<31) );
	mem_delay(1000);
}

//**************************************************
void setup_dram_pll(__u32 clk)
{
    __u32 reg_val;

    //setup DRAM PLL
    reg_val = mctl_read_w(DRAM_CCM_SDRAM_PLL_REG);
    reg_val &= ~(1U<<31);
    mctl_write_w(DRAM_CCM_SDRAM_PLL_REG, reg_val);		//disable PLL
    
    mem_delay(10000);

    reg_val &= ~0x3;
    reg_val |= 0x1;                                             //m factor
    reg_val &= ~(0x3<<4);
    reg_val |= 0x1<<4;                                          //k factor
    reg_val &= ~(0x1f<<8);
    reg_val |= ((clk/24)&0x1f)<<8;                              //n factor
    reg_val &= ~(0x3<<16);
    reg_val |= 0x1<<16;                                         //p factor
    reg_val &= ~(0x1<<29);                                      //clock output disable
    reg_val |= (__u32)0x1<<31;                                  //PLL En
    mctl_write_w(DRAM_CCM_SDRAM_PLL_REG, reg_val);
    //mctl_delay(0x100000);
    mem_delay(10000);
    reg_val = mctl_read_w(DRAM_CCM_SDRAM_PLL_REG);
    reg_val |= 0x1<<29;
    mctl_write_w(DRAM_CCM_SDRAM_PLL_REG, reg_val);
    
    mem_delay(10000);

}
//******************************************************
//	dram_freq_jum(unsigned char);
//	used for frequency jump, add by yeshaozhen at 20130628
//******************************************************
__s32	dram_freq_jum(unsigned char freq_p,__dram_para_t *dram_para)
{
	__u32 reg_val = 0;
	
	if((dram_para->dram_tpr4 & 0x8) == 0)	//freq jump not allow
		return 0;
		
	if(freq_p == 0)	//the first calibration frequency point
	{
			mctl_self_refresh_entry();	//DRAMC_enter_selfrefresh();
			//turn off DLL
			mctl_disable_dll();
			DRAMC_clock_output_en(0);	//dram clock off

			setup_dram_pll(120);
			mctl_itm_disable();
			mctl_enable_dll0(dram_para->dram_tpr3);
			mctl_enable_dllx(dram_para->dram_tpr3);

			DRAMC_clock_output_en(1);	//dram clock on

			//set refresh period
			DRAMC_set_autorefresh_cycle(120);

			reg_val = mctl_read_w(SDR_DRR);
			reg_val |= 0x1U<<31;
			mctl_write_w(SDR_DRR, reg_val);

			//03-08
			reg_val = mctl_read_w(SDR_DCR);
			reg_val &= ~(0x1fU<<27);
			reg_val |= 0x12U<<27;
			mctl_write_w(SDR_DCR, reg_val);
			while( mctl_read_w(SDR_DCR)& (0x1U<<31) );

			mem_delay(0x100);

			//exit self-refresh state
			reg_val = mctl_read_w(SDR_DCR);
			reg_val &= ~(0x1fU<<27);
			reg_val |= 0x17U<<27;
			mctl_write_w(SDR_DCR, reg_val);

			//check whether command has been executed
			while( mctl_read_w(SDR_DCR)& (0x1U<<31) );

			//enable auto-fresh			//by cpl 2013-5-6
			reg_val = mctl_read_w(SDR_DRR);
			reg_val &= ~(0x1U<<31);
			mctl_write_w(SDR_DRR, reg_val);

			mctl_itm_enable();
			//write back dqs gating value
			mctl_write_w(SDR_RSLR0, (dram_para->dram_tpr5) & 0xfff);	//dqs_value_save_120[0]
			mctl_write_w(SDR_RDQSGR,((dram_para->dram_tpr5)>>16) & 0xff);	// dqs_value_save_120[1]);		
	}
	else if(freq_p == 1)
	{
			mctl_self_refresh_entry();//DRAMC_enter_selfrefresh();

			mctl_disable_dll();
			DRAMC_clock_output_en(0);	//dram clock off

			setup_dram_pll(dram_para->dram_clk);
			mctl_itm_disable();
			mctl_enable_dll0(dram_para->dram_tpr3);
			mctl_enable_dllx(dram_para->dram_tpr3);

//			//dram clock on
			DRAMC_clock_output_en(1);

			//set refresh period
			DRAMC_set_autorefresh_cycle(dram_para->dram_clk);

		  	//disable auto-fresh			//by cpl 2013-5-6
			reg_val = mctl_read_w(SDR_DRR);
			reg_val |= 0x1U<<31;
			mctl_write_w(SDR_DRR, reg_val);

			//03-08
			reg_val = mctl_read_w(SDR_DCR);
			reg_val &= ~(0x1fU<<27);
			reg_val |= 0x12U<<27;
			mctl_write_w(SDR_DCR, reg_val);
			while( mctl_read_w(SDR_DCR)& (0x1U<<31) );

			mem_delay(0x100);

			//exit self-refresh state
			reg_val = mctl_read_w(SDR_DCR);
			reg_val &= ~(0x1fU<<27);
			reg_val |= 0x17U<<27;
			mctl_write_w(SDR_DCR, reg_val);

			//check whether command has been executed
			while( mctl_read_w(SDR_DCR)& (0x1U<<31) );

			//enable auto-fresh			//by cpl 2013-5-6
			reg_val = mctl_read_w(SDR_DRR);
			reg_val &= ~(0x1U<<31);
			mctl_write_w(SDR_DRR, reg_val);

			mctl_itm_enable();
			//write back dqs gating value
      reg_val = mctl_read_w(SDR_GP_REG1);
      mctl_write_w(SDR_RSLR0, reg_val);

      reg_val = mctl_read_w(SDR_GP_REG2);
      mctl_write_w(SDR_RDQSGR, reg_val);
	}
}
//*****************************************************************************end of freq jump at 20120628


__s32 init_DRAM(int type)
{
	__u32 i;
	__u32 temp_data_value,reg_val;
	__s32 ret_val;
	boot_dram_para_t  boot0_para;
    msg("dram driver version: %s\n",dram_driver_version);
	get_boot0_dram_para( &boot0_para );
 //   print_boot0_dram_para(&boot0_para);
	if(boot0_para.dram_clk > 2000)
	{
		boot0_para.dram_clk /= 1000000;
	}

	ret_val = 0;
	i = 0;
    
    super_standby_flag = 0;
	while( (ret_val == 0) && (i<4) )
	{
      
#if SYS_STORAGE_MEDIA_TYPE == SYS_STORAGE_MEDIA_NAND_FLASH
		//add by yeshaozhen at 20130628, used for frequency jump
		if(boot0_para.dram_tpr4 & 0x8)	//120M initial
		{
				temp_data_value				= boot0_para.dram_clk;
				boot0_para.dram_clk 	= 120;
				ret_val 							= DRAMC_init( &boot0_para );

//__asm__ __volatile__ ("cmp r0,r0");
//__asm__ __volatile__ ("beq .");
//__asm{cmp r0,r0};
//__asm{beq .}

//				{
//					int a=1;
//					while(a ==1);
//				}
//
//				for(i=0;i<0x8000000;i++)
//					mctl_write_w(0x40000000+(i<<2),i+1);
//					
//				for(i=0;i<0x8000000;i++)
//					if( (i+1) != mctl_read_w(0x40000000+(i<<2)))
//						ret_val = 0;
				
				if(ret_val==0)	//initial failed
					return 0;
					
				//save dqs gating value
  			boot0_para.dram_tpr5 = mctl_read_w(SDR_RSLR0) & 0xfff;
  			boot0_para.dram_tpr5 |=  ((mctl_read_w(SDR_RDQSGR) & 0xff) <<16);
  			
  			mctl_ddr3_reset();
  			//reset the controller logic
  			//close it first
		    reg_val = mctl_read_w(DRAM_CCM_AHB_GATE_REG);
		    reg_val &= ~(0x3<<14);
		    mctl_write_w(DRAM_CCM_AHB_GATE_REG, reg_val);
				delay_us(10);
    		//then open it
    		reg_val |= 0x3<<14;
    		mctl_write_w(DRAM_CCM_AHB_GATE_REG, reg_val);
				delay_us(10);
  			
  			boot0_para.dram_clk 	= temp_data_value;	//set the freq to user define
  			ret_val = 0;
		}
		ret_val = DRAMC_init( &boot0_para );
		
		reg_val = mctl_read_w(SDR_RSLR0);	//added by yeshaozhen at 20130725
		mctl_write_w(SDR_GP_REG1, reg_val);//added by yeshaozhen at 20130725
		reg_val = mctl_read_w(SDR_RDQSGR);//added by yeshaozhen at 20130725
		mctl_write_w(SDR_GP_REG2, reg_val);//added by yeshaozhen at 20130725
		set_boot0_dram_para( &boot0_para );//added by yeshaozhen at 20130725
		
#elif SYS_STORAGE_MEDIA_TYPE == SYS_STORAGE_MEDIA_SD_CARD
		if(!type)
		{
			ret_val = DRAMC_to_card_init( &boot0_para );
		}
		else
		{
			ret_val = DRAMC_init( &boot0_para );
		}
		set_boot0_dram_para( &boot0_para );
#else
#error  The storage media of Boot1 has not been defined.
#endif

		i++;
	}
	
//    print_boot0_dram_para(&boot0_para);
	return ret_val;
}

#pragma arm section

