/*
* SIM900A Interfacing with PIC18F4550
* http://www.electronicwings.com
*/


#include <pic18f4550.h>
#include "Configuration_Header_File.h"
#include "LCD_16x2_8-bit_Header_File.h"
#include "USART_Header_File.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>



void GSM_Init();
void GSM_Calling(char *);
void GSM_HangCall();
void GSM_Response();
void GSM_Response_Display();
void GSM_Msg_Read(int);
int GSM_Wait_for_Msg();
void GSM_Msg_Display();
void GSM_Msg_Delete(unsigned int);
void GSM_Send_Msg(const char* , const char*);


char buff[160];                             /* buffer to store responses and messages */
volatile char status_flag;                  /* monitor to check for any new message */
volatile int a;
unsigned int mobile_no[14];
unsigned char msg_received[60];
unsigned int position=0;
void main(void) 
{
    a=0;
    int is_message_arrived=0;
    OSCCON =0x72;       /* set internal oscillator Freq = 8 MHz*/
    LCD_Init();         /* initialize LCD */
    INTCONbits.GIE=1;   /* enable Global Interrupt */
    INTCONbits.PEIE=1;  /* enable Peripheral Interrupt */
    PIE1bits.RCIE=1;    /* enable Receive Interrupt */
    MSdelay(100);
    USART_Init(9600);   /* initialize USART communication */        
    GSM_Init();         /* check GSM responses and initialize GSM */
    
//    LCD_Clear();
//    LCD_String("Sending SMS...");
//    MSdelay(3000);
////    
//    GSM_Send_Msg("8007332284","TEST");  /*send sms on "mobile no."*/
//    MSdelay(3000);
    LCD_Clear();
    while(1)
    {   
        LCD_Clear();
        /* waiting for message to receive */
        LCD_String_xy(0,0,"waiting for msg");
    //    status_flag=0;
        if(status_flag==1)
        {
            LCD_Clear();
//            LCD_String_xy(0,0,"New msg arrive");
//            MSdelay(1000);
        //    LCD_Clear();
            is_message_arrived = GSM_Wait_for_Msg();                 /*wait for new message to read */
            if(is_message_arrived==1)
            {
                LCD_String_xy(0,0,"New msg arrive");
                GSM_Msg_Read(position);
            }
            MSdelay(3000);
            is_message_arrived==0;            
            status_flag = 0;            
        }
    MSdelay(1000);
    }
  //  while(1);
}


void GSM_Init()
{   
    while(1)
    {   
        LCD_Command(0xc0);
        USART_SendString("ATE0\r");   /* send AT to check module is ready or not*/
        MSdelay(500);
        if(strstr(buff,"OK"))
        {
            GSM_Response();                 /* find response and display it on LCD16x2 */
            memset(buff,0,160);
            break;
        }
        else
        {    
            LCD_String("Error");
        }
    }
    MSdelay(2000);

    LCD_Clear();
	LCD_String_xy(0,0,"Text Mode"); 
    LCD_Command(0xc0);
	USART_SendString("AT+CMGF=1\r");/* select message format as text */
    GSM_Response();
	MSdelay(2000);
    
    LCD_Clear();
	LCD_String_xy(0,0,"Mfd name");  
    LCD_Command(0xc0);
	USART_SendString("AT+GMI\r");   /* identify manufacturer */
    GSM_Response();
	MSdelay(2000);
    
    LCD_Clear();
	LCD_String_xy(0,0," Model No.");
    LCD_Command(0xc0);
	USART_SendString("AT+GMM\r");   /* find model no. */
    GSM_Response();
	MSdelay(2000);
    
	LCD_Clear();
	LCD_String_xy(0,0,"  IMEI No. ");
    LCD_Command(0xc0);
	USART_SendString("AT+GSN\r");   /* find IMEI no. of module */
    GSM_Response();
	MSdelay(2000);
        
    LCD_Clear();
	LCD_String_xy(0,0,"Service Provider");
    MSdelay(1000);
    LCD_Clear();
    LCD_Command(0xc0);
	USART_SendString("AT+CSPN?\r"); /* find service provider name */
    GSM_Response();
	MSdelay(2000);

}

void GSM_Msg_Delete(unsigned int position)
{
    
    a=0;
    char delete_cmd[20];    
    sprintf(delete_cmd,"AT+CMGD=%d\r",position);    /* delete message at specified position */
    USART_SendString(delete_cmd);
    MSdelay(100);
    memset(buff,0,strlen(buff));
}
int GSM_Wait_for_Msg()
{
    char i,val[4];
    
    LCD_Clear();
    a=0;
    while(1)
    {
        if(buff[a]==0x0d || buff[a]==0x0a) /*eliminate "\r \n" which is start of string */
        {
            a++;
        }
        else
            break;
    }
    
    if(strstr(buff,"CMTI:"))               /* check if any new message received */
    {   
        while(buff[a]!=',')
        {
            a++;
        }
        a++;
        
//        LCD_Command(0x80);
//        LCD_String("Position: ");        /* display position of message received */
        i=0;
        while(buff[a]!=0x0d)
        {
            val[i]=buff[a];
//            LCD_Char(buff[a]);           /* display position of new message where it is stored */
            a++;
            i++;
        } 
    position = atoi(val);
    if(position>20)
    {
        LCD_String_xy(0,0,"Msg mem full");
        memset(buff,0,strlen(buff));
        return 2;
    }
    memset(buff,0,strlen(buff));
    a=0;
      return 1;
//    GSM_Msg_Read(position);             /* read message which is recently arrived from position */
    }
    else
    {
        return 0;
    }
//    status_flag = 0; 
}

