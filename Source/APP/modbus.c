#include "bsp.h"
#include "os_cfg_app.h"


extern u8 Onlineflg;                                                           //在线升级标志
extern u8 UART1_RXBuff[MAX_USART1_DATA_LEN];                                   //串口接收缓存
extern u8 const SensorSoftVersion[8];                                         //软件版本
extern u8 const User_Default_Param[PRO_DEFAULT_LEN];                          //用户缺省参数
extern BitAction Add_Sub_Flag;
extern BitAction Calibrate_Flag;
extern EverySecFilTypeDef SecFilStr;


extern u8 CALIB_Table[CALIN_HEIGH_TABLE_NUM];
//extern u8 CALIB_Table1[CALIN_HEIGH_TABLE_NUM1];

u8 SendLen;                                                                     //发送数据长度
u8 SendBuf[DATAPACKET_LEN];                                                     //存储从机响应消息帧

u32 dwEepromHeightADValue;                                                      //油量数组液位高度
u32 dwEepromOilValue;                                                           //油量数组油量
u32 dwUpEepromHeightADValue;                                                    //上一个油量数组液位高度
u32 dwUpEepromOilValue;                                                         //上一个油量数组油量
u32 dwTankOilValue;                                                             //当前油箱油量
float dwTable_k;                                                               //油量关系斜率

extern u8 Pvd_Cnt;  //PVD状态上电计时
//**************************************************************************************************
// 名称         	: MBASCII_GetSlaveAddr()
// 创建日期   	        : 2015-10-29
// 作者         	: wang
// 功能         	:获得设备地址
// 输入参数     	:接收的缓冲数据(u8 *u8Msg)
// 输出参数     	: 设备地址
// 返回结果     	: 无
// 注意和说明   	:
// 修改内容     	:
//**************************************************************************************************

u32 MBASCII_GetSlaveAddr(u8 *u8Msg)
{
    return(u8Msg[0]);
}

//**************************************************************************************************
// 名称         	: MBASCII_GetFunCode()
// 创建日期   	        : 2015-10-29
// 作者         	: wang
// 功能         	:获得功能码
// 输入参数     	:接收的缓冲数据(u8 *u8Msg)
// 输出参数     	: 功能码
// 返回结果     	: 无
// 注意和说明   	:
// 修改内容     	:
//**************************************************************************************************

u32 MBASCII_GetFunCode(u8 *u8Msg)
{
    return(u8Msg[1]);
}

//**************************************************************************************************
// 名称         	: MBASCII_SendMsg()
// 创建日期   	        : 2015-10-29
// 作者         	: wang
// 功能         	:发送指令
// 输入参数     	:发送缓冲数据(uint8_t *u8Msg),数据长度(uint32_t u8MsgLen)
// 输出参数     	: 设备地址
// 返回结果     	: 无
// 注意和说明   	:
// 修改内容     	:
//**************************************************************************************************

void MBASCII_SendMsg(uint8_t *u8Msg, uint32_t u8MsgLen)
{
  
  
  Delay_Ms(39);
  if((MB_ADDRESS_BROADCAST != MBASCII_GetSlaveAddr(u8Msg)) && (UartComm.Echo == ECHO_ENABLE))
  {
    MODBUS_ASCII_SendData( u8Msg, u8MsgLen );	
  }
}


//**************************************************************************************************
// 名称         	: MBASCII_SendErr()
// 创建日期     	: 2015-07-13
// 作者        	 	: 王小鑫
// 功能         	: 发送MODBUS 错误
// 输入参数     	: u8 ErrCode
// 输出参数     	: 无
// 返回结果     	: 无
// 注意和说明   	:
// 修改内容     	:
//**************************************************************************************************

void MBASCII_SendErr(u8 ErrCode)
{
  SendLen = 1;
  SendBuf[SendLen++] |= 0x80;
  SendBuf[SendLen++] = ErrCode;

  MBASCII_SendMsg(SendBuf,SendLen);
}

//**************************************************************************************************
// 名称         	: MBASCII_SendMsg_NoLimit()
// 创建日期   	        : 2015-10-29
// 作者         	: wang
// 功能         	:发送指令
// 输入参数     	:发送缓冲数据(uint8_t *u8Msg),数据长度(uint32_t u8MsgLen)
// 输出参数     	: 设备地址
// 返回结果     	: 无
// 注意和说明   	:
// 修改内容     	:
//**************************************************************************************************

void MBASCII_SendMsg_NoLimit(u8 *u8Msg, u32 u8MsgLen)
{
  Delay_Ms(39);
  if (UartComm.Echo == ECHO_ENABLE)
  {
    MODBUS_ASCII_SendData( u8Msg, u8MsgLen );
  }
}

//**************************************************************************************************
// 名称         	: Read_LoadAndWeightTable()
// 创建日期     	: 2015-07-13
// 作者        	 	: 庄明群
// 修改人               : 庄明群
// 功能         	: 读取载重AD值和称重对应表
// 输入参数     	: 无
// 输出参数     	: 无
// 返回结果     	: 无
// 注意和说明   	:
// 修改内容     	:帧格式
//**************************************************************************************************

void Read_CALIBAndHeighTable(void)
{
  u8 i;
  u8 dwSendLen = 0;
  u16 dwPacketSum;//总包数
  u16 dwPacketNum;//读第几包数据
  //u8 temp = 0;
  u8 dwTable[200];
  dwPacketSum = UART1_RXBuff[7]*256 + UART1_RXBuff[8];
  dwPacketNum = UART1_RXBuff[9]*256 + UART1_RXBuff[10];

  dwTable[dwSendLen++] = (MBASCII_GetSlaveAddr(UART1_RXBuff)) ? UartComm.SlaveAddr : 0x00;
  dwTable[dwSendLen++] = MB_FUNC_READ_HOLDING_REGISTER;
  dwTable[dwSendLen++] = UART1_RXBuff[2];
  dwTable[dwSendLen++] = UART1_RXBuff[3];
  dwTable[dwSendLen++] = UART1_RXBuff[4];
  dwTable[dwSendLen++] = UART1_RXBuff[5];
  dwTable[dwSendLen++] = UART1_RXBuff[6];
  dwTable[dwSendLen++] = UART1_RXBuff[7];//总包数
  dwTable[dwSendLen++] = UART1_RXBuff[8];
  dwTable[dwSendLen++] = UART1_RXBuff[9];//第几包数
  dwTable[dwSendLen++] = UART1_RXBuff[10];

  if(0x05 >= dwPacketSum)//总包数
  {
    
    if(dwPacketNum < 0x05)
    {
      for(i = 0; i < 80; i++)
      {
//         I2C1_ReadNBytes(EEPROM_ADDRESS, CALIB_HEIGH_RELATIONSHIP + dwPacketNum * 80 + i, 1, (u8 *)temp);
//          
//        dwTable[dwSendLen++] =  temp;  
        
	dwTable[dwSendLen++] = CALIB_Table[dwPacketNum * 80 + i];
      }
      MBASCII_SendMsg(dwTable,dwSendLen);
    }
    
  }
}

//**************************************************************************************************
// 名称         	: MBASCII_Fun03()
// 创建日期     	: 2016-09-01
// 作者        	 	: 庄明群
// 功能         	: Modbus 03功能处理
// 输入参数     	: 无
// 输出参数     	: 无
// 返回结果     	: 无
// 注意和说明   	:
// 修改内容     	:
//**************************************************************************************************

void MBASCII_Fun03(void)
{
  u8 i;
  u8 Addrflg;//寄存器地址标志 用于标识只读一个从机地址信息时忽略请求帧的从机地址
  u8 ReadAdrH;//寄存器地址高8位
  u8 ReadAdrL;//寄存器地址低8位
  u16 Data_Buf;//数据内容
  u16 RegisterNum = (u16)UART1_RXBuff[4]*256 + UART1_RXBuff[5];//寄存器数量

  ReadAdrH = UART1_RXBuff[2];
  ReadAdrL = UART1_RXBuff[3];
        
  SendLen = 0;
  SendBuf[SendLen++] = (MBASCII_GetSlaveAddr(UART1_RXBuff)) ? UartComm.SlaveAddr : 0x00;
  SendBuf[SendLen++] = MB_FUNC_READ_HOLDING_REGISTER;//功能码
  SendBuf[SendLen++] = RegisterNum * 2;	//数据长度
        
  Addrflg = 0;
   
  //判断访问寄存器地址是否在有效范围内
  if(!(((ReadAdrL >= ASCII_HOLDING_REG_REGION1_BGEIN) && (ReadAdrL <= ASCII_HOLDING_REG_REGION1_END)
     && (ReadAdrL + RegisterNum <= (ASCII_HOLDING_REG_REGION1_END + 1))&&(ReadAdrH == UartComm.SlaveAddr))
     ||((ReadAdrL == ASCII_HOLDING_REG_REGION3_BGEIN) && (RegisterNum == 0x2A)
     && (RegisterNum*2 == UART1_RXBuff[6])&&(ReadAdrH == UartComm.SlaveAddr))
     ||((ASCII_HOLDING_REG_REGION1_BGEIN == ReadAdrL)&&(1 == RegisterNum)))||(0 == RegisterNum))
  {
    //返回非法数据地址
    MBASCII_SendErr(MB_EX_ILLEGAL_DATA_ADDRESS);
    return;
  }

  //如果访问油量标定数组
  if(0x80 == ReadAdrL)
  {
    //读取油量对应表
    Read_CALIBAndHeighTable();
    return;
  }
        
  for(long k = 0; k <  RegisterNum; ReadAdrL++, k++)
  {
    switch(ReadAdrL)
    {
    case 0x30://从机地址
      Addrflg = 1;
      Data_Buf = UartComm.SlaveAddr;
    break;
            
    case 0x31://波特率
      Data_Buf = UartComm.BaudRate;
    break;
            
    case 0x32://奇偶校验
      Data_Buf = UartComm.Parity;
    break;
            
    case 0x33:
      Data_Buf = 0;
    break;
    
    case 0x34://补偿使能
      Data_Buf = ProductPara.CompenEn;
    break;    
            
    case 0x35://滤波等级
      Data_Buf = UserParam.FilterLevel;
    break;
            
    case 0x36://自动上传周期
      Data_Buf = ProductPara.AutoUpLoadTime;
    break;
            
    case 0x37://电容修正系数K
      Data_Buf = (u32)(UserParam.UserFlowK * 100);
    break;
            
    case 0x38://电容修正系数B
      Data_Buf = (u32)(UserParam.UserFlowB * 100);
    break;
    
    case 0x39:
      Data_Buf = UserParam.FilterM;
      break;

    case 0x3A:
      Data_Buf = UserParam.FilterN;
      break;      
            
    case 0x3F://油杆长度
      Data_Buf = ProductPara.SensorLen;
    break;
            
    case 0x40://液位比较时间间隔
      Data_Buf = ProductPara.LiquidCmpTc;
    break;
            
    case 0x41://燃油类型
      Data_Buf = ProductPara.FuelType;
    break;
            
    case 0x42://油箱形状
      Data_Buf = ProductPara.BoxShape;
    break;
            
    case 0x43://油箱长度
      Data_Buf = ProductPara.BoxPara[0];
    break;
            
    case 0x44://油箱宽度
      Data_Buf = ProductPara.BoxPara[1];
    break;
            
    case 0x45://油箱高度
      Data_Buf = ProductPara.BoxPara[2];
    break;
            
    case 0x46://油箱壁厚 保留
      Data_Buf = ProductPara.BoxPara[3];
    break;
            
    case 0x47://加油时间阈值
      Data_Buf = ProductPara.AddLiqTime;
    break;
            
    case 0x48://加油量阈值
      Data_Buf = ProductPara.AddLiqCnt;
    break;
            
    case 0x49://漏油时间阈值
      Data_Buf = ProductPara.SubLiqTime;
    break;
            
    case 0x4A://漏油量阈值
      Data_Buf = ProductPara.SubLiqCnt;
    break;
            
    default:
      Data_Buf = 0;
    break;
    }
          
    for(i = 2; i > 0; i--)
    {
      SendBuf[SendLen++] = (u8)(Data_Buf >> ((i - 1) * 8));
    }
  }
  //只读一个从机地址消息的响应      
  if((1 == Addrflg)&&(2 == SendBuf[2]))
  {
    MODBUS_ASCII_SendData(SendBuf,SendLen);
  }
  //读多个消息或其它内容的响应
  else
  {
    MBASCII_SendMsg(SendBuf,SendLen);
  }
}

