// ================================================ 电源连接 ================================================ //
//   VCC         接          DC 5V/3.3V      // OLED 模块电源正
//   GND         接          GND             // OLED 模块电源地
// ================================================ 硬件引脚连接说明 ================================================ //
// 本模块默认通信方式为 4 线 SPI
//   OLED 模块
//   D0          接          GPIO53          // OLED 用 SPI 时时钟引脚           // 复用 SPI CLK
//   MISO        接          GPIO54          // OLED 用 SPI 读数据引脚           // 复用 SPI MISO  单片机读取数据
//   MOSI        接          GPIO55          // OLED 用 SPI 写数据引脚           // 复用 SPI MOSI  单片机写入数据
//   CS          接          GPIO56          // OLED 片选控制引脚               // 复用 SPI CSN0
//   RES         接          GPIO40          // OLED 复位控制引脚。             // 普通 GPIO
//   DC          接          GPIO39          // OLED 数据/命令选择控制引脚     // 普通 GPIO
//串口连线：GND-GND,1C102X, IO9-4G R1C102 IO8-4G TX。即1C102 IO 9是1C102的TX,1C102 IO 8是1C102的RX, 
//Adc_open(ADC_CHANNEL_1V); //启用具体的ADC通道
//Adc_open(ADC_CHANNEL_I5);   //I2 是当前CPU电压，I3是1v的标准电压，I4-I7是用户可以用的，IO口依次是14,15,16,17
/*
还需要进行的工作，将ADC的值转化为真实的物理值，并尽可能的求出平均值
*/




int getPHvalue();
int getRvalue();
int getDvalue();
int getWvalue();
int get_max(int arr[], int size);
int get_min(int arr[], int size);
int get_sum(int arr[], int size);   //注意，获得值的时候一定要有时间间隔，ADC采样时间间隔不能太短否则不准。
int ds18b20_get_temp(void); //
unsigned char ds18b20_read_byte(void); //
int ds18b20_read_bit(void); //
void ds18b20_write_byte(unsigned char data);//
void ds18b20_write_bit(int bit);//
int ds18b20_reset(void);


#include "ls1x.h"
#include "Config.h"
#include "ls1x_spi.h"
#include "ls1x_gpio.h"
#include "ls1x_latimer.h"
#include "ls1x_uart.h"
#include "ls1x_printf.h"
#include "ls1c102_adc.h"    //很多数据类型还都需要修改，先不管，统一用int
#include "string.h"
#include "ls1x_flash.h"

#define DS18B20_PIN 35
#define BUFFER_SIZE 20

//#define FLASH_PAGE_SIZE        256    // 假设Flash页大小为256字节
#define FLASH_START_ADDRESS    0x000000
#define FLASH_MAX_SIZE         (1024*1024) // 假设Flash总大小1MB

/* 添加全局变量声明 */
static uint32_t current_flash_addr = FLASH_START_ADDRESS;

typedef struct __attribute__((packed)) {
  uint32_t timestamp;
  int32_t RV_MAX;
  int32_t RV_MIN;
  int32_t RV_sum;
  int32_t DV_MAX;
  int32_t DV_MIN;
  int32_t DV_sum;
  int32_t WV_MAX;
  int32_t WV_MIN;
  int32_t WV_sum;
  int32_t PV_MAX;
  int32_t PV_MIN;
  int32_t PV_sum;
} FlashRecord;



int times=0;
int alertTime=00000000;  //记录报警时间，避免短时间内多次报警
//char alertText[100]="warning:xx值于xx年xx月xx日xx时超出阈值，设定阈值为：xx，测量值为xx";
//只需要发送的信息有：时间？什么东西？设定值？测量值？
char alertText[100];
char TimeText[100];
char PRDWText[100];
char SetVText[100];
char GetVText[100];
int temp=25;

