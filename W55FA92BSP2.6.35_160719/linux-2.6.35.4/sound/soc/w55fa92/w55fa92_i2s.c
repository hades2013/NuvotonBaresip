/*
 * Copyright (c) 2009-2010 Nuvoton technology corporation.
 *
 * Wan ZongShun <mcuos.com@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation;version 2 of the License.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/suspend.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <linux/device.h>
#include <linux/clk.h>

//#include <mach/mfp.h>
#include <mach/map.h>
#include <mach/regs-clock.h>

#include <sound/pcm_params.h>
#include <mach/hardware.h>

#include <mach/w55fa92_audio.h>
#include <mach/w55fa92_i2s.h>
#include <mach/w55fa92_reg.h>


//#define OPT_FPGA_DEBUG

//#define I2S_DEBUG
#define I2S_DEBUG_ENTER_LEAVE
#define I2S_DEBUG_MSG
#define I2S_DEBUG_MSG2

#ifdef I2S_DEBUG
#define DBG(fmt, arg...)			printk(fmt, ##arg)
#else
#define DBG(fmt, arg...)
#endif


#ifdef I2S_DEBUG_ENTER_LEAVE
#define ENTER()					DBG("[%-10s] : Enter\n", __FUNCTION__)
#define LEAVE()					DBG("[%-10s] : Leave\n", __FUNCTION__)
#else
#define ENTER()
#define LEAVE()
#endif


static DEFINE_MUTEX(i2s_mutex);
struct w55fa92_audio *w55fa92_i2s_data;
extern unsigned int w55fa92_apll_clock;
extern unsigned int w55fa92_upll_clock;
extern int w55fa92_set_apll_clock(unsigned int clock);

static struct clk *clk_hclk4 = NULL;

extern void nvt_lock(void);
extern void nvt_unlock(void);

static int w55fa92_i2s_set_fmt(struct snd_soc_dai *cpu_dai,
                              unsigned int fmt)
{
 //       struct w55fa92_audio *w55fa92_audio = w55fa92_i2s_data;
        unsigned long val = 0;

		ENTER();

		val = AUDIO_READ(REG_I2S_ACTL_I2SCON);
		
        switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
        case SND_SOC_DAIFMT_MSB:
                val |= MSB_Justified;
                break;
        case SND_SOC_DAIFMT_I2S:
                val &= ~MSB_Justified;
                break;
        default:
                return -EINVAL;
        }

        AUDIO_WRITE(REG_I2S_ACTL_I2SCON, val);

		LEAVE();
		
        return 0;
}

static int w55fa92_i2s_set_sysclk(struct snd_soc_dai *cpu_dai,
                                 int clk_id, unsigned int freq, int dir)
{
        unsigned int val, ii, u32Divider0;
  //      struct w55fa92_audio *w55fa92_audio = w55fa92_i2s_data;
		unsigned int PllFreq;
		unsigned int u32MCLK, u32ClockDivider;

		ENTER();

        if (clk_id == W55FA92_AUDIO_SAMPLECLK) 
        {
	#if 1 	// MCLK = 256*WS        	
	printk("freq = %d !!!\n", freq);
			switch (freq)	//all 16bit, 256fs
			{
				case AU_SAMPLE_RATE_8000:						//8KHz
					w55fa92_set_apll_clock(208896);
					u32MCLK = 12288/6;									
					break;
				case AU_SAMPLE_RATE_11025:						//11.025KHz
					w55fa92_set_apll_clock(169344);		
					u32MCLK = 16934/6;							
					break;
				case AU_SAMPLE_RATE_12000:						//12KHz
					w55fa92_set_apll_clock(208896);
					u32MCLK = 12288/4;
					break;
				case AU_SAMPLE_RATE_16000:						//16KHz
					w55fa92_set_apll_clock(208896);
					u32MCLK = 12288/3;
					break;
				case AU_SAMPLE_RATE_22050:						//22.05KHz
					w55fa92_set_apll_clock(169344);		
					u32MCLK = 16934/3;
					break;
				case AU_SAMPLE_RATE_24000:						//24KHz
					w55fa92_set_apll_clock(208896);
					u32MCLK = 12288/2;
					break;
				case AU_SAMPLE_RATE_32000:						//32KHz
					w55fa92_set_apll_clock(147456);
					u32MCLK = 16384/2;									
					break;
				case AU_SAMPLE_RATE_44100:						//44.1KHz
					w55fa92_set_apll_clock(169344);		
					u32MCLK = 16934*2/3;
					break;
				case AU_SAMPLE_RATE_48000:						//48KHz
					w55fa92_set_apll_clock(208896);
					u32MCLK = 12288;
					break;
				case AU_SAMPLE_RATE_64000:						//64KHz
					w55fa92_set_apll_clock(147456);
					u32MCLK = 16384;
					break;
				case AU_SAMPLE_RATE_88200:						//88.2KHz
					w55fa92_set_apll_clock(135475);		
					u32MCLK = 16934*4/3;
					break;
				case AU_SAMPLE_RATE_96000:						//96KHz
					w55fa92_set_apll_clock(147456);
					u32MCLK = 12288*2;
					break;
			//	case AU_SAMPLE_RATE_19200:						//192KHz
				default:		
					w55fa92_set_apll_clock(147456);
					u32MCLK = 12288*2*2;
					break;
					
			}
			val = AUDIO_READ(REG_I2S_ACTL_I2SCON) & 0x08;	
			AUDIO_WRITE(REG_I2S_ACTL_I2SCON, val);			
			
			PllFreq = w55fa92_apll_clock;
			u32ClockDivider = PllFreq / u32MCLK;
			
			for (ii=8; ii>1; ii--)
			{
				if (!(u32ClockDivider%ii))
					break;
			}			
			{
				u32Divider0 = ii;
				u32ClockDivider = u32ClockDivider / ii;
			}			
				
			u32Divider0 --;				
			u32ClockDivider	--;			

			nvt_lock();
			AUDIO_WRITE(REG_CLKDIV7, (AUDIO_READ(REG_CLKDIV7) & (~I2S_S)) | (0x02 << 11) );	// SPU clock from APLL	
			AUDIO_WRITE(REG_CLKDIV7, (AUDIO_READ(REG_CLKDIV7) & (~I2S_N0)) | (u32Divider0<<8));			
			AUDIO_WRITE(REG_CLKDIV7, (AUDIO_READ(REG_CLKDIV7) & (~I2S_N1)) | (u32ClockDivider<<16));		
			nvt_unlock();		
			
    #else    	
			switch (freq)	//all 16bit, 256fs
			{
                case 8000:							//8KHz (12.288/6)
                        val |= FS_256 | BCLK_32 | SCALE_6;
                        break;
                case 11025:							//11.025KHz(16.934/6)
                        val |= FS_256 | BCLK_32 | SCALE_6;
                        break;
                case 12000:							//12.000KHz(12.288/4)
                        val |= FS_256 | BCLK_32 | SCALE_4;
                        break;
                case 16000:							//16KHz(12.288/3)
                        val |= FS_256 | BCLK_32 | SCALE_3;
                        break;
                case 22050:							//22.05KHz(16.934/3)
                        val |= FS_256 | BCLK_32 | SCALE_3;
                        break;
                case 24000:							//24KHz(12.288/2)
                        val |= FS_256 | BCLK_32 | SCALE_2;
                        break;
                case 32000:							//32KHz(12.288/1)
                        val |= FS_384 | BCLK_32 | SCALE_1;
                        break;
                case 44100:							//44.1KHz(16.9344/1)
                        val |= FS_384 | BCLK_32 | SCALE_1;
                        break;
                case 48000:							//48KHz(12.288/1)
                        val |= FS_256 | BCLK_32 | SCALE_1;
                        break;
                case 88200:							//88.2KHz(16.9344/1)
                        val |= FS_384 | BCLK_32 | SCALE_1;
                        break;
                case 96000:							//96KHz(12.288/1)
                        val |= FS_256 | BCLK_32 | SCALE_1;
				default:		
                        val |= FS_256 | BCLK_32 | SCALE_1;
					break;
			}
			
			AUDIO_WRITE(REG_I2S_ACTL_I2SCON, val);			

			if ( (freq == AU_SAMPLE_RATE_48000) ||
				 (freq == AU_SAMPLE_RATE_32000) ||
				 (freq == AU_SAMPLE_RATE_24000) ||
				 (freq == AU_SAMPLE_RATE_16000) ||
				 (freq == AU_SAMPLE_RATE_12000) ||
				 (freq == AU_SAMPLE_RATE_8000) )
			{
				w55fa92_set_apll_clock(208896);
				u32MCLK = 12288;									
			}
			else if ( ( freq == AU_SAMPLE_RATE_64000) ||
				 (freq == AU_SAMPLE_RATE_96000) )
			{
				w55fa92_set_apll_clock(147456);
				u32MCLK = 24576;									
			}
			else if ( freq == AU_SAMPLE_RATE_88200 )
			{
				w55fa92_set_apll_clock(169344);		
				u32MCLK = 16934*2;							
			}
			else
			{
				w55fa92_set_apll_clock(169344);		
				u32MCLK = 16934;							
			}		

		#ifdef OPT_FPGA_DEBUG	
		
			PllFreq = 27000;
			u32ClockDivider = PllFreq / u32MCLK;
			printk("==>u32ClockDivider=0x%x\n", u32ClockDivider);			
		
			AUDIO_WRITE(REG_CLKDIV1, AUDIO_READ(REG_CLKDIV1) & (~ADO_N1));		
			u32ClockDivider &= 0xFF;
			u32ClockDivider	--;
			AUDIO_WRITE(REG_CLKDIV1, AUDIO_READ(REG_CLKDIV1) | (u32ClockDivider<<24));	
		
		#else
	
			PllFreq = w55fa92_apll_clock;
			u32ClockDivider = PllFreq / u32MCLK;
			
			// clock_in of CLKDIV1_N1 divider must be less than 60 MHz
			for (ii=8; ii>1; ii--)
			{
				if (!(u32ClockDivider%ii))
					break;
			}			
	
	//		if (ii==1)
	//			printk("SampleRate not support\n");		
	//		else
			{
				u32Divider0 = ii;
				u32ClockDivider = u32ClockDivider / ii;
			}			
				
			u32Divider0 --;				
			u32ClockDivider	--;			

			nvt_lock();
			AUDIO_WRITE(REG_CLKDIV7, (AUDIO_READ(REG_CLKDIV7) & (~I2S_S)) | (0x02 << 11) );	// SPU clock from APLL	
			AUDIO_WRITE(REG_CLKDIV7, (AUDIO_READ(REG_CLKDIV7) & (~I2S_N0)) | (u32Divider0<<8));			
			AUDIO_WRITE(REG_CLKDIV7, (AUDIO_READ(REG_CLKDIV7) & (~I2S_N1)) | (u32ClockDivider<<16));		
			nvt_unlock();		
		#endif			
	#endif
	
	
		}	// clk_id == W55FA92_AUDIO_SAMPLECLK
		
        if (clk_id == W55FA92_AUDIO_CLKDIV) {
			// use PLL1 to generate 12.288MHz ,16.934MHz or 11.285Mhz for I2S
            // input source clock is 15Mhz
                
#if 0                
                if (freq%8000 == 0  && (freq != 32000)) {
                        //(PLL1=122.88MHz / ACKDIV=10) = 12.288MHz
                        AUDIO_WRITE(REG_PLLCON1,0x92E7);
                        AUDIO_WRITE(REG_CLKDIV,(AUDIO_READ(REG_CLKDIV) & ~(0xF<<8)) | (9<<8)); //   /10
                } else if (freq == 44100) {
                        //(PLL1=169.34MHz / ACKDIV=15) = 11.289MHz
                        AUDIO_WRITE(REG_PLLCON1, 0x4E25);
                        AUDIO_WRITE(REG_CLKDIV,(AUDIO_READ(REG_CLKDIV) & ~(0xF<<8)) | (14<<8)); //   /15
                } else {
                        //(PLL1=169.34MHz / ACKDIV=10) = 16.934MHz
                        AUDIO_WRITE(REG_PLLCON1,0x4E25);
                        AUDIO_WRITE(REG_CLKDIV,(AUDIO_READ(REG_CLKDIV) & ~(0xF<<8)) | (9<<8));	// /10
                }

                AUDIO_WRITE(REG_CLKSEL,(AUDIO_READ(REG_CLKSEL)&~(3<<4)) | (1<<4));//ACLK from PLL1
#endif                
        }

		LEAVE();
        return 0;
}

static int w55fa92_i2s_trigger(struct snd_pcm_substream *substream,
                              int cmd, struct snd_soc_dai *dai)
{
   //     struct w55fa92_audio *w55fa92_audio = w55fa92_i2s_data;
        int ret = 0;
        unsigned long val, con;

		ENTER();
		
        con = AUDIO_READ(REG_I2S_ACTL_CON);

        switch (cmd) {
        case SNDRV_PCM_TRIGGER_START:
        case SNDRV_PCM_TRIGGER_RESUME:

                val = AUDIO_READ(REG_I2S_ACTL_RESET);
                con |= I2S_EN;
                if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
     				val |= I2S_PLAY;         
                	con |= P_DMA_IRQ_EN; 
                } 
                else
                {
                    val |= I2S_RECORD;
               		con |= R_DMA_IRQ_EN;
                }
                AUDIO_WRITE(REG_I2S_ACTL_RESET, val);
                
                AUDIO_WRITE(REG_I2S_ACTL_RSR, AUDIO_READ(REG_I2S_ACTL_RSR));                
                AUDIO_WRITE(REG_I2S_ACTL_PSR, AUDIO_READ(REG_I2S_ACTL_PSR));                                
                AUDIO_WRITE(REG_I2S_ACTL_CON, con);
				AUDIO_WRITE(REG_AIC_MECR,(1<<IRQ_I2S));                
               break;
               
        case SNDRV_PCM_TRIGGER_STOP:
        case SNDRV_PCM_TRIGGER_SUSPEND:
                val = AUDIO_READ(REG_I2S_ACTL_RESET);
                con &= ~I2S_EN;
                if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
                		AUDIO_WRITE(REG_I2S_ACTL_PSR, AUDIO_READ(REG_I2S_ACTL_PSR));                                	                	
                        val &= ~I2S_PLAY;
                } else {
                		AUDIO_WRITE(REG_I2S_ACTL_RSR, AUDIO_READ(REG_I2S_ACTL_RSR));                                	
                        val &= ~I2S_RECORD;
                }

                AUDIO_WRITE(REG_I2S_ACTL_RESET, val);
                AUDIO_WRITE(REG_I2S_ACTL_CON, con);
                break;
                
        default:
                ret = -EINVAL;
        }

		LEAVE();
        return ret;
}

static int w55fa92_i2s_probe(struct platform_device *pdev,
                            struct snd_soc_dai *dai)
{
        struct w55fa92_audio *w55fa92_audio = w55fa92_i2s_data;
//        unsigned long val;

		ENTER();
		
        mutex_lock(&i2s_mutex);

        /* enable unit clock */
        clk_enable(clk_hclk4);        
        clk_enable(w55fa92_audio->eng_clk);        
        clk_enable(w55fa92_audio->i2s_clk);        

		// enable I2S pins
		AUDIO_WRITE(REG_GPBFUN0, (AUDIO_READ(REG_GPBFUN0)&~(MF_GPB6+MF_GPB5+MF_GPB4+MF_GPB3+MF_GPB2))|0x01111100);	// GPB[6:2] to be I2S signals
		AUDIO_WRITE(REG_MISFUN, AUDIO_READ(REG_MISFUN) & (~0x01));					// I2S interface for I2S, but not SPU

		// set Play & Record interrupt encountered in half of DMA buffer length
		AUDIO_WRITE(REG_I2S_ACTL_CON, (AUDIO_READ(REG_I2S_ACTL_CON) & (~R_DMA_IRQ_SEL)) | (0x01 << 14)); 	
		AUDIO_WRITE(REG_I2S_ACTL_CON, (AUDIO_READ(REG_I2S_ACTL_CON) & (~P_DMA_IRQ_SEL)) | (0x01 << 12)); 		

		// clear I2S interrupt flags
		AUDIO_WRITE(REG_I2S_ACTL_RSR, R_FIFO_FULL | R_FIFO_EMPTY | R_DMA_RIA_IRQ);	
		AUDIO_WRITE(REG_I2S_ACTL_PSR, 0x1F);	
		AUDIO_WRITE(REG_I2S_ACTL_CON, AUDIO_READ(REG_I2S_ACTL_CON) | I2S_EN | P_DMA_IRQ | R_DMA_IRQ); 		