//**************************************************************************************************
// 名称         	: MBASCII_Fun04()
// 创建日期     	: 2016-09-01
// 作者        	 	: 庄明群
// 功能         	: Modbus 04功能处理
// 输入参数     	: 无
// 输出参数     	: 无
// 返回结果     	: 无
// 注意和说明   	:
// 修改内容     	:
//**************************************************************************************************
BitAction Read_Add_Sub_FLag = Bit_RESET;                                        //读数据标识
void MBASCII_Fun04(void)
{
    u8 i;
    u8 ReadAdrH;//寄存器地址高8位
    u8 ReadAdrL;//寄存器地址低8位
    u32 Data_Buf;//数据内容

    u16 RegisterNum = (u16)UART1_RXBuff[4]*256 + UART1_RXBuff[5];//寄存器数量

    float rate = 0;

    ReadAdrH = UART1_RXBuff[2];
    ReadAdrL = UART1_RXBuff[3];

    SendLen = 0;
    SendBuf[SendLen++] = (MBASCII_GetSlaveAddr(UART1_RXBuff)) ? UartComm.SlaveAddr : 0x00;//从机地址
    SendBuf[SendLen++] = MB_FUNC_READ_INPUT_REGISTER;//功能码
    SendBuf[SendLen++] = RegisterNum * 2;//数据长度

    //如果读取范围溢出
    if(!(((ReadAdrL <= ASCII_INPUT_REG_REGION1_END)
    && ((ReadAdrL + RegisterNum) <= (ASCII_INPUT_REG_REGION1_END + 2))))
    ||(0 == RegisterNum)||(0 != (RegisterNum & 0x01))|| (0 != (ReadAdrL & 0x01))||(ReadAdrH != UartComm.SlaveAddr))
    {
        //返回非法访问地址
        MBASCII_SendErr(MB_EX_ILLEGAL_DATA_ADDRESS);
        return;
    }

    for(long k = 0; k < RegisterNum; ReadAdrL += 2, k += 2)
    {
        switch(ReadAdrL)
        {
            case 0x00:                          
                Data_Buf = RunVar.LiquidAD;
                if(Add_Sub_Flag == Bit_SET)
                {
                    Data_Buf |= 0x80000000;
                }
                else
                {
                    Data_Buf &= 0x7FFFFFFF;
                }              
                break;

            case 0x02:                                                              //液体温度                                                     
                Data_Buf = (u32)((RunVar.TempInSensor + 273.15) * 10);             
                break;

            case 0x04:                                                                //环境温度
                Data_Buf = (u32)((RunVar.TempInAir + 273.15) * 10);
                break;

            case 0x06:                                                                //加油量
                if((Read_Add_Sub_FLag == Bit_RESET) && (Add_Sub_Flag == Bit_SET))
                {
                    Read_Add_Sub_FLag = Bit_SET;                                //读取以后再保持1.6s
                }                                                                                                       
                Data_Buf = ProductPara.AddOil;
                break;

            case 0x08:                                                                //漏油量      
                Data_Buf = ProductPara.SubOil;
                break;

            case 0x0A:                                                                //油箱油量
                rate = (float) (abs(RunVar.Pvd_LiquidHeight - RunVar.LiquidHeight))/ RunVar.Pvd_LiquidHeight;
                if( (RunVar.Pvd_Flag) && (rate <= 0.05) && (Pvd_Cnt <= 50) )
                {       
                    Data_Buf = RunVar.Pvd_OilQuantity;
                }
                else
                {
                    Data_Buf = RunVar.OilQuantity;                
                }
              
                break;

            case 0x0C:                                                                //油量百分比
                Data_Buf = RunVar.LiquidPercent;
                break;

            case 0x0E:                                                                //液位高度(0.1mm)
                rate = (float) (abs(RunVar.Pvd_LiquidHeight - RunVar.LiquidHeight))/ RunVar.Pvd_LiquidHeight;
                if( (RunVar.Pvd_Flag) && (rate <= 0.05) && (Pvd_Cnt <= 50) )
                {       
                    Data_Buf = RunVar.Pvd_LiquidHeight;
                 }
                else
                {
                    Data_Buf = RunVar.LiquidHeight;                
                }  
                break;
               
           case 0x10:                                                                //每秒电容值
                      
                    Data_Buf = SecFilStr.EverySecCap;                 
                break;

            default:
                Data_Buf = 0;
                break;
        } 
        for(i = 4; i > 0; i--)
        {
            SendBuf[SendLen++] = (u8)(Data_Buf >> ((i - 1) * 8));
        }
    }
    //Modbus发送消息      
    MBASCII_SendMsg(SendBuf,SendLen);
}

//**************************************************************************************************
// 名称         	: MBASCII_Fun05()
// 创建日期     	: 2016-09-01
// 作者        	 	: 庄明群
// 功能         	: Modbus 05功能处理
// 输入参数     	: 无
// 输出参数     	: 无
// 返回结果     	: 无
// 注意和说明   	:
// 修改内容     	:
//**************************************************************************************************

void MBASCII_Fun05(void)
{
  u8 ReadAdrH;//寄存器高8位地址
  u8 ReadAdrL;//寄存器低8位地址
  
  ReadAdrH = UART1_RXBuff[2];
  ReadAdrL = UART1_RXBuff[3];
  
  SendLen = 0;
  SendBuf[SendLen++] = (MBASCII_GetSlaveAddr(UART1_RXBuff)) ? UartComm.SlaveAddr : 0x00;
  SendBuf[SendLen++] = MB_FUNC_WRITE_SINGLE_COIL;
  if((ReadAdrL < ASCII_SINGLE_COIL_ADDR_BGEIN) || (ReadAdrL > ASCII_SINGLE_COIL_ADDR_END) || (ReadAdrH != UartComm.SlaveAddr))
  {
    MBASCII_SendErr(MB_EX_ILLEGAL_DATA_ADDRESS);
    return;
  }
  if((ReadAdrL != ASCII_SINGLE_COIL_ADDR_END) && (ProductPara.bFlashWEn == FLASH_WR_DISABLE))
  {
    MBASCII_SendErr(MB_EX_SLAVE_DEVICE_FAILURE);
    return;
  }
  if(!(((UART1_RXBuff[4] == 0xFF) && (UART1_RXBuff[5] == 0x00))
            || ((UART1_RXBuff[4] == 0x00) && (UART1_RXBuff[5] == 0x00))))
  {
    MBASCII_SendErr(MB_EX_ILLEGAL_DATA_VALUE);
    return;
  }
  
  switch(ReadAdrL)
  {
  case 0x50:
    if((UART1_RXBuff[4] == 0x00) && (UART1_RXBuff[5] == 0x00))//标定低液位
    {
      Cur_Param[CAPMIN] = (unsigned char)(RunVar.RawCap >> 24);
      Cur_Param[CAPMIN+1] = (unsigned char)(RunVar.RawCap >> 16);
      Cur_Param[CAPMIN+2] =(unsigned char)(RunVar.RawCap >> 8);
      Cur_Param[CAPMIN+3] = (unsigned char)RunVar.RawCap;
      RunVar.CalState |= CalibrateLow;
    }
    else if((UART1_RXBuff[4] == 0xFF) && (UART1_RXBuff[5] == 0x00))//标定高液位
    {
      Cur_Param[CAPMAX] = (unsigned char)(RunVar.RawCap >> 24);
      Cur_Param[CAPMAX+1] = (unsigned char)(RunVar.RawCap >> 16);
      Cur_Param[CAPMAX+2] =(unsigned char)(RunVar.RawCap >> 8);
      Cur_Param[CAPMAX+3] = (unsigned char)RunVar.RawCap;
      RunVar.CalState |= CalibrateHeight;
    }
    CapCalibrate();
    break;
    
  case 0x51:
    if ((UART1_RXBuff[4] == 0xFF) && (UART1_RXBuff[5] == 0x00))
    {
      Cur_Param[0] = User_Default_Param[0] + 1;//恢复出厂设置
      I2C1_WNBytesMul3T(EEPROM_ADDRESS, RUN_ADDR_BASE, 1, Cur_Param);
#if IWDG_EN > 0
            IWDG_ReloadCounter();
#endif
            Rd_Dev_Param();
#if IWDG_EN > 0
            IWDG_ReloadCounter();
#endif
    }
    break;
    
  case 0x52:
    if ((UART1_RXBuff[4] == 0xFF) && (UART1_RXBuff[5] == 0x00))
    {
      UartComm.Echo = ECHO_ENABLE;//冻结设备
    }
    else
    {
      UartComm.Echo = ECHO_DISABLE;//解冻设备
    }
    I2C1_WNBytesMul3T(EEPROM_ADDRESS, ECHOEN, 1, &UartComm.Echo);
    break;
    
  case 0x53:
    if ((UART1_RXBuff[4] == 0xFF) && (UART1_RXBuff[5] == 0x00))
    {
      ProductPara.bFlashWEn = FLASH_WR_ENABLE;//Flash写使能
    }
    else
    {
      ProductPara.bFlashWEn = FLASH_WR_DISABLE;//Flash写禁止
    }
    break;
    
  default:
    break;
  }
  //Modbus发送操作成功
  MBASCII_SendMsg(UART1_RXBuff, 6);
}

