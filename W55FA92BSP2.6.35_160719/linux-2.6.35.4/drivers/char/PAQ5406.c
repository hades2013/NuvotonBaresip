
/**********/

#include <linux/platform_device.h>
#include <asm/io.h>


#include <mach/w55fa92_gpio.h>
#include <linux/delay.h>
#include <linux/random.h>



extern unsigned long  libbackdata;

/**********/


typedef int YAN_BIT ;
typedef unsigned char BYTE;
//typedef unsigned int WORD;

void clkpic(BYTE bitdata)
{
    if(bitdata)
        //SCL=1;
       w55fa92_gpio_set(GPIO_GROUP_B, 13, 1);
    else         
       // SCL=0;
       w55fa92_gpio_set(GPIO_GROUP_B, 13, 0);
}
void datpic(BYTE bitdata)
{
    if(bitdata)
        //SDA=1;
        w55fa92_gpio_set(GPIO_GROUP_B, 14, 1);
    else         
        //SDA=0;
        w55fa92_gpio_set(GPIO_GROUP_B, 14, 0);
}

void delayms(int count)
{
    int i,j;
    for(i=0;i<count;i++)
    {       
		for(j=0;j<1000;j++);         
    }
}
void delaydm(void)
{
    int i,j;
    for(i=0;i<1;i++)
    {       
		//for(j=0;j<1000;j++)	;         
		udelay(100);
    }
}


void I2cStart(void)   
{
    datpic(1);
    clkpic(1);
    delaydm();
    datpic(0);
    delaydm();
    clkpic(0);
    delaydm();
}

void I2cStop(void)   
{
    datpic(0);
    delaydm();
    clkpic(1);
    delaydm();
    datpic(1);
    delaydm();
}

YAN_BIT I2cSendByte(BYTE bytedata) 
{
     BYTE i;
    YAN_BIT ack;
    
    //I2cStart();
    for(i=0;i<8;i++)
    {
        if(bytedata & 0x80)
        datpic(1);
        else
        datpic(0);
        bytedata <<= 1;
        delaydm();
        clkpic(1);
        delaydm();
        clkpic(0);
        delaydm();
      }
	//SDA=1;
	w55fa92_gpio_set_input(GPIO_GROUP_B, 14);
    //delaydm();
    clkpic(1);
    delaydm();
	delaydm();
	delaydm();
	delaydm();
    //ack = SDA;    
    ack = w55fa92_gpio_get(GPIO_GROUP_B, 14);
	w55fa92_gpio_set_output(GPIO_GROUP_B, 14);
    clkpic(0);
    delaydm();
    delaydm();
    

  
    return ack;
}


YAN_BIT I2cSendLong(long bytedata) 
{
    BYTE i;
    YAN_BIT ack;

    BYTE data8,data16,data24,data32;
     data8 = bytedata>>24;
    
    for(i=0;i<8;i++)
    {
        if(data8 & 0x80)
        datpic(1);
        else
        datpic(0);
        data8 <<= 1;
        delaydm();
        clkpic(1);
        delaydm();
        clkpic(0);
        delaydm();
      }
	//SDA=1;
	w55fa92_gpio_set_input(GPIO_GROUP_B, 14);
    delaydm();
    clkpic(1);
    delaydm();
    //ack =SDA; 
    ack = w55fa92_gpio_get(GPIO_GROUP_B, 14);
	w55fa92_gpio_set_output(GPIO_GROUP_B, 14);
    clkpic(0);
    delaydm();
    delaydm();
    
	if(ack==1)
	{
		I2cStop();
    	return ack;
    }

    data16 = bytedata>>16;
          
    for(i=0;i<8;i++)
    {
        if(data16 & 0x80)
        datpic(1);
        else
        datpic(0);
        data16<<= 1;
        delaydm();
        clkpic(1);
        delaydm();
        clkpic(0);
        delaydm();
      }
	//SDA=1;
	w55fa92_gpio_set_input(GPIO_GROUP_B, 14);
    delaydm();
    clkpic(1);
    delaydm();
    //ack =SDA; 
    ack = w55fa92_gpio_get(GPIO_GROUP_B, 14);
	w55fa92_gpio_set_output(GPIO_GROUP_B, 14);    
    clkpic(0);
    delaydm();
    delaydm();
    
	if(ack==1)
	{
		I2cStop();
    	return ack;
    }

     data24 = bytedata>>8;
	
    for(i=0;i<8;i++)
    {
        if(data24 & 0x80)
        datpic(1);
        else
        datpic(0);
        data24 <<= 1;
        delaydm();
        clkpic(1);
        delaydm();
        clkpic(0);
        delaydm();
      }
	//SDA=1;
	w55fa92_gpio_set_input(GPIO_GROUP_B, 14);	
    delaydm();
    clkpic(1);
    delaydm();
    //ack =SDA; 
    ack = w55fa92_gpio_get(GPIO_GROUP_B, 14);
	w55fa92_gpio_set_output(GPIO_GROUP_B, 14);    
    clkpic(0);
    delaydm();
    delaydm();
    
	if(ack==1)
	{
		I2cStop();
    	return ack;
    }

	data32 =  bytedata;   
    for(i=0;i<8;i++)
    {
        if(data32 & 0x80)
        datpic(1);
        else
        datpic(0);
        data32<<= 1;
        delaydm();
        clkpic(1);
        delaydm();
        clkpic(0);
        delaydm();
      }
	//SDA=1;
	w55fa92_gpio_set_input(GPIO_GROUP_B, 14);
    delaydm();
    clkpic(1);
    delaydm();
    //ack =SDA; 
    ack = w55fa92_gpio_get(GPIO_GROUP_B, 14);
	w55fa92_gpio_set_output(GPIO_GROUP_B, 14);        
    clkpic(0);
    delaydm();
    delaydm();
    	
    I2cStop();
    return ack;
}


