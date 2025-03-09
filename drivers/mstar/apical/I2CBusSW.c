#include <common.h>

#define Bit_RESET		0
#define Bit_SET			1

#define I2CBusDelay(us)   	udelay(50);
#define oI2C1_SCL(out)  	(ApkSetGpioVal(4, out))
#define oI2C1_SDA(out)  	(ApkSetGpioVal(5, out))
#define oI2C1_SDAGet()  	(ApkReadGpio(5))
#define oI2c_SDA_SETIN() 	(ApkSetGpioIn(5))	

extern int ApkGpioInit(int gpio);
extern int ApkSetGpioIn(int gpio);
extern void ms_i2c_io_reinit();

void ApkSwI2cInit()
{
	ApkGpioInit(4);
	ApkGpioInit(5);
}

void IIC_Starts(void)
{
	oI2C1_SCL(Bit_SET);
	I2CBusDelay(1);
	oI2C1_SDA(Bit_SET);		//空闲态
	I2CBusDelay(1);
	oI2C1_SDA(Bit_RESET);
	I2CBusDelay(1);
	oI2C1_SCL(Bit_RESET);
	I2CBusDelay(1);
}

void IIC_Stop(void)
{
	oI2C1_SCL(Bit_RESET);	//确保时钟为低
	I2CBusDelay(1);
	oI2C1_SDA(Bit_RESET);	//数据准备为低
	I2CBusDelay(1);
	oI2C1_SCL(Bit_SET);
	I2CBusDelay(1);
	oI2C1_SDA(Bit_SET);   //当SCL线为高电平时， SDA线上由低到高电平跳变
	I2CBusDelay(1);
}

int IIC_Wait_Ack(void)
{
	unsigned char ucTimeCnt= 0;

	oI2C1_SCL(Bit_RESET); 
	I2CBusDelay(1);
	oI2C1_SDA(Bit_SET);
	oI2c_SDA_SETIN();
	I2CBusDelay(1);
	oI2C1_SCL(Bit_SET);
	I2CBusDelay(1);
	while(oI2C1_SDAGet())
	{
		ucTimeCnt ++;
		if(ucTimeCnt>50)
		{
			IIC_Stop();
			return -1;
		}
		I2CBusDelay(1);
	}

	oI2C1_SCL(Bit_RESET);  
	I2CBusDelay(1);
	oI2C1_SDA(Bit_RESET);
	I2CBusDelay(1);
	return 0;
}

void  IIC_Send_Byte(unsigned char byte)
{
	unsigned char i = 0;
	unsigned char temp = byte;

	//printf(">\r\n");
	for(i=0;i<8;i++)
	{
		if(temp&0x80)
		{
			oI2C1_SDA(Bit_SET);  		//每次发最高位
			//printf("1");
		}
		else
		{
			oI2C1_SDA(Bit_RESET);
			//printf("0");
		}
		temp = temp << 1;				//temp左移一位
		I2CBusDelay(1);
		oI2C1_SCL(Bit_SET);
		I2CBusDelay(1);
		oI2C1_SCL(Bit_RESET);
		I2CBusDelay(1);
	}

	oI2C1_SDA(Bit_RESET);  ///
	//printf("\r\n>\r\n");
}
unsigned char IIC_Read_Byte(unsigned char ack)
{
	unsigned char i, recvVal=0;

	oI2c_SDA_SETIN();
	I2CBusDelay(4);
 	for(i=0; i<8; i++)
	{
		oI2C1_SCL(Bit_SET);
		I2CBusDelay(4);
		recvVal = recvVal<<1;
		recvVal	|= oI2C1_SDAGet();
		oI2C1_SCL(Bit_RESET);
		I2CBusDelay(4);
	}

#if 1
	if(ack)
	{
		oI2C1_SDA(Bit_RESET);
		I2CBusDelay(1);
		oI2C1_SCL(Bit_SET);
		I2CBusDelay(1);
		oI2C1_SCL(Bit_RESET);
		I2CBusDelay(1);
	}
	else
	{
		oI2C1_SDA(Bit_SET);
		I2CBusDelay(1);
		oI2C1_SCL(Bit_SET);
		I2CBusDelay(1);
		oI2C1_SCL(Bit_RESET);
		I2CBusDelay(1);
	}
#endif

	return recvVal;   
}