//**************************************************************************************************
// 名称         	: Write_CALIBAndHeighTable()
// 创建日期     	: 2015-07-13
// 作者        	 	: 庄明群 
// 修改人               : 庄明群
// 功能         	: 保存油高度AD值与油箱油量对应表
// 输入参数     	: 无
// 输出参数     	: 无
// 返回结果     	: 无
// 注意和说明   	:
// 修改内容     	: 帧格式
//**************************************************************************************************

void Write_CALIBAndHeighTable(void)
{
  u8 opara[4];
  u16 i,j;
  u16 PacketSum;//总包数
  u16 PacketNum;//包序号
  u16 RegisterNum;//寄存器数量
  u32 Oiltmp;
  //u8 temp=0;
	
  PacketSum = UART1_RXBuff[7]*256 + UART1_RXBuff[8];//总包数
  PacketNum = UART1_RXBuff[9]*256 + UART1_RXBuff[10];//第几包
  RegisterNum = (u16)UART1_RXBuff[4]*256 + UART1_RXBuff[5];//寄存器数量
  //如果总包数超过5包或者第一包寄存器数量小于10
  if((0x05 < PacketSum)||(0x000A > RegisterNum)&&(0 == PacketNum))
  {
    SendLen = 0;
    SendBuf[SendLen++] = (MBASCII_GetSlaveAddr(UART1_RXBuff)) ? UartComm.SlaveAddr : 0x00;
    SendBuf[SendLen++] = MB_FUNC_WRITE_MULTIPLE_REGISTERS;
    SendBuf[SendLen++] = ((u16)UART1_RXBuff[4]*256 + UART1_RXBuff[5]) * 2;

    //返回非法数据地址错误
    MBASCII_SendErr(MB_EX_ILLEGAL_DATA_ADDRESS);
    return;
  }

  //总包数且寄存器数量要小于42
  if((0x01 <= PacketSum)&&(0x05 >= PacketSum)&&(0x002A >= RegisterNum))
  {
    if(PacketNum < 0x05)
    {
      //传输一包数据到缓存
      for(i = 0; i < 80; i++)
      {
	CALIB_Table[PacketNum*80 + i] = UART1_RXBuff[11 + i];
      }
      
      

      //保存一包数据到EEPROM
      I2C1_WriteNBytes(EEPROM_ADDRESS, CALIB_HEIGH_RELATIONSHIP + 80 * PacketNum, 80, &UART1_RXBuff[11]);
      
      RunVar.OilInit = 0;
      //是否是最后一包
      if(PacketSum == (PacketNum + 1))
      {
        Oiltmp = 0;
        ProductPara.OilRange = 0;
        for(i = 0; i < 4; i++)
        {
          Oiltmp = (Oiltmp << 8) + CALIB_Table[4 + i];
        }
                          
        for(i = 0; i < 4; i++)
        {
          j = 80 * PacketNum + 2 * RegisterNum - 16 + i;
//           I2C1_ReadNBytes(EEPROM_ADDRESS, CALIB_HEIGH_RELATIONSHIP + j, 1,  (u8 *)temp);
//            
//           ProductPara.OilRange = (ProductPara.OilRange << 8) + temp;
          
          ProductPara.OilRange = (ProductPara.OilRange << 8) + CALIB_Table[j];
        }
        
        //油量量程  
        
        ProductPara.OilRange -= Oiltmp;
        
        //MinVarOil = (u32)(ProductPara.OilRange*0.01);
                          
        for(i = 4; i > 0; i--)
        {
          opara[4 - i] = (u8)(ProductPara.OilRange >> ((i - 1) * 8));
        }
                          
        I2C1_WriteNBytes(EEPROM_ADDRESS, CALIBOILRAG, 4, opara);
                          
        //j = (5 - PacketSum)*80 + 80 - (RegisterNum - 2)*2;
        //补充油量标定数组的最后一组
        for(i = 0; i < 8; i++)
        {
          j = CALIB_HEIGH_RELATIONSHIP + 80 * PacketNum + 2 * RegisterNum - 4 + i;
          I2C1_WriteByte(EEPROM_ADDRESS, j, 0xFF);
        }
                          
        RunVar.OilInit = 1;
      }
      
      //UART1_RXBuff[6] = UART1_RXBuff[7];
      //UART1_RXBuff[7] = UART1_RXBuff[8];
      for(j = 0; j < 11; j++)
      {
	SendBuf[j] = UART1_RXBuff[j];
      }

      MBASCII_SendMsg(SendBuf,11);
      return;
    }
    /*else
      {
	SendLen = 0;
	SendBuf[SendLen++] = (MBASCII_GetSlaveAddr(UART1_RXBuff)) ? UartComm.SlaveAddr : 0x00;
	SendBuf[SendLen++] = MB_FUNC_WRITE_MULTIPLE_REGISTERS;
	SendBuf[SendLen++] = ((u16)UART1_RXBuff[4]*256 + UART1_RXBuff[5]) * 2;

	MBASCII_SendErr(MB_EX_ILLEGAL_DATA_ADDRESS);
	return;
      }*/
		
  }
  else//否则发送访问地址溢出错误
  {
    SendLen = 0;
    SendBuf[SendLen++] = (MBASCII_GetSlaveAddr(UART1_RXBuff)) ? UartComm.SlaveAddr : 0x00;
    SendBuf[SendLen++] = MB_FUNC_WRITE_MULTIPLE_REGISTERS;
    //SendBuf[SendLen++] = ((u16)UART1_RXBuff[4]*256 + UART1_RXBuff[5]) * 2;
    SendBuf[SendLen++] = 0x02;

    MBASCII_SendErr(MB_EX_ILLEGAL_DATA_ADDRESS);
    return;
  }
}


//**************************************************************************************************
// 名称         	: MBASCII_Fun10()
// 创建日期     	: 2016-09-01
// 作者        	 	: 庄明群
// 功能         	: Modbus 10功能处理
// 输入参数     	: 无
// 输出参数     	: 无
// 返回结果     	: 无
// 注意和说明   	:
// 修改内容     	:
//**************************************************************************************************