char cmd[BUFFER_SIZE + 1]; // +1 for null terminator  
int P_lowline=400,P_highline=7999,    //报警阈值，具体初始阈值还需要根据实际修改
    R_lowline=400,R_highline=7999,
    W_lowline=400,W_highline=7999,
    D_lowline=400,D_highline=7999;

int alertFlag[4]={0,0,0,0};

int main(int arg, char *args[])
{   

    

    gpio_set_direction(20, 1);   //设置GPIO20为输出模式（置1） 接板载LED,使用gpio_write_pin(20,1)设置高电平（置1）亮
    Uart1_init(115200);//串口初始化
    gpio_set_direction(DS18B20_PIN, 1);  // 初始化为输出模式 
    gpio_write_pin(DS18B20_PIN, 1);    

   
    gpio_write_pin(20, 0);delay_ms(150);gpio_write_pin(20, 1);delay_ms(150); gpio_write_pin(20, 0);delay_ms(150);gpio_write_pin(20, 1);delay_ms(150); gpio_write_pin(20, 0);delay_ms(150);gpio_write_pin(20, 1);delay_ms(150);
   
        

    while(1)
    {        memset(cmd, 0, sizeof(cmd));                                                           //01  23 45 67 89   01  23 45  67  89                             
      for(int i = 0; i < BUFFER_SIZE; i++)   //通过串口获取指令 ，指令格式12  25 03 04 13   xx  AA aa  BB  bb
       {
         cmd[i] = UART_ReceiveData(UART1);
         }
         cmd[BUFFER_SIZE] = '\0'; 
     // myprintf2(1, "指令接收确认%s\n", cmd);   //通过串口发送信息

        if (cmd[0]=='3'&&cmd[1]=='2')
        {
          gpio_set_direction(DS18B20_PIN, 1); // 输出模式
          gpio_write_pin(DS18B20_PIN, 0);     // 先拉低
          delay_ms(100);
          gpio_set_direction(DS18B20_PIN, 0); // 切换为输入模式
          
          // 检查它是否自动回到高电平
          int dq_status = gpio_get_pin(DS18B20_PIN);
          myprintf2(1, "DQ 线状态: %d\n", dq_status);

        }

        if (cmd[0]=='2'&&cmd[1]=='2')//温度测试
        { myprintf2(1, "温度测试开始");  
 
          temp=ds18b20_get_temp();
          myprintf2(1, "温度是%d\n", temp);  
        }

        if (cmd[0]=='4'&&cmd[1]=='4')//验证delay
        {
          delay_ms(1);
          myprintf2(1, "1\n");  
          delay_ms(1);
          myprintf2(1, "1\n");  
          delay_ms(1);
          myprintf2(1, "1\n");  
          delay_ms(500);
          myprintf2(1, "500");
          delay_ms(500);
          myprintf2(1, "500"); 
          delay_ms(500);
          myprintf2(1, "500"); 
          delay_ms(2000);
          myprintf2(1, "2000");
          delay_ms(2000);
          myprintf2(1, "2000");
          delay_ms(2000);
          myprintf2(1, "2000");
        }

        if (cmd[0]=='8'&&cmd[1]=='8')//亮LED以测试通信
        {
          
        }
        
        if (cmd[0]=='0'&&cmd[1]=='8')//灭LED以测试通信
        {gpio_write_pin(20, 0);}

        if (cmd[0]=='9'&&cmd[1]=='9')//亮灭LED以测试某IO口是否正常输入读取高低电平，顺利读高则亮LED，顺利读低则灭LED，
        {
        gpio_set_direction(DS18B20_PIN, 0);//1输出模式，0输入模式
        int flag=gpio_get_pin(DS18B20_PIN);
        myprintf2(1, "%d\n", flag);  
          if(flag==1)//是高电平
          {
            gpio_write_pin(20, 1);
          }
          if(flag==0)//是低电平
          {
            gpio_write_pin(20, 0);
          }
        }

        if (cmd[0]=='0'&&cmd[1]=='0')  //指令00，获取最新信息，只读取发送，不记录，人要看，所有要发
        {  
          if (cmd[12]=='0')
          {
            myprintf2(1, "wl:%d,wh:%d,pl:%d,ph:%d,dl:%d,dh:%d,rl:%d,rh:%d",W_lowline,W_highline,P_lowline,P_highline,D_lowline,D_highline,R_lowline,R_highline);
          }
          
          Adc_powerOn();
          int PV=0,RV=0,DV=0,WV=0,NV=0;    
          PV=getPHvalue();
          RV=getRvalue();
          DV=getDvalue();
          WV=getWvalue();
          Adc_powerOff();
        
          int year = (cmd[2] - '0') * 10 + (cmd[3] - '0');
          int month = (cmd[4] - '0') * 10 +(cmd[5] - '0');
          int day = (cmd[6] - '0') * 10 + (cmd[7] - '0');
          int hour =(cmd[8] - '0') * 10 + (cmd[9] - '0');
          int cmdTime=hour+day*100+month*10000+year*1000000;

if( alertTime!=cmdTime )
{
            if ((P_lowline>PV||PV>P_highline)  ) //不在范围内则发送报警信息，且当前时间不是上次报警时间1小时内
            {  
              alertFlag[0]=1;
                
            }
            if ((R_lowline>RV||RV>R_highline)  )
            {           
              alertFlag[1]=1;    
              
            }
            if ((D_lowline>DV||DV>D_highline)    )
            {          
              alertFlag[2]=1;     
             
            }
            if ((W_lowline>WV||WV>W_highline)  )
            {        
              alertFlag[3]=1;       
           
            }

            delay_ms(300);
         if (get_sum(alertFlag,4)==0)  {   }

         if ( get_sum(alertFlag,4)==1)
         {
         if (alertFlag[0]==1)
         {
          myprintf2(1, "warning:PH值于20%d年%d月%d日%d时超出阈值，设定阈值为：%d 到 %d，测量值为%d", year, month, day, hour,P_lowline,P_highline,PV); alertFlag[0]=0; 
        
         }
         if (alertFlag[1]==1)
         {
          myprintf2(1, "warning:溶解度值于20%d年%d月%d日%d时超出阈值，设定阈值为：%d 到 %d，测量值为%d", year, month, day, hour,R_lowline,R_highline,RV); alertFlag[1]=0;
         }

         if (alertFlag[2]==1)
         {
          myprintf2(1, "warning:电导率值于20%d年%d月%d日%d时超出阈值，设定阈值为：%d 到 %d，测量值为%d", year, month, day, hour,D_lowline,D_highline,DV); alertFlag[2]=0;
         }

         if (alertFlag[3]==1)
         {
          myprintf2(1, "warning:污浊度值于20%d年%d月%d日%d时超出阈值，设定阈值为：%d 到 %d，测量值为%d", year, month, day, hour,W_lowline,W_highline,WV);    alertFlag[3]=0;
         }
         }

         if (get_sum(alertFlag,4)!=0 && get_sum(alertFlag,4)!= 1) 
         {
          myprintf2(1, "warning:多个参数超出阈值，请及时处理.");  alertFlag[0]=0;alertFlag[1]=0;alertFlag[2]=0;alertFlag[3]=0;
         }
          
          
            ///alertTime=cmdTime;
  }
   
              delay_ms(350);
               myprintf2(1, "'newdata:w:%d,p:%d,d:%d,r:%d'",WV,PV,DV,RV);

        }

        if (cmd[0]=='0'&&cmd[1]=='1') //指令01，定时自动获取信息，只读取处理记录，不发送，因为没人看，记录就好
        {int dPV[3],dRV[3],dDV[3],dWV[3],h1,h2;    //且需要根据阈值判断要不要报警
          h1=cmd[8] - '0';
          h2=cmd[9] - '0';
          times=(h1*10+h2-1)/2;     //t是当天的记录次数0-11
          dPV[times]=getPHvalue();
          dRV[times]=getRvalue();
          dDV[times]=getDvalue();
          dWV[times]=getWvalue();

          int year = (cmd[2] - '0') * 10 + (cmd[3] - '0');
          int month = (cmd[4] - '0') * 10 + (cmd[5] - '0');
          int day = (cmd[6] - '0') * 10 + (cmd[7] - '0');
          int hour = (cmd[8] - '0') * 10 + (cmd[9] - '0');
          int cmdTime=hour+day*100+month*10000+year*1000000;
          
          myprintf2(1, "记录数据次数:%d\n",times+1);
          myprintf2(1, "记录数据时间:%d\n",hour);  


          if( alertTime!=cmdTime )
          { 
                      if ((P_lowline>dPV[times]||dPV[times]>P_highline)  ) //不在范围内则发送报警信息，且当前时间不是上次报警时间1小时内
                      {  
                        alertFlag[0]=1;
                          
                      }
                      if ((R_lowline>dRV[times]||dRV[times]>R_highline)  )
                      {           
                        alertFlag[1]=1;    
                        
                      }
                      if ((D_lowline>dDV[times]||dDV[times]>D_highline)    )
                      {          
                        alertFlag[2]=1;     
                       
                      }
                      if ((W_lowline>dWV[times]||dWV[times]>W_highline)  )
                      {        
                        alertFlag[3]=1;       
                     
                      }
          
                      delay_ms(300);
                   if (get_sum(alertFlag,4)==0)  {   }
          
                   if (get_sum(alertFlag,4)==1)
                   {
                   if (alertFlag[0]==1)
                   {
                    myprintf2(1, "warning:PH值于20%d年%d月%d日%d时超出阈值，设定阈值为：%d 到 %d，测量值为%d", year, month, day, hour,P_lowline,P_highline,dPV[times]);   alertFlag[0]=0;
                   }
                   if (alertFlag[1]==1)
                   {
                    myprintf2(1, "warning:溶解度值于20%d年%d月%d日%d时超出阈值，设定阈值为：%d 到 %d，测量值为%d", year, month, day, hour,R_lowline,R_highline,dRV[times]);  alertFlag[1]=0;
                   }
          
                   if (alertFlag[2]==1)
                   {
                    myprintf2(1, "warning:电导率值于20%d年%d月%d日%d时超出阈值，设定阈值为：%d 到 %d，测量值为%d", year, month, day, hour,D_lowline,D_highline,dDV[times]);  alertFlag[2]=0;
                   }
          
                   if (alertFlag[3]==1)
                   {
                    myprintf2(1, "warning:污浊度值于20%d年%d月%d日%d时超出阈值，设定阈值为：%d 到 %d，测量值为%d", year, month, day, hour,W_lowline,W_highline,dWV[times]);     alertFlag[3]=0;
                   }
                   }
          
                   if (get_sum(alertFlag,4)!=0 && get_sum(alertFlag,4)!= 1) 
                   { 
                    myprintf2(1, "warning:多个参数超出阈值，请及时处理");   alertFlag[0]=0; alertFlag[1]=0; alertFlag[2]=0; alertFlag[3]=0;
                   }
                    
                    
                      ///alertTime=cmdTime;
            }
          

       //   为啥是和呢？因为连浮点运算都搞的巨麻烦，搞不懂，到EC600N上或微信上在算就是。  //当天最后一次数据记录完毕后进行统计
        
   // 组织数据结构
   
       
       if (times==3)           //4次，    好测试，之后记得改回11，即12次。     
         {   times=0;
          int PVMAX=get_max(dPV, 3);  
          int PVMIN=get_min(dPV, 3); 
          int PVsum=get_sum(dPV, 4); 

          int RVMAX=get_max(dRV, 3); 
          int RVMIN=get_min(dRV, 3); 
          int RVsum=get_sum(dRV, 4);

          int DVMAX=get_max(dDV, 3); 
          int DVMIN=get_min(dDV, 3); 
          int DVsum=get_sum(dDV, 4);

          int WVMAX=get_max(dWV, 3); 
          int WVMIN=get_min(dWV, 3); 
          int WVsum=get_sum(dWV, 4); 

          int m=day+month*100+year*10000;
        
          day++;
         
          //测试历史记录
          FlashRecord record = {
            .timestamp = m,
            .RV_MAX = RVMAX,
            .RV_MIN = RVMIN,
            .RV_sum = RVsum,
            .DV_MAX = DVMAX,
            .DV_MIN = DVMIN,
            .DV_sum = DVsum,
            .WV_MAX = WVMAX,
            .WV_MIN = WVMIN,
            .WV_sum = WVsum,
            .PV_MAX = PVMAX,
            .PV_MIN = PVMIN,
            .PV_sum = PVsum
        };
               
       

      uint8_t write_buffer[FLASH_PAGE_SIZE] = {0};
      memcpy(write_buffer, &record, sizeof(FlashRecord));


      if (current_flash_addr + FLASH_PAGE_SIZE > FLASH_MAX_SIZE) {
        myprintf2(1, "Flash storage full!\n");
        return -1;
    }



    if (ls1c_flash_write2(current_flash_addr, write_buffer, FLASH_PAGE_SIZE)) {
      myprintf2(1, "已记录一天历史数据");
      myprintf2(1, "Record ssumd at 0x%06X\n", current_flash_addr);
      current_flash_addr += FLASH_PAGE_SIZE; // 移动到下一页
  } else {
      myprintf2(1, "Flash write failed at 0x%06X\n", current_flash_addr);
      return -1;
  }



         }
         
        
      }

                                                                      //01  23 45 67  89                              01               23 45      67  89    
                                      //                               指令  年 月 日  时  用于修改阈值     什么要修改：xx  修改后：最小值AA.aa  最大值BB.bb
        if (cmd[0]=='0'&&cmd[1]=='2') //指令02，修改报警阈值    PRWD
        { 
          
          if (cmd[11]=='1')
          {
            P_lowline=1000*(cmd[12]-'0')+100*(cmd[13]-'0')+10*(cmd[14]-'0')+(cmd[15]-'0');      // 最小值
            P_highline=1000*(cmd[16]-'0')+100*(cmd[17]-'0')+10*(cmd[18]-'0')+(cmd[19]-'0');     //最大值
          }
          if (cmd[11]=='2')
          {
            R_lowline=1000*(cmd[12]-'0')+100*(cmd[13]-'0')+10*(cmd[14]-'0')+(cmd[15]-'0'); 
            R_highline=1000*(cmd[16]-'0')+100*(cmd[17]-'0')+10*(cmd[18]-'0')+(cmd[19]-'0'); 
          }
          if (cmd[11]=='3')
          {
            W_lowline=1000*(cmd[12]-'0')+100*(cmd[13]-'0')+10*(cmd[14]-'0')+(cmd[15]-'0'); 
            W_highline=1000*(cmd[16]-'0')+100*(cmd[17]-'0')+10*(cmd[18]-'0')+(cmd[19]-'0'); 
          }
          if (cmd[11]=='4')
          {
            D_lowline=1000*(cmd[12]-'0')+100*(cmd[13]-'0')+10*(cmd[14]-'0')+(cmd[15]-'0'); 
            D_highline=1000*(cmd[16]-'0')+100*(cmd[17]-'0')+10*(cmd[18]-'0')+(cmd[19]-'0'); 
          }
          myprintf2(1, "修改完成");
         
        }

                                                                      //01  23 45 67  89                                  01               23 45      67  89  
                                      //                               指令  年 月 日  时    读取历史数据 读取哪一个的数据：xx  
        if (cmd[0]=='0'&&cmd[1]=='3')//指令03，读取历史数据        读取 ：P R W D  01 02 03 04
        { 
          switch (cmd[11])   
        {
        case '1':        //指令1读取ph，
        {
          uint32_t read_addr = (current_flash_addr > FLASH_START_ADDRESS) ? 
          (current_flash_addr - FLASH_PAGE_SIZE) : 0;
           while (read_addr >= FLASH_START_ADDRESS) 
           {
           uint8_t read_buffer[FLASH_PAGE_SIZE];
           FlashRecord record;

           if (FLASH_ReadPage(read_addr, read_buffer, FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE) {// 读取整页数据
           myprintf2(1, "Read failed at 0x%06X\n", read_addr);
           break;
           }

           memcpy(&record, read_buffer, sizeof(FlashRecord));// 提取数据结构

           myprintf2(1, "'时间:%u,MAX:%d,MIN:%d,SUM:%d,'",// 发送PH数据
              record.timestamp,
              record.PV_MAX,
              record.PV_MIN,
              record.PV_sum);


           if (read_addr == FLASH_START_ADDRESS) break;    // 翻到前一页
           read_addr = (read_addr >= FLASH_PAGE_SIZE) ? 
                 (read_addr - FLASH_PAGE_SIZE) : 0;
                delay_ms(330);
           }
           break;
        }   
        
        case '2':  //指令2，读取溶解度 
        {
          uint32_t read_addr = (current_flash_addr > FLASH_START_ADDRESS) ? 
          (current_flash_addr - FLASH_PAGE_SIZE) : 0;
           while (read_addr >= FLASH_START_ADDRESS) 
           {
           uint8_t read_buffer[FLASH_PAGE_SIZE];
           FlashRecord record;

           if (FLASH_ReadPage(read_addr, read_buffer, FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE) {// 读取整页数据
           myprintf2(1, "Read failed at 0x%06X\n", read_addr);
           break;
           }

           memcpy(&record, read_buffer, sizeof(FlashRecord));// 提取数据结构

           myprintf2(1, "'时间:%u,MAX:%d,MIN:%d,SUM:%d,'",// 发送溶解度数据
              record.timestamp,
              record.RV_MAX,
              record.RV_MIN,
              record.RV_sum);


           if (read_addr == FLASH_START_ADDRESS) break;    // 翻到前一页
           read_addr = (read_addr >= FLASH_PAGE_SIZE) ? 
                 (read_addr - FLASH_PAGE_SIZE) : 0;
                 delay_ms(330);
           }
           break;

        }  
       
        case '3'://指令3，读取污浊度
         {
          uint32_t read_addr = (current_flash_addr > FLASH_START_ADDRESS) ? 
          (current_flash_addr - FLASH_PAGE_SIZE) : 0;
           while (read_addr >= FLASH_START_ADDRESS) 
           {
           uint8_t read_buffer[FLASH_PAGE_SIZE];
           FlashRecord record;

           if (FLASH_ReadPage(read_addr, read_buffer, FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE) {// 读取整页数据
           myprintf2(1, "Read failed at 0x%06X\n", read_addr);
           break;
           }

           memcpy(&record, read_buffer, sizeof(FlashRecord));// 提取数据结构

           myprintf2(1, "'时间:%u,MAX:%d,MIN:%d,SUM:%d,'",// 发送污浊度
              record.timestamp,
              record.WV_MAX,
              record.WV_MIN,
              record.WV_sum);


           if (read_addr == FLASH_START_ADDRESS) break;    // 翻到前一页
           read_addr = (read_addr >= FLASH_PAGE_SIZE) ? 
                 (read_addr - FLASH_PAGE_SIZE) : 0;
                 delay_ms(330);
           }
           break;

        }  
     
        case '4': //指令4，读取电导率
        {
          uint32_t read_addr = (current_flash_addr > FLASH_START_ADDRESS) ? 
          (current_flash_addr - FLASH_PAGE_SIZE) : 0;
           while (read_addr >= FLASH_START_ADDRESS) 
           {
           uint8_t read_buffer[FLASH_PAGE_SIZE];
           FlashRecord record;

           if (FLASH_ReadPage(read_addr, read_buffer, FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE) {// 读取整页数据
           myprintf2(1, "Read failed at 0x%06X\n", read_addr);
           break;
           }

           memcpy(&record, read_buffer, sizeof(FlashRecord));// 提取数据结构

           myprintf2(1, "'时间:%u,MAX:%d,MIN:%d,SUM:%d,'",// 发送电导率数据
              record.timestamp,
              record.DV_MAX,
              record.DV_MIN,
              record.DV_sum);


           if (read_addr == FLASH_START_ADDRESS) break;    // 翻到前一页
           read_addr = (read_addr >= FLASH_PAGE_SIZE) ? 
                 (read_addr - FLASH_PAGE_SIZE) : 0;
          delay_ms(330);
           }
           break;

        }  
        default:
          break;
        }

        }
       
    }
    return 0;
}


/************************************************************************************************/

int ds18b20_reset(void) 
{
    int presence;
    gpio_set_direction(DS18B20_PIN, 1);  // 设置为输出
    gpio_write_pin(DS18B20_PIN, 1);  
    delay_us(10);     
    gpio_write_pin(DS18B20_PIN, 0);    // 拉低总线  
    delay_us(520);                 // 保持480us以上
    gpio_set_direction(DS18B20_PIN, 0); // 释放总线
    delay_us(30);                  // 等待15-60us

    presence = gpio_get_pin(DS18B20_PIN);  // 检测应答脉冲
    myprintf2(1, "DS18B20 Reset Presence: %d,0说明引脚为低电\n", presence); // 添加调试信息
    delay_us(40);                // 等待整个时隙完成
    return presence;
}

void ds18b20_write_bit(int bit)
{
    gpio_set_direction(DS18B20_PIN, 1);
    gpio_write_pin(DS18B20_PIN, 0);                  // 拉低总线
    delay_us(2);                  // 拉低至少1us
  
    if (bit == 1) {
        gpio_set_direction(DS18B20_PIN, 0); // 释放总线
        myprintf2(1, "IO%d写了bit:1\n", DS18B20_PIN); // 添加调试信息
    } else {
        delay_us(90);                 // 保持60-120us
        gpio_set_direction(DS18B20_PIN, 0); // 释放总线
        delay_us(1);                  // 恢复时间
        myprintf2(1, "IO%d写了bit:0\n", DS18B20_PIN);
    }
}

/* 写一个字节到DS18B20 */
void ds18b20_write_byte(unsigned char data)
{
    int i;
    myprintf2(1, "尝试写入: 0x%02X\n", data);
    for (i = 0; i < 8; i++) {
        ds18b20_write_bit(data & 0x01);
        data >>= 1;
    }
}

/* 从DS18B20读取一个位 */
int ds18b20_read_bit(void) 
{
    int bit = 0;
    gpio_set_direction(DS18B20_PIN, 1);
    gpio_write_pin(DS18B20_PIN, 0);
    delay_us(2);                  // 拉低至少1us
  
    gpio_set_direction(DS18B20_PIN, 0); // 释放总线
    delay_us(13);                 // 等待15us后采样
  
    bit = gpio_get_pin(DS18B20_PIN);
    myprintf2(1, "IO%d 读取 Bit: %d\n", DS18B20_PIN, bit);
  
    delay_us(55);                 // 等待整个时隙完成
    return bit;
}

/* 从DS18B20读取一个字节 */
unsigned char ds18b20_read_byte(void) 
{
    unsigned char data = 0;
    int i;
    for (i = 0; i < 8; i++) {
        data >>= 1;
        if (ds18b20_read_bit()) {
            data |= 0x80;
        }
    }
    myprintf2(1, "IO%d读到了Byte: 0x%02X\n", DS18B20_PIN, data); // 添加调试信息
    return data;
}


int ds18b20_get_temp(void) 
{
    unsigned char temp_l, temp_h;
    int temp;
    ds18b20_reset();
    ds18b20_write_byte(0xCC);  // 跳过ROM
    ds18b20_write_byte(0x44);  // 启动温度转换

    delay_ms(1000);
    ds18b20_reset();
    ds18b20_write_byte(0xCC);  // 跳过ROM
    ds18b20_write_byte(0xBE);  // 读取暂存器
   int status = ds18b20_read_byte();
   myprintf2(1, "读取转换状态: 0x%02X\n", status);
    temp_l = ds18b20_read_byte();
    temp_h = ds18b20_read_byte();

    myprintf2(1, "Raw Data: temp_l=0x%02X, temp_h=0x%02X\n", temp_l, temp_h);
  
    // 合并温度值
    temp = (temp_h << 8) | temp_l;
  
    // 处理负温度
    if (temp & 0x8000) {
        temp = -( (temp ^ 0xFFFF) + 1 );
    }
  
    // 转换为实际温度的100倍（扩大100倍返回整数）
    int temperature = temp *  625 / 100;
    return (int)(temperature); 
}


int getPHvalue()  //获取PH值                     只获得了AD电压值，还有具体算法，温度修正要写
{  int  PHvalue=0,U1,U;
   Adc_open(ADC_CHANNEL_I7);
   //temp=ds18b20_get_temp();//温度对PH影响比较小
   delay_ms(100);
   U1=Adc_Measure(ADC_CHANNEL_I7);
   delay_ms(150);  
   Adc_close(ADC_CHANNEL_I7);
   delay_ms(50);
   //PHvalue=k*U1+b;     电压和PH线性关系，K和b需要通过实验得出，k一般是-5.5到-6，b一般是15到25
   U=U1*((2*temp+50)/100);
   PHvalue=U;    // 电压和PH线性关系，K和b需要通过实验得出，k一般是-5.5到-6，b一般是15到25
   return PHvalue;
  
}

int getRvalue()  //获取溶解度值。电导率通过溶解度利用经验公式算出来             只获得了AD电压值，还有具体算法，温度修正要写
{   int  Rvalue=0,U1,U;
   Adc_open(ADC_CHANNEL_I5);
   //temp=ds18b20_get_temp();//获取温度用于修正
   delay_ms(100);
   U1=Adc_Measure(ADC_CHANNEL_I5);
   delay_ms(150);  
   Adc_close(ADC_CHANNEL_I5);
   delay_ms(50);
   //U=U1*((2*temp+50)/100);
   Rvalue=U1;
   //Rvalue=(67*U*U*U/1000000000-128*U*U/1000000+429*U/1000)*10/10;//  2，0.9是个常数，需要利用标准液体定值测量修正
   return Rvalue;
}

int getDvalue()  //获取电导率值。电导率通过溶解度利用经验公式算出来        
{   int  Dvalue=0,rv;
   Dvalue=2*getRvalue();   
   return Dvalue;
}
 
int getWvalue()  //获取污浊度值                只获得了AD电压值，还有具体算法，温度修正要写
{  int  Wvalue=0,U1,U; 
  Adc_open(ADC_CHANNEL_I4);
//  temp=ds18b20_get_temp();//获取温度用于修正
  delay_ms(100);
  U1=Adc_Measure(ADC_CHANNEL_I4);
  delay_ms(150);  
  Adc_close(ADC_CHANNEL_I4);
  delay_ms(50);
  U=U1*((2*temp+50)/100);
 Wvalue=-866*U/1000+3300+246;  //2，3300是个常数，需要利用标准液体定值测量修正,但基本上就是3300左右偏差不会大,246是初步修正的修正参数
  return Wvalue;
}



int get_max(int arr[], int size)  //获取最大值 
{    
    int max = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    return max;
}

int get_min(int arr[], int size) //获取最小值 
{
    int min = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] < min) {
            min = arr[i];
        }
    }
    return min;
}

int get_sum(int arr[], int size)   //获取和
{
  int sum = 0;
  for (int i = 0; i < size; i++) {
      sum += arr[i];
  }
  return sum ;
}