void interrupt ISR()
{
    if(RCIF)
    {
        buff[a] = RCREG;                /* read received byte from serial buffer */
        a++;
        
        if(RCSTAbits.OERR)              /* check if any overrun occur due to continuous reception */
        {           
            CREN = 0;
            NOP();
            CREN=1;
        }
        status_flag=1;                  /* use for new message arrival */
    }

}

void GSM_Send_Msg(const char *num,const char *sms)
{
    int i;
    char sms_buffer[35];
    a=0;
    sprintf(sms_buffer,"AT+CMGS=\"%s\"\r",num);
    USART_SendString(sms_buffer);               /*send command AT+CMGS="Mobile No."\r */
    MSdelay(200);
    while(1)
    {
        if(buff[a]==0x3e)                       /* wait for '>' character*/
        {
            a=0;
            memset(buff,0,strlen(buff));
            USART_SendString(sms);              /* send msg to given no. */
            USART_TxChar(0x1a);                 /* send Ctrl+Z then only message will transmit*/
            break;
        }  
        a++;
    }        
    MSdelay(300);
    a=0;
    memset(buff,0,strlen(buff));
    memset(sms_buffer,0,strlen(sms_buffer));
}

void GSM_Calling(char *mobile)
{
    char call[20];
    sprintf(call,"ATD%s;\r",mobile);		/* send command ATD8007xxxxxx; for calling*/
    USART_SendString(call);    
}

void GSM_HangCall()
{
    USART_SendString("ATH\r");			/*send command ATH\r to hang call*/
}

//void GSM_Response()
//{
//    unsigned int timeout=0;
//    int CRLF_Found=0;
//    char CRLF_buff[2];
//    int Response_Length=0;
//    while(1)
//    {
//        if(timeout>=60000)			/*if timeout occur then return */
//            return;
//        Response_Length = strlen(buff);		
//        if(Response_Length)
//        {
//            MSdelay(1);
//            timeout++;
//            if(Response_Length==strlen(buff))	
//            {
//                for(int i=0;i<Response_Length;i++)
//                {
//                    memmove(CRLF_buff,CRLF_buff+1,1);
//                    CRLF_buff[1]=buff[i];
//                    if(strncmp(CRLF_buff,"\r\n",2))
//                    {
//                        if(CRLF_Found++==2)	/* search for \r\n in string */
//                        {
//                            GSM_Response_Display();
//                            return;
//                        }
//                    }
//
//                }
//                CRLF_Found =0;
//
//            }
//        
//    }
//    MSdelay(1);
//    timeout++;
// }
//}

void GSM_Response(){
    unsigned int timeout=0;
    int Response_Length=0;
//    while(1){
//       if(timeout>=60000)			/*if timeout occur then return */
//            return;
//       Response_Length
        MSdelay(500);
        GSM_Response_Display();
  //  }
}
    

void GSM_Response_Display()
{
    a=0;
    int lcd_pointer=0;
    while(1)
    {
        if(buff[a]==0x0d || buff[a]==0x0a)
        {
            a++;
        }
        else
            break;
    }
    LCD_String_xy(1,0,"                   ");
    LCD_Command(0xc0);
    while(buff[a]!=0x0d)
    {  
        LCD_Char(buff[a]);
        a++;
        lcd_pointer++;
        if(lcd_pointer==15)
            LCD_Command(0x80);
    }
    a=0;
    
        memset(buff,0,strlen(buff));
        
}

void GSM_Msg_Read(int position)
{
    int i,k;
    char flag,read_cmd[10];
    i=0;
    sprintf(read_cmd,"AT+CMGR=%d\r",position);
    USART_SendString(read_cmd);                 
    MSdelay(1000);
    GSM_Msg_Display();
}

void GSM_Msg_Display()
{
    unsigned int m=0;
    if(!(strstr(buff,"+CMGR")))                 /*check for +CMGR response */
    {
            LCD_String_xy(0,0,"No message");   
    }   
    else
    {   
        a=0;
        
        while(1)
        {
            if(buff[a]==0x0d || buff[a]==0x0a)  /*wait till \r\n not over*/
            {
                a++;
            }
            else
                break;
        }
//        while(buff[a]!=0x3a)                    /*wait till string not equal to ':' */
//        {
//            a++;
//        }
        while(buff[a]!=',')
        {
            a++;
        }
        a=a+2;
        while(buff[a]!=0x22)
        {
            mobile_no[m] = buff[a];
            m++;
        }
     do
        {        
            a++;
        }while(buff[a-1]!=0x0a);
        
        LCD_Command(0xC0);
        int i=0;
            while(buff[a]!=0x0d && i<31)
            {
                    msg_received[i]=buff[a];
                    LCD_Char(buff[a]);
                    a++;
                    i++;
                    if(i==16)               /* if received message is greater than 16(for display message)*/
                        LCD_Command(0x80);  /* resume display of message from 1st line */
                    
            }
        
            a=0;
            memset(buff,0,strlen(buff));
    }
    status_flag = 0;
}