void MBASCII_Fun10(void)
{
        u8 i = 0;
	u8 u8Temp[4];
        u8 ReadAdrH;
        u8 ReadAdrL;
        u16 u16Temp;
	u16 index = 0;
        u8 ComTime;
        
	u16 RegisterNum = (u16)UART1_RXBuff[4]*256 + UART1_RXBuff[5];
	
	ReadAdrH = UART1_RXBuff[2];
        ReadAdrL = UART1_RXBuff[3];
        
	SendLen = 0;
    	SendBuf[SendLen++] = (MBASCII_GetSlaveAddr(UART1_RXBuff)) ? UartComm.SlaveAddr : 0x00;
    	SendBuf[SendLen++] = MB_FUNC_WRITE_MULTIPLE_REGISTERS;
    	SendBuf[SendLen++] = RegisterNum * 2;

        //如果读取范围溢出
    if (!(((ReadAdrL >= ASCII_MUL_REG_REGION1_BGEIN) && (ReadAdrL <= ASCII_MUL_REG_REGION1_END)
            && (ReadAdrL + RegisterNum <= (ASCII_MUL_REG_REGION1_END + 1)))
            //|| ((ReadAdrL >= ASCII_MUL_REG_REGION2_BGEIN) && (ReadAdrL <= ASCII_MUL_REG_REGION2_END)
            //    && (ReadAdrL + RegisterNum <= (ASCII_MUL_REG_REGION2_END + 1)))
            || ((ReadAdrL == ASCII_MUL_REG_REGION3_BGEIN))//&&(0x000A <= RegisterNum)&&(0x002A >= RegisterNum))
            || ((ReadAdrL >= ASCII_MUL_REG_EXTERN0_BEGIN)&&(ReadAdrL <= ASCII_MUL_REG_EXTERN0_END)
                &&(ReadAdrL + RegisterNum <= (ASCII_MUL_REG_EXTERN0_END + 1)))                                  //放过广播地址
            )||(0 == RegisterNum)||((RegisterNum * 2) != UART1_RXBuff[6])||((ReadAdrH != UartComm.SlaveAddr) && (ReadAdrH != 0)))
    	{
		MBASCII_SendErr(MB_EX_ILLEGAL_DATA_ADDRESS);
	       return;
	}
        ProductPara.bFlashWEn = FLASH_WR_ENABLE;
        //写油量标定数组
        if(0x80 == ReadAdrL)
        {
            //uprintf("receive OK!\r\n");
          Calibrate_Flag = Bit_SET;
          Write_CALIBAndHeighTable();
          return;
        }
        
	for (long k = 0; k < RegisterNum; ReadAdrL++, k++)
	{
          u16Temp = (u16)UART1_RXBuff[7+index] * 256 + UART1_RXBuff[8+index];
          
          switch(ReadAdrL)
          {
          case 0x30://设备地址
            //if((UART1_RXBuff[8 + index] < 0x41)||(UART1_RXBuff[8 + index] > 0x42))
            //{
              //UartComm.SlaveAddr = 0x41;
            //}
            //else
            //{
              UartComm.SlaveAddr = UART1_RXBuff[8 + index];
            //}
            I2C1_WNBytesMul3T(EEPROM_ADDRESS, SLAVE_ADDR, 1, &UartComm.SlaveAddr);
            break;
            
          case 0x31://波特率
            if((UART1_RXBuff[8 + index] < 0x01)||(UART1_RXBuff[8 + index] > 0x07))
            {
              UartComm.BaudRate = 0x03;
            }
            else
            {
              UartComm.BaudRate = UART1_RXBuff[8+index];
            }
            I2C1_WNBytesMul3T(EEPROM_ADDRESS, BAUDRATE, 1, &UartComm.BaudRate);
            break;
            
          case 0x32://奇偶校验
            if((UART1_RXBuff[8 + index] < 0x01)||(UART1_RXBuff[8 + index] > 0x03))
            {
              UartComm.Parity = 0x03;
            }
            else
            {
              UartComm.Parity = UART1_RXBuff[8+index];
            }
            
            I2C1_WNBytesMul3T(EEPROM_ADDRESS, PARITY, 1, &UartComm.Parity);
            break;
            
          case 0x34://补偿使能
            if((0x00 < UART1_RXBuff[8+index]) && (0x03 > UART1_RXBuff[8+index]))
            {
              ProductPara.CompenEn = UART1_RXBuff[8+index];
            }
            I2C1_WNBytesMul3T(EEPROM_ADDRESS, COMPENSATE, 1, &ProductPara.CompenEn);
            break;
            
          case 0x35://滤波系数
            if((0x00 < UART1_RXBuff[8+index]) && (0x04 > UART1_RXBuff[8+index]))
            {
              UserParam.FilterLevel = UART1_RXBuff[8+index];
              Switch_Fiter(UserParam.FilterLevel);
            }
            I2C1_WNBytesMul3T(EEPROM_ADDRESS, FILTER_LEVEL, 1, &UserParam.FilterLevel);
            break;
            
          case 0x36://自动上传周期
            ProductPara.AutoUpLoadTime = UART1_RXBuff[8+index];
            if ((ProductPara.AutoUpLoadTime < 0x01)||(ProductPara.AutoUpLoadTime > 0x04))
            {
              ProductPara.AutoUpLoadTime = 0x01;
            }
            I2C1_WNBytesMul3T(EEPROM_ADDRESS, AUTO_UPLOAD_TIME, 1, &ProductPara.AutoUpLoadTime);
            break;
            
          case 0x37://修正系数K
            UserParam.UserFlowK = u16Temp / 100.0;
            floatToHexArray(UserParam.UserFlowK, u8Temp, 4);
            I2C1_WNBytesMul3T(EEPROM_ADDRESS, USER_FLOW_K, 4, u8Temp);
            break;
            
          case 0x38://修正系数B
            UserParam.UserFlowB = u16Temp / 100.0;
            floatToHexArray(UserParam.UserFlowB, u8Temp, 4);
            I2C1_WNBytesMul3T(EEPROM_ADDRESS, USER_FLOW_B, 4, u8Temp);
            break;
            
          case 0x39:
            /*if(u16Temp != 0)
            {
                UserParam.FilterM = u16Temp;
                I2C1_WNBytesMul3T(EEPROM_ADDRESS, FILTER_M, 1, &UserParam.FilterM);
            }*/
            break;
            
          case 0x3A:
            /*if(u16Temp != 0)
            {
                UserParam.FilterN = u16Temp;
                I2C1_WNBytesMul3T(EEPROM_ADDRESS, FILTER_N, 1, &UserParam.FilterN);                
            }*/
            break;
            
          case 0x3F://油杆长度
            ProductPara.SensorLen = u16Temp;
            ProductPara.Range = ProductPara.SensorLen - 330;                    //传感器量程=长度-盲区（上下盲区和固定33mm）
            I2C1_WNBytesMul3T(EEPROM_ADDRESS, RANGE, 2, &UART1_RXBuff[7+index]);
            break;
            
          case 0x40://液位比较时间间隔
            ComTime = UART1_RXBuff[8 + index];
            if((ComTime > 0x01) && (ComTime < 0x06))
            {
              ProductPara.LiquidCmpTc = ComTime;
              I2C1_WNBytesMul3T(EEPROM_ADDRESS, LIQUIDCMPTC, 1, &ProductPara.LiquidCmpTc);
            }        
            break;
            
          case 0x41://油料类型
            ProductPara.FuelType = UART1_RXBuff[8 + index];
            if((UART1_RXBuff[8 + index] < 0x01)||(UART1_RXBuff[8 + index] > 0x04))
            {
              ProductPara.FuelType = 0x01;
            }
            I2C1_WNBytesMul3T(EEPROM_ADDRESS, FUELTYPE, 1, &ProductPara.FuelType);
            break;
            
          case 0x42://油箱形状
            ProductPara.BoxShape = UART1_RXBuff[8 + index];
            if((UART1_RXBuff[8 + index] < 0x01)||(UART1_RXBuff[8 + index] > 0x05))
            {
              ProductPara.BoxShape = 0x01;
            }
            I2C1_WNBytesMul3T(EEPROM_ADDRESS, BOXSHAPE, 1, &ProductPara.BoxShape);
            break;
            
          case 0x43://油箱长度
            Cur_Param[BOXPARA0] = UART1_RXBuff[7+index];
            Cur_Param[BOXPARA0 + 1] = UART1_RXBuff[8+index];
            ProductPara.BoxPara[0] = u16Temp;
            I2C1_WNBytesMul3T(EEPROM_ADDRESS, BOXPARA0, 2, &Cur_Param[BOXPARA0]);
            break;
            
          case 0x44://油箱宽度
            Cur_Param[BOXPARA1] = UART1_RXBuff[7+index];
            Cur_Param[BOXPARA1 + 1] = UART1_RXBuff[8+index];
            ProductPara.BoxPara[1] = u16Temp;
            I2C1_WNBytesMul3T(EEPROM_ADDRESS, BOXPARA1, 2, &Cur_Param[BOXPARA1]);
            
            if(ComTime == 0)    //下发比较时间为0，则需要油箱截面积大小，根据截面积自动设置液位比较时间
            {
                if(ProductPara.BoxPara[0] * ProductPara.BoxPara[1] <= 750000)
                {
                    ProductPara.LiquidCmpTc = 3;
                }
                else if(ProductPara.BoxPara[0] * ProductPara.BoxPara[1] <= 1000000)
                {
                    ProductPara.LiquidCmpTc = 4;
                }
                else
                {
                    ProductPara.LiquidCmpTc = 5;
                }
                    
                I2C1_WNBytesMul3T(EEPROM_ADDRESS, LIQUIDCMPTC, 1, &ProductPara.LiquidCmpTc);
            }
            break;
            
          case 0x45://油箱高度
            Cur_Param[BOXPARA2] = UART1_RXBuff[7+index];
            Cur_Param[BOXPARA2 + 1] = UART1_RXBuff[8+index];
            ProductPara.BoxPara[2] = u16Temp;
            I2C1_WNBytesMul3T(EEPROM_ADDRESS, BOXPARA2, 2, &Cur_Param[BOXPARA2]);
            break;
            
          case 0x46://油箱厚度
            Cur_Param[BOXPARA3] = UART1_RXBuff[7+index];
            Cur_Param[BOXPARA3 + 1] = UART1_RXBuff[8+index];
            ProductPara.BoxPara[3] = u16Temp;
            I2C1_WNBytesMul3T(EEPROM_ADDRESS, BOXPARA3, 2, &Cur_Param[BOXPARA3]);
            break;
            
          case 0x47://加油时间阀值，范围0-999
            if(1000 > u16Temp)
            {
              Cur_Param[ADDLIQTIME] = UART1_RXBuff[7+index];
              Cur_Param[ADDLIQTIME + 1] = UART1_RXBuff[8+index];
              ProductPara.AddLiqTime = u16Temp;
              I2C1_WNBytesMul3T(EEPROM_ADDRESS, ADDLIQTIME, 2, &Cur_Param[ADDLIQTIME]);
            }
            break;
            
          case 0x48://加油量阀值，范围1-9999
            if((0 < u16Temp)&&(10000 > u16Temp))
            {
              Cur_Param[ADDLIQLMT] = UART1_RXBuff[7+index];
              Cur_Param[ADDLIQLMT + 1] = UART1_RXBuff[8+index];
              ProductPara.AddLiqCnt = u16Temp;
              I2C1_WNBytesMul3T(EEPROM_ADDRESS, ADDLIQLMT, 2, &Cur_Param[ADDLIQLMT]);
            }
            break;
            
          case 0x49://漏油时间阀值，范围0-999
            if(1000 > u16Temp)
            {
              Cur_Param[SUBLIQTIME] = UART1_RXBuff[7+index];
              Cur_Param[SUBLIQTIME + 1] = UART1_RXBuff[8+index];
              ProductPara.SubLiqTime = u16Temp;
              I2C1_WNBytesMul3T(EEPROM_ADDRESS, SUBLIQTIME, 2, &Cur_Param[SUBLIQTIME]);
            }
            break;
            
          case 0x4A://漏油量阀值，范围1-9999
            if((0 < u16Temp)&&(10000 > u16Temp))
            {
              Cur_Param[SUBLIQLMT] = UART1_RXBuff[7+index];
              Cur_Param[SUBLIQLMT + 1] = UART1_RXBuff[8+index];
              ProductPara.SubLiqCnt = u16Temp;
              I2C1_WNBytesMul3T(EEPROM_ADDRESS, SUBLIQLMT, 2, &Cur_Param[SUBLIQLMT]);
            }
            break;
            
          case 0x70://车速Km
            RunVar.CarSpeed = u16Temp;
            if((0 == RunVar.CarSpeed)&&(65535 > RunVar.LowSpeedCount))
            {
              RunVar.LowSpeedCount++;
            }
            else if(0 != RunVar.CarSpeed)
            {
              RunVar.LowSpeedCount = 0;
            }
            
            if(10 <= RunVar.LowSpeedCount)
            {
              RunVar.FiltSpeed = 0;
            }
            else
            {
              RunVar.FiltSpeed = (RunVar.FiltSpeed + RunVar.CarSpeed) / 2;
            }
            break;
            
          case 0x71://ACC状态
            RunVar.AccStatus = UART1_RXBuff[8+index];
            break;
            
          case 0x72://里程高位
            RunVar.CarMileage &= 0x0000FFFF;
            RunVar.CarMileage |= (u32)(u16Temp << 16);
            break;
            
          case 0x73://里程低位
            RunVar.CarMileage &= 0xFFFF0000;
            RunVar.CarMileage |= u16Temp;
            break;
            
          default:
            break;
          }
          
          index += 2;
        }
        
        ProductPara.bFlashWEn = FLASH_WR_DISABLE;
        
        for(i = 0; i < 6; i++)
        {
          SendBuf[i] = UART1_RXBuff[i];
        }
        
        MBASCII_SendMsg(UART1_RXBuff,6);
}