//		AUDIO_WRITE(REG_I2S_ACTL_CON, AUDIO_READ(REG_I2S_ACTL_CON) | P_DMA_IRQ | R_DMA_IRQ); 		

        mutex_unlock(&i2s_mutex);

		LEAVE();
        return 0;
}

static void w55fa92_i2s_remove(struct platform_device *pdev,
                              struct snd_soc_dai *dai)
{
        struct w55fa92_audio *w55fa92_audio = w55fa92_i2s_data;

		ENTER();
		
		/* disable audio enigne clock */
        clk_disable(w55fa92_audio->i2s_clk);
        clk_disable(w55fa92_audio->eng_clk);
	    clk_disable(clk_hclk4);                
        clk_put(w55fa92_audio->i2s_clk);
        clk_put(w55fa92_audio->eng_clk);
	    clk_put(clk_hclk4);                
			        
		LEAVE();		
}

static struct snd_soc_dai_ops w55fa92_i2s_dai_ops = {
        .trigger	= w55fa92_i2s_trigger,
        .set_fmt	= w55fa92_i2s_set_fmt,
        .set_sysclk	= w55fa92_i2s_set_sysclk,
};

struct snd_soc_dai w55fa92_i2s_dai = {
        .name			= "w55fa92-audio-i2s",
        .id 			= 0,
        .probe			= w55fa92_i2s_probe,
        .remove			= w55fa92_i2s_remove,
        .playback = {
                .rates		= SNDRV_PCM_RATE_8000_48000,
                .formats	= SNDRV_PCM_FMTBIT_S16_LE,
                .channels_min	= 1,
                .channels_max	= 2,
        },
        .capture = {
                .rates		= SNDRV_PCM_RATE_8000_48000,
                .formats	= SNDRV_PCM_FMTBIT_S16_LE,
                .channels_min	= 1,
                .channels_max	= 2,
        },
        .ops = &w55fa92_i2s_dai_ops,
};
EXPORT_SYMBOL_GPL(w55fa92_i2s_dai);