void I2cLongWrite(BYTE device,long bytedata)
{      
    BYTE i,j;    
    YAN_BIT ack;

    for(i=0;i<3;i++)
    {
        I2cStart();
        ack = I2cSendByte(device);
        if(ack==1)
        {
            I2cStop();
            for(j=0;j<20;j++)
            {
                delaydm();
            }
            continue;
        }
        ack = I2cSendLong(bytedata);
        if(ack==1)
        {
            for(j=0;j<20;j++)
            {
                delaydm();
            }
            continue;
        }
        if(ack==0)  break;    
    }
   
}

void SendAck(BYTE ack) 
{
    datpic(ack);
    clkpic(1);
    delaydm();
    clkpic(0);
   	datpic(1);
    delaydm();
    delaydm();
    
}


BYTE I2cReceiveByte(void)  
{
	BYTE i;
	BYTE bytedata = 0;

	w55fa92_gpio_set_input(GPIO_GROUP_B, 14);
	
	for(i=0;i<8;i++)
  	{
    	clkpic(1);
		delaydm();
		bytedata <<= 1;
		
    	if( w55fa92_gpio_get(GPIO_GROUP_B, 14) )
    	{
      		bytedata |= 0x01;
    	}
    	clkpic(0);
    	delaydm();
    	delaydm();
  	}

	w55fa92_gpio_set_output(GPIO_GROUP_B, 14); 
  	return bytedata;
}


long I2cLongRead(BYTE device)
{      
	long backdata;
	YAN_BIT ack;
	long bytedata8;
	long bytedata16;
	long bytedata24;
	long bytedata32;
	I2cStart();
	ack=I2cSendByte(device);
	if(ack)
	{
		I2cStop();
	}
	bytedata32=I2cReceiveByte();
	SendAck(0);
	bytedata24=I2cReceiveByte();
	SendAck(0);
	bytedata16=I2cReceiveByte();
	SendAck(0);
	bytedata8=I2cReceiveByte();
	SendAck(0);
	I2cStop();
	backdata= (bytedata32<<24) | (bytedata24<<16) | (bytedata16<<8) | bytedata8;
	//backdata=0xcc22aa55;
	return backdata;
}


extern 	unsigned long rdmdata;

void paq_check(void){
	unsigned long i2cbackdata;

	//rdmdata = xxxx

	printk("paq 1=0x%x\n", rdmdata);

	w55fa92_gpio_configure(GPIO_GROUP_B, 13);
	w55fa92_gpio_configure(GPIO_GROUP_B, 14);
	
	w55fa92_gpio_set_output(GPIO_GROUP_B, 13);
	w55fa92_gpio_set_output(GPIO_GROUP_B, 14);


	//for(i=0;i<50;i++)
		{
		I2cLongWrite(0x50,rdmdata);
		udelay(100);
		udelay(100);
		udelay(100);
		i2cbackdata=I2cLongRead(0x51);
		printk("paq 2:0x%x\n",i2cbackdata);

		libbackdata=i2cbackdata;
	}
	


}


static int __init  paq5406_module_init(void)
{

	paq_check();
	
	printk("w55fa92 paq5406_module initialized successfully!\n");

	return 0;
}

static void __exit paq5406_module_exit(void)
{
	
}

module_init(paq5406_module_init);
module_exit(paq5406_module_exit);

MODULE_DESCRIPTION("W55FA92 paq5406_module driver");
MODULE_LICENSE("GPL");