//**************************************************************************************************
// 名称         	: MBASCII_Fun25()
// 创建日期     	: 2016-10-20
// 作者        	 	: 庄明群
// 功能         	: Modbus 25功能处理
// 输入参数     	: 无
// 输出参数     	: 无
// 返回结果     	: 无
// 注意和说明   	:
// 修改内容     	:
//**************************************************************************************************

void MBASCII_Fun25(void)
{
    u8 ReadAdrH;
    u8 ReadAdrL;
    u8 Data_Buf[4];
    
    ReadAdrH = UART1_RXBuff[2];
    ReadAdrL = UART1_RXBuff[3];
    
    SendLen = 0;
    SendBuf[SendLen++] = (MBASCII_GetSlaveAddr(UART1_RXBuff)) ? UartComm.SlaveAddr : 0x00;
    SendBuf[SendLen++] = 0x25;

    //如果操作范围溢出

    if ((ReadAdrL > ASCII_SPECICAL_COIL_REGION1_END)
        && ((ReadAdrL < ASCII_SPECICAL_COIL_REGION2_BGEIN) || (ReadAdrL > ASCII_SPECICAL_COIL_REGION2_END))
        ||(ReadAdrH != UartComm.SlaveAddr))
    {
        MBASCII_SendErr(MB_EX_ILLEGAL_DATA_ADDRESS);
        return;
    }

    if (!(((UART1_RXBuff[4] == 0xFF) && (UART1_RXBuff[5] == 0x00))
            || ((UART1_RXBuff[4] == 0x00) && (UART1_RXBuff[5] == 0x00))))
    {
        MBASCII_SendErr(MB_EX_ILLEGAL_DATA_VALUE);
        return;
    }

    if (ProductPara.bFlashWEn == FLASH_WR_DISABLE)
    {
        MBASCII_SendErr(MB_EX_SLAVE_DEVICE_FAILURE);
        return;
    }

    switch(ReadAdrL)
    {
    case 0x00:
      if((UART1_RXBuff[4] == 0x00) && (UART1_RXBuff[5] == 0x00))//标定低液位
      {
        Cur_Param[CAPMIN] = (unsigned char)(RunVar.RawCap >> 24);
        Cur_Param[CAPMIN+1] = (unsigned char)(RunVar.RawCap >> 16);
        Cur_Param[CAPMIN+2] =(unsigned char)(RunVar.RawCap >> 8);
        Cur_Param[CAPMIN+3] = (unsigned char)RunVar.RawCap;
        RunVar.CalState |= CalibrateLow;
      }
      else if((UART1_RXBuff[4] == 0xFF) && (UART1_RXBuff[5] == 0x00))//标定高液位
      {
        Cur_Param[CAPMAX] = (unsigned char)(RunVar.RawCap >> 24);
        Cur_Param[CAPMAX+1] = (unsigned char)(RunVar.RawCap >> 16);
        Cur_Param[CAPMAX+2] =(unsigned char)(RunVar.RawCap >> 8);
        Cur_Param[CAPMAX+3] = (unsigned char)RunVar.RawCap;
        RunVar.CalState |= CalibrateHeight;
      }
      CapCalibrate();
      break;
      
    case 0x01:
      if((UART1_RXBuff[4] == 0xFF) && (UART1_RXBuff[5] == 0x00))
      {
        DAOilDebug.bDADebugEn = Bit_SET;
        DAOutDebug.bDADebugEn = Bit_RESET;
      }
      else
      {
        DAOilDebug.bDADebugEn = Bit_RESET;
      }
      break;
      
    case 0x02:
      if((UART1_RXBuff[4] == 0xFF) && (UART1_RXBuff[5] == 0x00))
      {
        DAOutDebug.bDADebugEn = Bit_SET;
        DAOilDebug.bDADebugEn = Bit_RESET;
      }
      else
      {
        DAOutDebug.bDADebugEn = Bit_RESET;
      }
      break;
      
    case 0x03:
      if(Bit_SET == DAOutDebug.bDADebugEn)
      {
        if((UART1_RXBuff[4] == 0x00) && (UART1_RXBuff[5] == 0x00))//零点DA 码
        {
          ProductPara.DAMin = DAOutDebug.usDACode;
          floatToHexArray(ProductPara.DAMin, Data_Buf, 4);
          I2C1_WNBytesMul3T(EEPROM_ADDRESS, DAMIN, 4, Data_Buf);
        }
        else if((UART1_RXBuff[4] == 0xFF) && (UART1_RXBuff[5] == 0x00))//满程DA 码
        {
          ProductPara.DAMax = DAOutDebug.usDACode;
          floatToHexArray(ProductPara.DAMax, Data_Buf, 4);
          I2C1_WNBytesMul3T(EEPROM_ADDRESS, DAMAX, 4, Data_Buf);
        }
      }
      break;
      
    case 0x40:
      NVIC_SystemReset();
      break;
      
    default:
      break;
    }
    
    MBASCII_SendMsg(UART1_RXBuff, 6);
}

//**************************************************************************************************
// 名称         	: MBASCII_Fun26()
// 创建日期     	: 2016-10-20
// 作者        	 	: 庄明群
// 功能         	: Modbus 26功能处理
// 输入参数     	: 无
// 输出参数     	: 无
// 返回结果     	: 无
// 注意和说明   	:
// 修改内容     	:
//**************************************************************************************************

void MBASCII_Fun26(void)
{
    u8 i;
    u8 ReadAdrH;
    u8 ReadAdrL;
    u32 Data_Buf;
    float fTemp = 0;
    
    ReadAdrH = UART1_RXBuff[2];
    ReadAdrL = UART1_RXBuff[3];
    
    u16 Register_Num = (u16)UART1_RXBuff[4] * 256 + UART1_RXBuff[5];
    
    SendLen = 0;
    SendBuf[SendLen++] = (MBASCII_GetSlaveAddr(UART1_RXBuff)) ? UartComm.SlaveAddr : 0x00;
    SendBuf[SendLen++] = 0x26;
    SendBuf[SendLen++] = Register_Num * 2;      //数据长度

    if (!(((ReadAdrL <= ASCII_DB_HOLD_REG_REGION1_END) && (ReadAdrL + Register_Num) <= (ASCII_DB_HOLD_REG_REGION1_END + 2))
               || ((ReadAdrL >= ASCII_DB_HOLD_REG_REGION2_BEGIN) && (ReadAdrL <= ASCII_DB_HOLD_REG_REGION2_END) && (ReadAdrL + Register_Num) <= (ASCII_DB_HOLD_REG_REGION2_END + 2))
               || ((ReadAdrL >= ASCII_DB_HOLD_REG_REGION3_BEGIN) &&(ReadAdrL <= ASCII_DB_HOLD_REG_REGION3_END) && (ReadAdrL + Register_Num) <= (ASCII_DB_HOLD_REG_REGION3_END + 2)))
               || (0 == Register_Num) || (0 != (Register_Num & 0x01)) || (0 != (ReadAdrL & 0x01)) || (ReadAdrH != UartComm.SlaveAddr))
    {
        MBASCII_SendErr(MB_EX_ILLEGAL_DATA_ADDRESS);
        return;
    }
    
    for (long k = 0; k < Register_Num; ReadAdrL += 2, k += 2)
    {
      switch(ReadAdrL)
      {
      case 0x0E:
        fTemp = ProductPara.Floater.Scale[(ReadAdrL - ASCII_DB_HOLD_REG_REGION1_BEGIN) >> 1];
        Data_Buf = *(u32*) & fTemp;
        break;
        
      case 0x1E:
        fTemp = ProductPara.Floater.DACode[(ReadAdrL -(ASCII_DB_HOLD_REG_REGION1_BEGIN + 0x10)) >> 1];
        Data_Buf = *(u32*) & fTemp;
        break;
        
      case 0x80:
        Data_Buf = *(u32*) &  RunVar.RawCap;
        break;
        
      case 0x82:
        Data_Buf = *(u32*) & ProductPara.CapMin;
        break;
        
      case 0x84:
        Data_Buf = *(u32*) & ProductPara.CapMax;
        break;
        
      case 0x86:
        Data_Buf = *(u32*) & ProductPara.DAMin;
        break;
        
      case 0x88:
        Data_Buf = *(u32*) & ProductPara.DAMax;
        break;        
        
      case 0x90:
        Data_Buf = *(u32*) & ProductPara.Temp_K;
        break;
        
      case 0x92:
        Data_Buf = *(u32*) & ProductPara.Temp_B;
        break;
        
      case 0x94:
        Data_Buf = *(u32*) & ProductPara.Temp2_K;
        break;
        
      case 0x96:
        Data_Buf = *(u32*) & ProductPara.Temp2_B;
        break;
        
      case 0xB6:
        if(Bit_SET == DAOilDebug.bDADebugEn)
        {
          Data_Buf = *(u32*) & DAOilDebug.usDACode;
        }
        else if(Bit_SET == DAOutDebug.bDADebugEn)
        {
          Data_Buf = *(u32*) & DAOutDebug.usDACode;
        }
        else
        {
          Data_Buf = 0;
        }
        break;
        
      default:
        Data_Buf = 0;
        break;
      }
      
      for (i = 4; i > 0; i--)
      {
        SendBuf[SendLen++] = (u8)(Data_Buf >> ((i - 1) * 8));
      }
    }

    MBASCII_SendMsg(SendBuf, SendLen);
}