static int __devinit w55fa92_i2s_drvprobe(struct platform_device *pdev)
{
        struct w55fa92_audio *w55fa92_audio;
        int ret;

		ENTER();
		
        if (w55fa92_i2s_data)
                return -EBUSY;

        w55fa92_audio = kzalloc(sizeof(struct w55fa92_audio), GFP_KERNEL);
        if (!w55fa92_audio)
                return -ENOMEM;

        spin_lock_init(&w55fa92_audio->lock);
		spin_lock_init(&w55fa92_audio->irqlock);		
        
        w55fa92_audio->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        if (!w55fa92_audio->res) {
                ret = -ENODEV;
                goto out0;
        }

        if (!request_mem_region(w55fa92_audio->res->start,
                                resource_size(w55fa92_audio->res), pdev->name)) {
                ret = -EBUSY;
                goto out0;
        }

        w55fa92_audio->mmio = ioremap(w55fa92_audio->res->start,
                                     resource_size(w55fa92_audio->res));
        if (!w55fa92_audio->mmio) {
                ret = -ENOMEM;
                goto out1;
        }

//        w55fa92_audio->clk = clk_get(&pdev->dev, NULL);
//        if (IS_ERR(w55fa92_audio->clk)) {
//                ret = PTR_ERR(w55fa92_audio->clk);
//                goto out2;
//        }

       if ( clk_hclk4 == NULL )
       {
			clk_hclk4 = clk_get(NULL, "hclk4");
	        if (IS_ERR(clk_hclk4)) {
                ret = PTR_ERR(clk_hclk4);
                goto out2;       	
			}                
       }

        w55fa92_audio->i2s_clk = clk_get(NULL, "i2s");
        if (IS_ERR(w55fa92_audio->i2s_clk)) {
                ret = PTR_ERR(w55fa92_audio->i2s_clk);
                goto out2;
        }

        w55fa92_audio->eng_clk = clk_get(NULL, "ado");
        if (IS_ERR(w55fa92_audio->eng_clk)) {
                ret = PTR_ERR(w55fa92_audio->eng_clk);
                goto out2;
        }

//        w55fa92_audio->irq_num = platform_get_irq(pdev, 0);
        w55fa92_audio->irq_num = IRQ_I2S;        
        if (!w55fa92_audio->irq_num) {
                ret = -EBUSY;
                goto out2;
        }
		
		ret = w55fa92_dma_create(w55fa92_audio);
		if (ret != 0)
			return ret;
		
        w55fa92_i2s_data = w55fa92_audio;

        w55fa92_audio->dev = w55fa92_i2s_dai.dev =  &pdev->dev;

        ret = snd_soc_register_dai(&w55fa92_i2s_dai);
        if (ret)
                goto out3;

//        mfp_set_groupg(w55fa92_audio->dev); /* enbale i2s multifunction pin*/

		DBG("w55fa92_i2s_drvprobe OK \n");
		LEAVE();
        return 0;

out3:
//        clk_put(w55fa92_audio->clk);
out2:
        iounmap(w55fa92_audio->mmio);
out1:
        release_mem_region(w55fa92_audio->res->start,
                           resource_size(w55fa92_audio->res));
out0:
        kfree(w55fa92_audio);
        
        
		DBG("w55fa92_i2s_drvprobe FAIL \n");        

		LEAVE();        
        return ret;
}