int ApkI2c2WriteOneByte(unsigned char chipAddr, unsigned int regAddr, unsigned char DataToWrite)
{
	//printf("ApkI2c2WriteOneByte %d[%d] = %d star\r\n", chipAddr<<1, regAddr, DataToWrite);

	return 0;
	
	IIC_Starts();					//发出起始信号
	IIC_Send_Byte(chipAddr<<1);   	//写器件地址+写操作
	if(IIC_Wait_Ack()<0)
	{
		printf("ApkI2c2WriteOneByte NACK 1\r\n");
		return -1;
	}

	if(regAddr & 0xFF00)
	{
		IIC_Send_Byte(regAddr>>8);
		if(IIC_Wait_Ack()<0)
		{
			printf("ApkI2c2WriteOneByte NACK 2\r\n");
			return -2;
		}
	}
	IIC_Send_Byte(regAddr&0xFF);
	if(IIC_Wait_Ack()<0)
	{
		printf("ApkI2c2WriteOneByte NACK 3\r\n");
		return -3;
	}

	//for()
	{
		IIC_Send_Byte(DataToWrite);
		if(IIC_Wait_Ack()<0)
		{
			printf("ApkI2c2WriteOneByte NACK 4\r\n");
			return -4;
		}
	}
	
	IIC_Stop();
	printf("ApkI2c2WriteOneByte 0x%x[0x%x] = 0x%x OK\r\n", chipAddr<<1, regAddr, DataToWrite);
	return 0;
}

unsigned char AkpI2cReadOneByte(unsigned char chipAddr, unsigned int regAddr)
{
	unsigned char  tempVal = 0;
	
	return 0;

	//printf("AkpI2cReadOneByte %d[%d] = %d start\r\n", chipAddr<<1, regAddr, tempVal);
	
	IIC_Starts();   				//发出起始信号
	IIC_Send_Byte(chipAddr<<1); 		//写器件地址+写操作
	if(IIC_Wait_Ack()<0)
	{
		printf("AkpI2cReadOneByte NACK 1\r\n");
		return -1;
	}

	if(regAddr & 0xFF00)
	{
		IIC_Send_Byte(regAddr>>8);
		if(IIC_Wait_Ack()<0)
		{
			printf("AkpI2cReadOneByte NACK 2\r\n");
			return -2;
		}
	}
	IIC_Send_Byte(regAddr&0xFF);
	if(IIC_Wait_Ack()<0)
	{
		printf("AkpI2cReadOneByte NACK 3\r\n");
		return -3;
	}

	I2CBusDelay(1);
	IIC_Starts();  //重复起始信号
	
	IIC_Send_Byte((chipAddr<<1)+1); 		//写器件地址+读操作
	if(IIC_Wait_Ack()<0)
	{
		printf("AkpI2cReadOneByte NACK 4\r\n");
		return -4;
	}

	//for()
	{
		if(0)
			tempVal = IIC_Read_Byte(1);  //读取一个字节，发出ACK
		else
			tempVal = IIC_Read_Byte(0);  //读取最后一个字节，发出NACK
	}
	IIC_Stop();     //发出终止信号

	printf("AkpI2cReadOneByte 0x%x[0x%x] = 0x%x OK\r\n", chipAddr<<1, regAddr, tempVal);
	return tempVal;   
}

int do_swi2c (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    char *ep;
	unsigned char chipAddr;
	unsigned int regAddr;
	unsigned char dat;
	
    chipAddr = (unsigned char)simple_strtoul(argv[2], &ep, 16);
    regAddr = (unsigned int)simple_strtoul(argv[3], &ep, 16);

	ApkSwI2cInit();
	
	if(argv[1][0] == 'r')
	{
		AkpI2cReadOneByte(chipAddr, regAddr);
	}
	else if(argv[1][0] == 'w')
	{
		dat = (unsigned int)simple_strtoul(argv[4], &ep, 16);
		ApkI2c2WriteOneByte(chipAddr, regAddr ,dat);
	}
	
	//ms_i2c_io_reinit();
	
	return 0;
}

U_BOOT_CMD(
	swi2c, 6, 1, do_swi2c,
	"apk sw I2C sub-system",
	"swi2c r/w chipAddr regAddr dat"
);

int do_sda (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int i=0;
	for(i=0; i<1000; i++)
	{
		oI2C1_SDA(1);
		I2CBusDelay(100);
		oI2C1_SDA(0);
		I2CBusDelay(100);
	}
	
	oI2c_SDA_SETIN();
	for(i=0; i<10; i++)
	{
		I2CBusDelay(100);
		printf("SDA:%d\r\n", oI2C1_SDAGet());
	}
	
	return 0;
}
U_BOOT_CMD(
	sda, 6, 1, do_sda,
	"apk sw I2C sub-system",
	"swi2c r/w chipAddr regAddr dat"
);