//**************************************************************************************************
// 名称         	: MBASCII_Fun27()
// 创建日期     	: 2016-10-20
// 作者        	 	: 庄明群
// 功能         	: Modbus 27功能处理
// 输入参数     	: 无
// 输出参数     	: 无
// 返回结果     	: 无
// 注意和说明   	:
// 修改内容     	:
//**************************************************************************************************

void MBASCII_Fun27(void)
{
    u8 ReadAdrH;
    u8 ReadAdrL;
    
    u16 Register_Num = (u16)UART1_RXBuff[4] * 256 + UART1_RXBuff[5];
    u32 index = 0;

    ReadAdrH = UART1_RXBuff[2];
    ReadAdrL = UART1_RXBuff[3];
    
    SendLen = 0;
    SendBuf[SendLen++] = (MBASCII_GetSlaveAddr(UART1_RXBuff)) ? UartComm.SlaveAddr : 0x00;
    SendBuf[SendLen++] = 0x27;
    SendBuf[SendLen++] = Register_Num * 2;

    if (!(((ReadAdrL <= ASCII_DB_HOLD_REG_REGION1_END) && (ReadAdrL + Register_Num) <= (ASCII_DB_HOLD_REG_REGION1_END + 2))
               || ((ReadAdrL >= ASCII_DB_HOLD_REG_REGION2_BEGIN) && (ReadAdrL <= ASCII_DB_HOLD_REG_REGION2_END) && (ReadAdrL + Register_Num) <= (ASCII_DB_HOLD_REG_REGION2_END + 2))
               || ((ReadAdrL >= ASCII_DB_HOLD_REG_REGION3_BEGIN) && (ReadAdrL <= ASCII_DB_HOLD_REG_REGION3_END) && (ReadAdrL + Register_Num) <= (ASCII_DB_HOLD_REG_REGION3_END + 2)))
             ||(0 != (Register_Num & 0x01))||(0 == Register_Num)||(0 != (ReadAdrL & 0x01))||((Register_Num * 2) != UART1_RXBuff[6])||(ReadAdrH != UartComm.SlaveAddr))
    {
        MBASCII_SendErr(MB_EX_ILLEGAL_DATA_ADDRESS);
        return;
    }

    if (ProductPara.bFlashWEn == FLASH_WR_DISABLE)
    {
        MBASCII_SendErr(MB_EX_SLAVE_DEVICE_FAILURE);
        return;
    }

    for (long k = 0; k < Register_Num; ReadAdrL += 2, k += 2)
    {
      switch(ReadAdrL)
      {
      case 0x0E:
        if(DAOilDebug.bDADebugEn == Bit_SET)
        {
          ProductPara.Floater.Scale[(ReadAdrL - ASCII_DB_HOLD_REG_REGION1_BEGIN) >> 1] = (unsigned short)HexToFloat(&UART1_RXBuff[7 + index]);
          I2C1_WNBytesMul3T(EEPROM_ADDRESS, CALIB_FLOATER_SCALE_1 + 2 * ((ReadAdrL - ASCII_DB_HOLD_REG_REGION1_BEGIN) >> 1),
                                 2, (u8 *)&(ProductPara.Floater.Scale[(ReadAdrL - ASCII_DB_HOLD_REG_REGION1_BEGIN) >> 1]));
        }
        break;
        
      case 0x1E:
        if(DAOilDebug.bDADebugEn == Bit_SET)
        {
          ProductPara.Floater.DACode[(ReadAdrL - ASCII_DB_HOLD_REG_REGION1_BEGIN - 0x10) >> 1] = (unsigned short)HexToFloat(&UART1_RXBuff[7 + index]);
          I2C1_WNBytesMul3T(EEPROM_ADDRESS, CALIB_FLOATER_DA_1 + 2 * ((ReadAdrL - ASCII_DB_HOLD_REG_REGION1_BEGIN - 0x10) >> 1),
                                 2, (u8 *)&(ProductPara.Floater.DACode[(ReadAdrL -ASCII_DB_HOLD_REG_REGION1_BEGIN - 0x10) >> 1]));
        }
        break;
        
      case 0x80:
        ProductPara.Def_Range = HexToFloat(&UART1_RXBuff[7 + index]);
        I2C1_WNBytesMul3T(EEPROM_ADDRESS, DEF_RANGE, 4, &UART1_RXBuff[7 + index]);
        break;
        
      case 0x82:
        ProductPara.Def_CapMin = HexToFloat(&UART1_RXBuff[7 + index]);
        I2C1_WNBytesMul3T(EEPROM_ADDRESS, DEF_CAPMIN, 4, &UART1_RXBuff[7 + index]);
        break;
        
      case 0x84:
        ProductPara.Def_CapMax = HexToFloat(&UART1_RXBuff[7 + index]);
        I2C1_WNBytesMul3T(EEPROM_ADDRESS, DEF_CAPMAX, 4, &UART1_RXBuff[7 + index]);
        break;
        
      case 0x86:
        ProductPara.DAMin = HexToFloat(&UART1_RXBuff[7 + index]);
        I2C1_WNBytesMul3T(EEPROM_ADDRESS, DAMIN, 4, &UART1_RXBuff[7 + index]);
        break;
        
      case 0x88:
        ProductPara.DAMax = HexToFloat(&UART1_RXBuff[7 + index]);
        I2C1_WNBytesMul3T(EEPROM_ADDRESS, DAMAX, 4, &UART1_RXBuff[7 + index]);
        break;        
        
      case 0x90:
        ProductPara.Temp_K = HexToFloat(&UART1_RXBuff[7 + index]);
        I2C1_WNBytesMul3T(EEPROM_ADDRESS, TEMP_K, 4, &UART1_RXBuff[7 + index]);
        break;
        
      case 0x92:
        ProductPara.Temp_B = HexToFloat(&UART1_RXBuff[7 + index]);
        I2C1_WNBytesMul3T(EEPROM_ADDRESS, TEMP_B, 4, &UART1_RXBuff[7 + index]);
        break;
        
      case 0x94:
        ProductPara.Temp2_K = HexToFloat(&UART1_RXBuff[7 + index]);
        I2C1_WNBytesMul3T(EEPROM_ADDRESS, TEMP2_K, 4, &UART1_RXBuff[7 + index]);
        break;
        
      case 0x96:
        ProductPara.Temp2_B = HexToFloat(&UART1_RXBuff[7 + index]);
        I2C1_WNBytesMul3T(EEPROM_ADDRESS, TEMP2_B, 4, &UART1_RXBuff[7 + index]);
        break;
        
      case 0xB6:
        if(DAOilDebug.bDADebugEn == Bit_SET)
        {
          DAOilDebug.usDACode = HexToFloat(&UART1_RXBuff[7 + index]);
          DA_Write((unsigned short)DAOilDebug.usDACode);
        }
        else if(DAOutDebug.bDADebugEn == Bit_SET)
        {
          DAOutDebug.usDACode = HexToFloat(&UART1_RXBuff[7 + index]);
          DA2_Write((unsigned short)DAOutDebug.usDACode);
        }
        break;
        
      default:
        break;
      }
      
      index += 4;
    }

    MBASCII_SendMsg(UART1_RXBuff, 6);
}

//**************************************************************************************************
// 名称         	: MBASCII_Fun2A()
// 创建日期     	: 2016-10-20
// 作者        	 	: 庄明群
// 功能         	: 写版本信息
// 输入参数     	: 无
// 输出参数     	: 无
// 返回结果     	: 无
// 注意和说明   	:
// 修改内容     	:
//**************************************************************************************************

void MBASCII_Fun2A(void)
{
    u8 i,j,objlen;
    u8 ReadAdr = UART1_RXBuff[3];
    u8 Register_Num = UART1_RXBuff[5];
    SendLen = 0;

    SendBuf[SendLen++] = (MBASCII_GetSlaveAddr(UART1_RXBuff)) ? UartComm.SlaveAddr : 0x00;
    SendBuf[SendLen++] = 0x2A;
    SendBuf[SendLen++] = UART1_RXBuff[2];
    if(!((UART1_RXBuff[2] == UartComm.SlaveAddr)
       &&(ReadAdr >= ASCII_PRODUCTION_INF_BEGIN)
       &&(ReadAdr <= ASCII_PRODUCTION_INF_END)
       &&((ReadAdr + Register_Num) <= (ASCII_PRODUCTION_INF_END + 1))
       &&(0 != Register_Num)&&(0x00 == UART1_RXBuff[4])))
    {
      MBASCII_SendErr(MB_EX_ILLEGAL_DATA_ADDRESS);
      return;
    }
    j = 6;
    ProductPara.bFlashWEn = FLASH_WR_ENABLE;
    for(i = 0; i < Register_Num; i++)
    {
      switch(ReadAdr)
      {
      case 0xE0://机构名称
        objlen = UART1_RXBuff[j] + 1;
        I2C1_WriteNBytes(EEPROM_ADDRESS, COMPANY, objlen, &UART1_RXBuff[j]);                                    
        j += objlen;
        ReadAdr++;
        break;
        
      case 0xE1://产品代号
        objlen = UART1_RXBuff[j] + 1;
        I2C1_WriteNBytes(EEPROM_ADDRESS, DEV_ENCODING, objlen, &UART1_RXBuff[j]);                               
        j += objlen;
        ReadAdr++;
        break;
        
      case 0xE2://硬件版本
        objlen = UART1_RXBuff[j] + 1;
        I2C1_WriteNBytes(EEPROM_ADDRESS, SENSOR_HV, objlen, &UART1_RXBuff[j]);                                  
        j += objlen;
        ReadAdr++;
        break;
        
      case 0xE3://软件版本
        objlen = UART1_RXBuff[j] + 1;
        //I2C1_WriteNBytes(EEPROM_ADDRESS, SENSOR_SV, objlen, &UART1_RXBuff[j]);                                  
        j += objlen;
        ReadAdr++;
        break;
        
      case 0xE4://设备ID
        objlen = UART1_RXBuff[j] + 1;
        I2C1_WriteNBytes(EEPROM_ADDRESS, DEV_ID, objlen, &UART1_RXBuff[j]);                                     
        j += objlen;
        ReadAdr++;
        break;
        
      case 0xE5://客户编码
        objlen = UART1_RXBuff[j] + 1;
        I2C1_WriteNBytes(EEPROM_ADDRESS, CUSTOMERCODE, objlen, &UART1_RXBuff[j]);                               
        j += objlen;
        ReadAdr++;
        break;
        
      default:
        break;
      }
    }
    ProductPara.bFlashWEn = FLASH_WR_DISABLE;
    
    MBASCII_SendMsg(UART1_RXBuff, 6);
}