static int __devexit w55fa92_i2s_drvremove(struct platform_device *pdev)
{
		ENTER();
			
		w55fa92_dma_destroy(w55fa92_i2s_data);
		
        snd_soc_unregister_dai(&w55fa92_i2s_dai);

//        clk_put(w55fa92_i2s_data->clk);
        iounmap(w55fa92_i2s_data->mmio);
        release_mem_region(w55fa92_i2s_data->res->start,
                           resource_size(w55fa92_i2s_data->res));

        w55fa92_i2s_data = NULL;

        return 0;
}

static struct platform_driver w55fa92_i2s_driver = {
        .driver	= {
                .name	= "w55fa92-audio-i2s",
                .owner	= THIS_MODULE,
        },
        .probe		= w55fa92_i2s_drvprobe,
        .remove		= __devexit_p(w55fa92_i2s_drvremove),
                     };

static int __init w55fa92_i2s_init(void)
{
		ENTER();
			
        return platform_driver_register(&w55fa92_i2s_driver);
}

static void __exit w55fa92_i2s_exit(void)
{
		ENTER();
			
        platform_driver_unregister(&w55fa92_i2s_driver);
}

module_init(w55fa92_i2s_init);
module_exit(w55fa92_i2s_exit);

MODULE_AUTHOR("Wan ZongShun <mcuos.com@gmail.com>");
MODULE_DESCRIPTION("W55FA92 IIS SoC driver!");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:w55fa92-i2s");