//**************************************************************************************************
// 名称         	: MBASCII_Fun2B()
// 创建日期     	: 2016-10-20
// 作者        	 	: 庄明群
// 功能         	: 读版本信息
// 输入参数     	: 无
// 输出参数     	: 无
// 返回结果     	: 无
// 注意和说明   	:
// 修改内容     	:
//**************************************************************************************************

void MBASCII_Fun2B(void)
{
  u8 i,objlen;
  u8 ReadAdr = UART1_RXBuff[3];
  u8 Register_Num = UART1_RXBuff[5];
  SendLen = 0;

  SendBuf[SendLen++] = (MBASCII_GetSlaveAddr(UART1_RXBuff)) ? UartComm.SlaveAddr : 0x00;
  SendBuf[SendLen++] = 0x2B;
  SendBuf[SendLen++] = UART1_RXBuff[4];
  SendBuf[SendLen++] = UART1_RXBuff[5];
  if(!((UART1_RXBuff[2] == UartComm.SlaveAddr)
       &&(ReadAdr >= ASCII_PRODUCTION_INF_BEGIN)
       &&(ReadAdr <= ASCII_PRODUCTION_INF_END)
       &&((ReadAdr + Register_Num) <= (ASCII_PRODUCTION_INF_END + 1))
       &&(0 != Register_Num)&&(0x00 == UART1_RXBuff[4])))
  {
    MBASCII_SendErr(MB_EX_ILLEGAL_DATA_ADDRESS);
    return;
  }
  for(i = 0; i < Register_Num; i++)
  {
    switch(ReadAdr)
    {
    case 0xE0://机构名称
      SendBuf[SendLen++] = 0xE0;
      I2C1_ReadNBytes(EEPROM_ADDRESS, COMPANY, 1, &objlen);                                                       
      SendBuf[SendLen++] = objlen;
      if((objlen > (DATAPACKET_LEN/3))||(0 == objlen))
      {
        SendBuf[SendLen - 1] = 1;
        SendBuf[SendLen++] = 0;
        break;
      }
      I2C1_ReadNBytes(EEPROM_ADDRESS, COMPANY + 1, objlen, &SendBuf[SendLen]);
      SendLen += objlen;
      //ReadAdr++;
      break;
      
    case 0xE1://产品代号
      SendBuf[SendLen++] = 0xE1;
      I2C1_ReadNBytes(EEPROM_ADDRESS, DEV_ENCODING, 1, &objlen);                                                  
      SendBuf[SendLen++] = objlen;
      if((objlen > (DATAPACKET_LEN/3))||(0 == objlen))
      {
        SendBuf[SendLen - 1] = 1;
        SendBuf[SendLen++] = 0;
        break;
      }
      I2C1_ReadNBytes(EEPROM_ADDRESS, DEV_ENCODING + 1, objlen, &SendBuf[SendLen]);
      SendLen += objlen;
      //ReadAdr++;
      break;
      
    case 0xE2://硬件版本
      SendBuf[SendLen++] = 0xE2;
      I2C1_ReadNBytes(EEPROM_ADDRESS, SENSOR_HV, 1, &objlen);                                                     
      SendBuf[SendLen++] = objlen;
      if((objlen > (DATAPACKET_LEN/3))||(0 == objlen))
      {
        SendBuf[SendLen - 1] = 1;
        SendBuf[SendLen++] = 0;
        break;
      }
      I2C1_ReadNBytes(EEPROM_ADDRESS, SENSOR_HV + 1, objlen, &SendBuf[SendLen]);
      SendLen += objlen;
      //ReadAdr++;
      break;
    
    case 0xE3://软件版本
      SendBuf[SendLen++] = 0xE3;
      
      SendBuf[SendLen++] = 0x07;
      
      for(objlen = 0; objlen < 7; objlen++)
      {
        SendBuf[SendLen++] = SensorSoftVersion[objlen + 1];
      }
      
      //SendLen += objlen;
      
      //ReadAdr++;
      break;
      
    case 0xE4:
      SendBuf[SendLen++] = 0xE4;
      I2C1_ReadNBytes(EEPROM_ADDRESS, DEV_ID, 1, &objlen);                                                        //设备ID
      SendBuf[SendLen++] = objlen;
      if((objlen > (DATAPACKET_LEN/3))||(0 == objlen))
      {
        SendBuf[SendLen - 1] = 1;
        SendBuf[SendLen++] = 0;
        break;
      }
      I2C1_ReadNBytes(EEPROM_ADDRESS, DEV_ID + 1, objlen, &SendBuf[SendLen]);
      SendLen += objlen;
      //ReadAdr++;
      break;
      
    case 0xE5:
      SendBuf[SendLen++] = 0xE5;
      I2C1_ReadNBytes(EEPROM_ADDRESS, CUSTOMERCODE, 1, &objlen);                                                  //客户编码
      SendBuf[SendLen++] = objlen;
      if((objlen > (DATAPACKET_LEN/3))||(0 == objlen))
      {
        SendBuf[SendLen - 1] = 1;
        SendBuf[SendLen++] = 0;
        break;
      }
      I2C1_ReadNBytes(EEPROM_ADDRESS, CUSTOMERCODE + 1, objlen, &SendBuf[SendLen]);
      SendLen += objlen;
      //ReadAdr++;
      break;
      
    default:
      break;
    }
    
    ReadAdr++;
  }
  
  MBASCII_SendMsg(SendBuf, SendLen);
}

//**************************************************************************************************
// 名称         	: MBASCII_Fun41()
// 创建日期     	: 2016-10-20
// 作者        	 	: 庄明群
// 功能         	: 在线升级功能
// 输入参数     	: 无
// 输出参数     	: 无
// 返回结果     	: 无
// 注意和说明   	:
// 修改内容     	:
//**************************************************************************************************

void MBASCII_Fun41(void)
{
  u16 ReadAdr = (u16)UART1_RXBuff[2] * 256 + UART1_RXBuff[3];
  u16 DataLen = (u16)UART1_RXBuff[4] * 256 + UART1_RXBuff[5];
  
  SendLen = 0;
  SendBuf[SendLen++] = (MBASCII_GetSlaveAddr(UART1_RXBuff)) ? UartComm.SlaveAddr : 0x00;
  SendBuf[SendLen++] = 0x41;
  SendBuf[SendLen++] = DataLen;
  
  if(0x0001 != ReadAdr)
  {
    MBASCII_SendErr(MB_EX_ILLEGAL_DATA_ADDRESS);
    return;
  }
  UART1_RXBuff[5] = 0x01;
  
  if(0x0000 != DataLen)
  {
    UART1_RXBuff[6] = 0x01;
    MBASCII_SendMsg(UART1_RXBuff, 7);
    return;
  }
  
  UART1_RXBuff[6] = 0x00;
  MBASCII_SendMsg(UART1_RXBuff, 7);
  Onlineflg = 0x0C;
  ProductPara.bFlashWEn = FLASH_WR_ENABLE;
  I2C1_WriteByte(EEPROM_ADDRESS, ONLINEUPGRADE, 0x0C);
  I2C1_WNBytesMul3T(EEPROM_ADDRESS, SLAVE_ADDR, 1, &UartComm.SlaveAddr);
  ProductPara.bFlashWEn = FLASH_WR_DISABLE;
  
  if(0x0C == Onlineflg)
  {
    Delay_Ms(20);
    NVIC_SystemReset();
  }
}
//**************************************************************************************************
// 名称         	: MBASCII_Function()
// 创建日期   	        : 2016-09-01
// 作者         	: 庄明群
// 功能         	: ModbusASCII码解析
// 输入参数     	: 无
// 输出参数     	: 无
// 返回结果     	: 无
// 注意和说明   	:
// 修改内容     	:
//**************************************************************************************************

void MBASCII_Function(void)
{
    u8 RecvLen = 0;
    if(2 == MODBUS_ASCII_RecvData(UART1_RXBuff, &RecvLen))//校验错误
    {
      return;
    }
    if(RecvLen <= 0)
    {
      return;
    }
    else if((UartComm.SlaveAddr != MBASCII_GetSlaveAddr(UART1_RXBuff))&&
            (MB_ADDRESS_BROADCAST != MBASCII_GetSlaveAddr(UART1_RXBuff)))
    {
      return;
    }
    else
    {
        switch (MBASCII_GetFunCode(UART1_RXBuff))
        {
        case MB_FUNC_READ_HOLDING_REGISTER:	//读单个寄存器0x03
          MBASCII_Fun03();
          break;
            
        case MB_FUNC_READ_INPUT_REGISTER:		//读两个寄存器0x04
          MBASCII_Fun04();
          break;
            
        case MB_FUNC_WRITE_SINGLE_COIL:		//写单个线圈
          MBASCII_Fun05();
          break;
            
        case MB_FUNC_WRITE_MULTIPLE_REGISTERS://写多个寄存器0x10
          if((RecvLen - 7) != UART1_RXBuff[6])
          {
            return ;
          }
          MBASCII_Fun10();
          break;
          
        case 0x25:
          MBASCII_Fun25();
          break;
          
        case 0x26:
          MBASCII_Fun26();
          break;
          
        case 0x27:
          MBASCII_Fun27();
          break;
          
        case 0x2A:
          MBASCII_Fun2A();
          break;
          
        case 0x2B:
          MBASCII_Fun2B();
          break;
          
        case 0x41:
          MBASCII_Fun41();
          break;
          
	default:
          SendLen = 0;
          SendBuf[SendLen++] = MBASCII_GetSlaveAddr(UART1_RXBuff);
          SendBuf[SendLen++] = 0x80 | (u8)MBASCII_GetFunCode(UART1_RXBuff);
          SendBuf[SendLen++] = MB_EX_ILLEGAL_FUNCTION;
          MBASCII_SendMsg(SendBuf, SendLen);
	  break;
        }
    }  
}



//**************************************************************************************************
// 名称         	: Get_TankOil()
// 创建日期   	        : 2017-10-26
// 作者         	: 庄明群
// 修改人               :
// 功能         	: 得到油量(根据标定的液位高度对应的油量计算油量)
// 输入参数     	: 无
// 输出参数     	: 无
// 返回结果     	: 无
// 注意和说明   	:
// 修改内容     	:
//**************************************************************************************************
//u32 dwEepromHeightADValue;                                                      //标定的  油量数组液位高度
//u32 dwEepromOilValue;                                                           //标定的  油量数组油量
//u32 dwUpEepromHeightADValue;                                                    //上一个油量数组液位高度
//u32 dwUpEepromOilValue;                                                         //上一个油量数组油量
//u32 dwTankOilValue;                                                             //当前油箱油量
//float dwTable_k;                                                               //油量关系斜率
u32 Get_TankOil(u32 cyLiquidHeightAD)
{
  //u32 dwTable_B;
  u16 countK;
  u16 i;
  
  u32 EepromHeightADValue_0,EepromHeightADValue_1;
  u32 EepromOilValue_0, EepromOilValue_1;
  float rate;
  //u8 temp=0;
   //u8 opara[4];
  
  for(i = 0; i < 50; i++)
  {
    countK = (i << 3);    
    dwEepromHeightADValue = CALIB_Table[countK++];
    dwEepromHeightADValue = (dwEepromHeightADValue << 8) + CALIB_Table[countK++];
    dwEepromHeightADValue = (dwEepromHeightADValue << 8) + CALIB_Table[countK++];
    dwEepromHeightADValue = (dwEepromHeightADValue << 8) + CALIB_Table[countK++];

    dwEepromOilValue = CALIB_Table[countK++];
    dwEepromOilValue = (dwEepromOilValue << 8) + CALIB_Table[countK++];
    dwEepromOilValue = (dwEepromOilValue << 8) + CALIB_Table[countK++];
    dwEepromOilValue = (dwEepromOilValue << 8) + CALIB_Table[countK++];
    
    if(0 == i)
    {
      dwUpEepromHeightADValue = dwEepromHeightADValue;
      dwUpEepromOilValue = dwEepromOilValue;
    }
       
    if(cyLiquidHeightAD <= dwEepromHeightADValue)
    {        
      if(0x07FFFFFF > dwEepromHeightADValue)
      {
        if(0 == i)
        {
          if(cyLiquidHeightAD < dwEepromHeightADValue )  //如果当前液位高度值小于第一组标定值的第一个数，往前插值计算当前油量
          {    
            //例如
            //cyLiquidHeightAD = 24.8           dwTankOilValue = ?  21.7465
            //EepromHeightADValue_0 = 29.7      EepromOilValue_0 = 26.3
            //EepromHeightADValue_1 = 59.4      EepromOilValue_1 = 53.9
            
            EepromHeightADValue_0 = dwEepromHeightADValue;     
             
            EepromHeightADValue_1 = CALIB_Table[8];
            EepromHeightADValue_1 = (EepromHeightADValue_1 << 8) + CALIB_Table[9];
            EepromHeightADValue_1 = (EepromHeightADValue_1 << 8) + CALIB_Table[10];
            EepromHeightADValue_1 = (EepromHeightADValue_1 << 8) + CALIB_Table[11];
            
            EepromOilValue_0 = dwEepromOilValue;
            
            EepromOilValue_1 = CALIB_Table[12];
            EepromOilValue_1 = (EepromOilValue_1 << 8) + CALIB_Table[13];
            EepromOilValue_1 = (EepromOilValue_1 << 8) + CALIB_Table[14];
            EepromOilValue_1 = (EepromOilValue_1 << 8) + CALIB_Table[15];        
           
             
            rate = (float)(EepromOilValue_1 - EepromOilValue_0)/(float)(EepromHeightADValue_1 - EepromHeightADValue_0);
            dwTankOilValue = EepromOilValue_0 - (u32)(EepromHeightADValue_0 - cyLiquidHeightAD) * rate ;
          }
          else
          {
               dwTankOilValue = dwEepromOilValue;          
          }
        }
        else
        {
          dwTable_k = (float)(dwEepromOilValue - dwUpEepromOilValue)/(float)(dwEepromHeightADValue - dwUpEepromHeightADValue);
          dwTankOilValue = (u32)((cyLiquidHeightAD - dwUpEepromHeightADValue) * dwTable_k + dwUpEepromOilValue);
        }
      }
      else
      {
        dwTankOilValue = dwUpEepromOilValue;
      }
      break;
    }
    else
    {
      if(49 == i)
      {
        dwTankOilValue = dwEepromOilValue;
        break;
      }
      dwUpEepromHeightADValue = dwEepromHeightADValue;
      dwUpEepromOilValue = dwEepromOilValue;
    }
  }
  return dwTankOilValue;
}

//**************************************************************************************************
// 名称         	: MBASCII_AutoUpLoadFrame()
// 创建日期   	        : 2016-09-01
// 作者         	: 庄明群
// 功能         	: 自动上传
// 输入参数     	: 无
// 输出参数     	: 无
// 返回结果     	: 无
// 注意和说明   	:
// 修改内容     	:
//**************************************************************************************************

void MBASCII_AutoUpLoadFrame(void)
{
	u8 i;
	u32 pTemp;
        
        SendBuf[0] = UartComm.SlaveAddr;
        SendBuf[1] = MB_FUNC_READ_INPUT_REGISTER;
        SendBuf[2] = 32;

        pTemp = (u32)RunVar.LiquidAD;
       
	for (i = 4; i > 0; i--)
    	{
          SendBuf[7 - i] = (u8)(pTemp >> ((i - 1) * 8));
    	}
        
        pTemp = (u32)((RunVar.TempInSensor + 273.15) * 10);                                           //液体温度
        for(i = 4; i > 0; i--)
        {
          SendBuf[11 - i] = (u8)(pTemp >> ((i - 1) * 8));
        }
        
        pTemp = (u32)((RunVar.TempInAir + 273.15) * 10);                                              //环境温度
        for(i = 4; i > 0; i--)
        {
          SendBuf[15 - i] = (u8)(pTemp >> ((i - 1) * 8));
        }
        

         pTemp = ProductPara.AddOil;   
        for(i = 4; i > 0; i--)
        {
          SendBuf[19 - i] = (u8)(pTemp >> ((i - 1) * 8));                                             //加油量
        }
        

        pTemp = ProductPara.SubOil;  
        for(i = 4; i > 0; i--)
        {
          SendBuf[23 - i] = (u8)(pTemp >> ((i - 1) * 8));                                             //漏油量
        }
        
        pTemp = RunVar.OilQuantity; 
	for (i = 4; i > 0; i--)
        {
          SendBuf[27 - i] = (u8)(pTemp >> ((i - 1) * 8));
        }
        
        
        pTemp = RunVar.LiquidPercent;                                             //液位百分比
        for (i = 4; i > 0; i--)
        {
          SendBuf[31 - i] = (u8)(pTemp >> ((i - 1) * 8));
        }
        
        SendLen = 35;
        pTemp = (u32)RunVar.LiquidHeight;                                                        //液位高度      
        
        for (i = 4; i > 0; i--)
        {
          SendBuf[35 - i] = (u8)(pTemp >> ((i - 1) * 8));
        }
        
	MBASCII_SendMsg_NoLimit(SendBuf, SendLen);
}




void  APP_MB_Task(void *p_arg)
{
    OS_ERR      os_err;
    u32 UartRunCnt = 0;
    u32 UartRunCntPre = 0;

    UART1_Init(&UartComm);
    
    RunVar.uartInitTime = OSTimeGet(&os_err);

    while (1)
    {
        OSTaskSemPend((OS_TICK)UART_WAIT_TIME,
                      (OS_OPT)OS_OPT_PEND_BLOCKING,
                      (CPU_TS *)0,
                      (OS_ERR *)&os_err);

#if IWDG_EN > 0
        IWDG_ReloadCounter();
#endif
        if(Bit_SET == RunVar.bInitializing)
        {
            RunVar.bInitializing = Bit_RESET;
        }
        if (os_err == OS_ERR_NONE)
        {          
          MBASCII_Function();
        }
        else if (os_err == OS_ERR_TIMEOUT)
        {
            UartRunCnt += UART_WAIT_TIME;
            
            if ((ProductPara.AutoUpLoadTime >= 0x02)&&(ProductPara.AutoUpLoadTime <= 0x04))
            {
		if ((UartRunCnt - UartRunCntPre) >= ((u32)(ProductPara.AutoUpLoadTime -1) * 10000))
		{
		  MBASCII_AutoUpLoadFrame();
		  UartRunCntPre = UartRunCnt;
		}
            }
            else
            {
              UartRunCntPre = UartRunCnt;
            }
        }
        
         
          
    }
}