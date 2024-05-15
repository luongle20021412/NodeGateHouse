#include "main.h"
#include "RFID_RCC522.h"
#include "MY_NRF24.h"
#include "LCD20X4.h"
#include "FLASH.h"

typedef enum{
	Page3 = 0x00,
	Page2 = 0x02,
	Page1 = 0x01
}PageActiveType;	

typedef enum{
	Do_Not_Card = 0,
	Slave_Card,
	Master_Card,
}CardIDStateType;

typedef enum{
	Do_Nothing = 0,
	AddSlaveCard,
	RemoveSlaveCard,
	CreatPass,
	ChangePass
}MasterCardStateType;

typedef struct {
	char ID;
	uint8_t status;
	uint8_t count;
}Data_Node_Gate_t;

uint8_t buffer[32];
uint8_t CardID[5];

uint8_t Slaver_CardID[20][5];
uint8_t Name_Slaver[20][3];
uint8_t CheckName[4];
uint8_t Pass_Home[7];
uint8_t Pass_Change[7];
uint8_t XacNhanLai[7];
uint8_t Enter_Pass[7];
uint8_t Display_Name_slave[4];

static uint8_t Master_CardID[5] = {0xE3, 0x9B, 0x33, 0x25, 0x6E};
static PageActiveType PageActive = Page1;
static CardIDStateType CardID_Status = Do_Not_Card;
static MasterCardStateType Master_Status = Do_Nothing;
static Data_Node_Gate_t Gate;
uint8_t checkPage;
uint64_t Address_NodeGate = 0x1122334400;
uint32_t lastDelay = 0;
uint16_t countName = 0;
uint8_t ucountName = 0;
uint8_t ucountCheck = 0;
bool last_count;
uint8_t countPass = 0, countCard = 0;
uint8_t key;
uint8_t buffer[32];
bool Display_Add, Display_Remove;
bool Old_Add_Remove;
bool Old_PassID;
bool EnterThePass;
bool Display_Pass_OK, Display_Pass_ERR,Display_Loop;
bool Correct_Pass,ERROR_Pass, Display_CheckPass, Display_CheckPassHome;
bool Send_ToGateWay;
char Send_TextDisplayGateWay = 1;
SPI_HandleTypeDef hspi1;
TIM_HandleTypeDef htim4;

	
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM4_Init(void);
static void MX_SPI1_Init(void);

void Read_FLashFrom(void);
void Check_CardID(void);
void Check_Add_Remove(void);
void Add_RemoveCard(void);
void Check_Creat_Change_PassID(void);
void Creat_Change_PassID(void);
void Slave_Name_Add(void);
void Check_PassID(void);
char keyPad(void);
void Display_Total(void);
void Display_PassHome(void);
void Display_Add_Remove(void);
void Display_Creat_Change_Pass(void);
void Display_Check_Add_Remove(void);
void Display_CheckPass_Corret_Error(void);
void Display_Creat_Change_Pass_OK_ERR(void);
void Display_Slave_Name(void);
void Door_StepMotor(void);

int main(void)
{

  HAL_Init();

  SystemClock_Config();
	
  MX_GPIO_Init();
  MX_TIM4_Init();
  MX_SPI1_Init();
  LCD20X4_Init();
  MFRC522_Init();
  HAL_TIM_Base_Start(&htim4);
  /* NRF */
  NRF24_Begin(hspi1,SPI1_Port,SPI1_CS_PIN,SPI1_CE_PIN);
  NRF24_OpenWritingPipe(Address_NodeGate);
  NRF24_OpenReadingPipe(1,Address_NodeGate);
  NRF24_SetPALevel(RF24_PA_0dB);
  NRF24_SetDataRate(RF24_250KBPS);
  NRF24_SetChannel(52);
  NRF24_EnableDynamicPayloads();
  NRF24_SetAutoAck(true);
  NRF24_SetRetries(10,15);
  NRF24_StartListening();
  
  lastDelay = HAL_GetTick();
  /* Flash */
  Read_FLashFrom();
  /* Send Data to Gatway */
  Gate.ID = 'A';
  NRF24_StopListening();
  NRF24_Write(&Gate,sizeof(Gate));
  NRF24_StartListening();
  while (1)
  {
	  Check_CardID();
	  Check_PassID();
	  Door_StepMotor();
	  if(Correct_Pass){
			if(key == 0x41 && Gate.count != 20){
				Gate.status = 1;
			}else if(key == 0x42 && Gate.count != 0){
				Gate.status =3;
			}else{
				if(Gate.status == 1) Gate.status = 2;
				if(Gate.status == 3) Gate.status = 4;
			}
			if( key == 0x44){
				Correct_Pass =!  Correct_Pass;
			}
			Display_CheckPass_Corret_Error();
	  }else{
		  Display_Total();
	  }
	  Check_Creat_Change_PassID();
	  Check_Add_Remove();
	  if(((CardID_Status == Master_Card || CardID_Status == Slave_Card) && !Send_ToGateWay)){
		  Gate.ID = 'A';
		  NRF24_StopListening();
		  NRF24_Write(&Gate,sizeof(Gate));
		  NRF24_StartListening();
		  Flash_Erase(Page_addres_Gate);
		  Flash_Write_Array(Page_addres_Gate + 2,(uint8_t *)&Gate, 4);
		  Flash_Write_Array(Page_addres_Gate + 2 + 4,(uint8_t *)&Send_TextDisplayGateWay,1);
	  }Send_ToGateWay = !CardID_Status;
	  key = keyPad();
	  if((uint32_t) (HAL_GetTick() -lastDelay) > 1000){
		
		  last_count =! last_count;
		  if(!Correct_Pass){
			  if(key == 0x42)countPass++;
			  if(key == 0x41) countCard++;
		  }
		  lastDelay = HAL_GetTick();  
	  }
	  static bool Retransmit;
	  if(NRF24_Available()){
		uint8_t Length_NRF = NRF24_GetDynamicPayloadSize();
		NRF24_Read(&buffer,Length_NRF);
		Gate.ID = 'A';
		HAL_Delay(5);
		NRF24_StopListening();
		 if (buffer[0] == 'A') {
			memcpy(&Gate, buffer, sizeof(Gate));
			NRF24_Write(&Gate,sizeof(Gate));
		} else if( strcmp((char*)buffer, "Reconnect") == 0) {
			NRF24_Write(&Gate, sizeof(Gate));
		}
		if(Retransmit == 0){
			NRF24_Write(&Gate,sizeof(Gate));
		}
		NRF24_StartListening();
	  }
  }
}
void Display_Total(void){
	if(Gate.status == 1){
		if( Gate.count > 0 && Gate.count < 20){
			LCD20X4_Gotoxy(0,1);
			LCD20X4_PutString("    OPENING GATE    ");
		}
	}else if(Gate.status == 2 || Gate.status == 4){
		if(Gate.count == 0  && Gate.status == 4){
			LCD20X4_Gotoxy(0,1);
			LCD20X4_PutString("       CLOSED       ");
		}else if(Gate.count == 20 && Gate.status == 2){
			LCD20X4_Gotoxy(0,1);
			LCD20X4_PutString("       OPENED       ");
		}else{
			LCD20X4_Gotoxy(0,1);
			LCD20X4_PutString("     PAUSE GATE     ");
		}
	}else if(Gate.status == 3 ){
		if( Gate.count > 0 && Gate.count < 20){
			LCD20X4_Gotoxy(0,1);
			LCD20X4_PutString("    CLOSING GATE    ");
		}
	}
	if(CardID_Status == Master_Card && Gate.status == 1){
		LCD20X4_Gotoxy(0,2);
		LCD20X4_PutString("OPEN BY CART MASTER ");
	}else if(CardID_Status == Master_Card && Gate.status == 3){
		LCD20X4_Gotoxy(0,2);
		LCD20X4_PutString("CLOSE BY CART MASTER");
	}else if(CardID_Status == Master_Card && (Gate.status == 2 || Gate.status == 4)){
		LCD20X4_Gotoxy(0,2);
		LCD20X4_PutString("PAUSE BY CART MASTER");
	}
	if(CardID_Status == Slave_Card && Gate.status == 1){
		LCD20X4_Gotoxy(0,2);
		LCD20X4_PutString(" OPEN BY CART SLAVE ");
		LCD20X4_Gotoxy(1,3);
		LCD20X4_PutString("NAME: ");
		LCD20X4_Gotoxy(7,3);
		LCD20X4_PutString((char *)CheckName);
	}else if(CardID_Status == Slave_Card && Gate.status == 3){
		LCD20X4_Gotoxy(0,2);
		LCD20X4_PutString(" CLOSE BY CART SLAVE");
		LCD20X4_Gotoxy(1,3);
		LCD20X4_PutString("NAME: ");
		LCD20X4_Gotoxy(7,3);
		LCD20X4_PutString((char *)CheckName);
	}else  if(CardID_Status == Slave_Card && (Gate.status == 2 || Gate.status == 4)){
		LCD20X4_Gotoxy(1,2);
		LCD20X4_PutString("PAUSE BY CART SLAVE");
		LCD20X4_Gotoxy(1,3);
		LCD20X4_PutString("NAME: ");
		LCD20X4_Gotoxy(7,3);
		LCD20X4_PutString((char *)CheckName);
	}else if(CardID_Status == Do_Not_Card){
		if(!Old_Add_Remove || !Old_PassID ){
			LCD20X4_Gotoxy(2,0);
			LCD20X4_PutString("WELCOME TO HOME");
			LCD20X4_Gotoxy(0,2);
			LCD20X4_PutString("    CARD OR PASS    ");
			LCD20X4_Gotoxy(0,3);
			LCD20X4_PutString("                 ");
		}
	}
}
void Display_PassHome(void){
	if(Display_CheckPass && !Correct_Pass){
		LCD20X4_Gotoxy(2,0);
		LCD20X4_PutString("OPEN BY PASSWORD");
		LCD20X4_Gotoxy(0,1);
		LCD20X4_PutString("ENTER PASS:");
		LCD20X4_Gotoxy(4,2);
		LCD20X4_PutString("PESS A TO DELETE");
		LCD20X4_Gotoxy(4,3);
		LCD20X4_PutString("PESS D TO BACK");
	}
	if(Enter_Pass[0] != 0x00){
		LCD20X4_Gotoxy(12,1);
		LCD20X4_PutString("*");
	}else{
		LCD20X4_Gotoxy(12,1);
		LCD20X4_PutString(" ");
	}
	if(Enter_Pass[1] != 0x00){
		LCD20X4_Gotoxy(13,1);
		LCD20X4_PutString("*");
	}else{
		LCD20X4_Gotoxy(13,1);
		LCD20X4_PutString(" ");
	}
	if(Enter_Pass[2] != 0x00){
		LCD20X4_Gotoxy(14,1);
		LCD20X4_PutString("*");
	}else{
		LCD20X4_Gotoxy(14,1);
		LCD20X4_PutString(" ");
	}
	if(Enter_Pass[3] != 0x00){
		LCD20X4_Gotoxy(15,1);
		LCD20X4_PutString("*");
	}else{
		LCD20X4_Gotoxy(15,1);
		LCD20X4_PutString(" ");
	}
	if(Enter_Pass[4] != 0x00){
		LCD20X4_Gotoxy(16,1);
		LCD20X4_PutString("*");
	}else{
		LCD20X4_Gotoxy(16,1);
		LCD20X4_PutString(" ");
	}
	if(Enter_Pass[5] != 0x00){
		LCD20X4_Gotoxy(17,1);
		LCD20X4_PutString("*");
	}else{
		LCD20X4_Gotoxy(17,1);
		LCD20X4_PutString(" ");
	}
}
void Display_CheckPass_Corret_Error(void){
	if(Correct_Pass){
		LCD20X4_Gotoxy(4,0);
		LCD20X4_PutString("CORRECT PASS");
		LCD20X4_Gotoxy(2,1);
		LCD20X4_PutString("PESS A TO OPEN");
	    LCD20X4_Gotoxy(2,2);
		LCD20X4_PutString("PESS B TO CLOSE");
	    LCD20X4_Gotoxy(2,3);
	    LCD20X4_PutString("PESS D TO BACK");
	}
	if(ERROR_Pass){
		LCD20X4_Gotoxy(3,1);
		LCD20X4_PutString("WRONG PASSWORD");
		LCD20X4_Gotoxy(3,2);
		LCD20X4_PutString("ENTER THE PASS");
		HAL_Delay(1000);
	}
	if(Display_CheckPassHome){
		LCD20X4_Clear();
		LCD20X4_Gotoxy(5,1);
		LCD20X4_PutString("NO PASSWORD");
		LCD20X4_Gotoxy(1,2);
		LCD20X4_PutString("PLEASE CREATE PASS");
		HAL_Delay(2000);
		Display_CheckPass =! Display_CheckPass;
	}
}
void Display_Check_Add_Remove(void){
		/* hien thi trang thai them the hoac xoa the */
	if(Old_Add_Remove){
		LCD20X4_Gotoxy(2,0);
		LCD20X4_PutString("  ADD OR REMOVE  ");
		LCD20X4_Gotoxy(0,1);
		LCD20X4_PutString("PESS A TO ADD");
		LCD20X4_Gotoxy(0,2);
		LCD20X4_PutString("PESS B TO REMOVE");
		LCD20X4_Gotoxy(0,3);
		LCD20X4_PutString("PESS D TO BACK   ");
	}
}
void Display_Check_Creat_Change_Pass(void){
	if(Old_PassID){
		LCD20X4_Gotoxy(2,0);
		LCD20X4_PutString("CREATE OR CHANGE");
		LCD20X4_Gotoxy(0,1);
		LCD20X4_PutString("PESS C TO VIEW ");
		LCD20X4_Gotoxy(0,3);
		LCD20X4_PutString("PESS D TO BACK   ");
	}
}
void Display_Creat_Change_Pass_OK_ERR(void){
	if(Master_Status == CreatPass){
		if(Display_Pass_OK == true){
			LCD20X4_Gotoxy(0,1);
			LCD20X4_PutString("CREATE SUCCESSFULLY");
			HAL_Delay(2000);
		}
		if(Display_Pass_ERR){
			LCD20X4_Gotoxy(0,1);
			LCD20X4_PutString("CREATE UNSUCCESFULLY");
			LCD20X4_Gotoxy(4,2);
			LCD20X4_PutString("ENTER THE PASS");
			HAL_Delay(2000);
			Display_Pass_ERR =! Display_Pass_ERR;
		}
	}else if(Master_Status == ChangePass){
		if(Display_Pass_ERR){
			LCD20X4_Gotoxy(0,1);
			LCD20X4_PutString("CHANGE UNSUCCESFULLY");
			LCD20X4_Gotoxy(4,2);
			LCD20X4_PutString("ENTER THE PASS");
			HAL_Delay(2000);
			Display_Pass_ERR =! Display_Pass_ERR;
		}
		if(Display_Loop){
			LCD20X4_Gotoxy(1,1);
			LCD20X4_PutString("DUPLICATE PASSWORD");
			LCD20X4_Gotoxy(4,2);
			LCD20X4_PutString("ENTER THE PASS");
			HAL_Delay(2000);
			LCD20X4_Clear();
			Display_Loop = !Display_Loop;
		}
		if(Display_Pass_OK){
			LCD20X4_Gotoxy(0,1);
			LCD20X4_PutString("CHANGE SUCCESSFULLY");
			HAL_Delay(2000);
		}
	}
}
void Display_Add_Remove(void){
	if(Master_Status == AddSlaveCard){
		if(Display_Add){
			LCD20X4_Clear();
			LCD20X4_Gotoxy(0,1);
			LCD20X4_PutString("  ADD SUCCESSFULLY");
			HAL_Delay(1000);
			Display_Add =! Display_Add;
		}else{
			LCD20X4_Gotoxy(3,0);
			LCD20X4_PutString("ADD CARD SLAVE ");
			LCD20X4_Gotoxy(0,1);
			LCD20X4_PutString("SCANF CARD TO SLAVE ");
			LCD20X4_Gotoxy(0,2);
			LCD20X4_PutString("CARD SLAVE: ");
			LCD20X4_Gotoxy(12,2);
			LCD20X4_SendInteger(ucountCheck);
			LCD20X4_Gotoxy(0,3);
			LCD20X4_PutString("   PESS D TO BACK");
		}
	}else if(Master_Status == RemoveSlaveCard){
		if(Display_Remove){
			LCD20X4_Clear();
			LCD20X4_Gotoxy(0,1);
			LCD20X4_PutString("REMOVE SUCCESSFULLY");
			HAL_Delay(1000);
			Display_Remove = !Display_Remove;
		}else{
			LCD20X4_Gotoxy(1,0);
			LCD20X4_PutString("REMOVE CARD SLAVE ");
			LCD20X4_Gotoxy(0,1);
			LCD20X4_PutString("SCANF CARD TO SLAVE");
			LCD20X4_Gotoxy(0,2);
			LCD20X4_PutString("CARD SLAVE: ");
			LCD20X4_Gotoxy(12,2);
			LCD20X4_SendInteger(ucountCheck);
			LCD20X4_Gotoxy(0,3);
			LCD20X4_PutString("   PESS D TO BACK");
		}
	}
	
}
void Display_Creat_Change_Pass(void){
	/* hien thi tao hoac them mat khau */
	if(Old_PassID){
		if(Master_Status == CreatPass){
			LCD20X4_Gotoxy(2,0);
			LCD20X4_PutString("  CREAT PASS     ");
			LCD20X4_Gotoxy(6,2);
			LCD20X4_PutString("PESS A DELETE");
			LCD20X4_Gotoxy(6,3);
			LCD20X4_PutString("PESS D TO BACK");
			if(EnterThePass){
				LCD20X4_Gotoxy(0,1);
				LCD20X4_PutString("CONFIRM PASS:");
				LCD20X4_Gotoxy(13,1);
				LCD20X4_PutString((char*)XacNhanLai);
			}else{
				LCD20X4_Gotoxy(0,1);
				LCD20X4_PutString("ENTER PASS: ");
				LCD20X4_Gotoxy(12,1);
				LCD20X4_PutString(Pass_Home);
			}
		}else if(Master_Status == ChangePass){
			LCD20X4_Gotoxy(2,0);
			LCD20X4_PutString("  CHANGE PASS     ");			
			LCD20X4_Gotoxy(6,2);
			LCD20X4_PutString("PESS A DELETE");
			LCD20X4_Gotoxy(6,3);
			LCD20X4_PutString("PESS D TO BACK");
			if(EnterThePass){
				LCD20X4_Gotoxy(0,1);
				LCD20X4_PutString("CONFIRM PASS:");
				LCD20X4_Gotoxy(13,1);
				LCD20X4_PutString(XacNhanLai);
			}else{
				LCD20X4_Gotoxy(0,1);
				LCD20X4_PutString("ENTER PASS: ");
				LCD20X4_Gotoxy(12,1);
				LCD20X4_PutString(Pass_Change);
			}
		}
	}
}
void Display_Slave_Name(void){
	/* hien thi trang thai them ten nguoi giu the */
	if(countName == 5){
		LCD20X4_Gotoxy(3,0);
		LCD20X4_PutString("ADD CARD SLAVE ");
		LCD20X4_Gotoxy(0,1);
		LCD20X4_PutString("ENTER NAME: ");
		LCD20X4_Gotoxy(0,3);
		LCD20X4_PutString("    PESS A TO DELETE");
		LCD20X4_Gotoxy(12,1);
		LCD20X4_PutString((char *)Display_Name_slave);
	}
}
void Door_StepMotor(void){
	static bool Old_RFID_static;
	if(CardID_Status && !Old_RFID_static){
		if(CardID_Status == Master_Card || CardID_Status == Slave_Card){
				Gate.status++;
				if(Gate.status > 4) Gate.status = 1;
		}
	}Old_RFID_static = CardID_Status;
	/* status = 1, dong co quay thuan -> mo cua (nguoc lai voi 4); status = 2 or 3 -> dung dong co */
	if(Gate.status == 1){
		HAL_TIM_PWM_Start(&htim4,TIM_CHANNEL_1);
		HAL_GPIO_WritePin(Motor_Port, ENABLE_Motor, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(Motor_Port,DIR_Motor,GPIO_PIN_RESET);
	}else if(Gate.status == 2){
		HAL_GPIO_WritePin(Motor_Port, ENABLE_Motor, GPIO_PIN_SET);
		HAL_TIM_PWM_Stop(&htim4,TIM_CHANNEL_1);
	}else if(Gate.status == 3){
		HAL_TIM_PWM_Start(&htim4,TIM_CHANNEL_1);
		HAL_GPIO_WritePin(Motor_Port, ENABLE_Motor, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(Motor_Port,DIR_Motor,GPIO_PIN_SET);
	}else if(Gate.status == 4){
		HAL_GPIO_WritePin(Motor_Port, ENABLE_Motor, GPIO_PIN_SET);
		HAL_TIM_PWM_Stop(&htim4,TIM_CHANNEL_1);
	}
	if(Gate.status == 1){
		if(Send_TextDisplayGateWay > 21) Send_TextDisplayGateWay = 1;
		if(Gate.count < Send_TextDisplayGateWay) Send_TextDisplayGateWay--;
		if(Gate.count > Send_TextDisplayGateWay) Send_TextDisplayGateWay++;
		if(Gate.count > 20 && Gate.status == 1){
			Gate.count = 20; 
			Gate.status = 2;
		}
		if(last_count){
			Gate.count++;
			Send_TextDisplayGateWay++;
		}
	}else if(Gate.status == 3){
		if(Send_TextDisplayGateWay > 21) Send_TextDisplayGateWay = 21;
		if(Gate.count < Send_TextDisplayGateWay) Send_TextDisplayGateWay --;
		if(Gate.count > Send_TextDisplayGateWay) Send_TextDisplayGateWay ++;
			if(last_count){
				Gate.count --;
				Send_TextDisplayGateWay--;
		}
		if(Gate.count == 0 && Gate.status == 3){
			Gate.count = 0;
			Gate.status = 4;
		}
	}
	if(Gate.count == Send_TextDisplayGateWay){
		if(Gate.status == 1 && Send_TextDisplayGateWay == 20 ){
			Send_TextDisplayGateWay = 21;
			Gate.count = 20; 
			Gate.status = 2;
		}else if(Gate.status == 4 && Send_TextDisplayGateWay == 0){
			Send_TextDisplayGateWay = 1;
			Gate.count = 0;
			Gate.status = 4;
		}else if((Gate.status == 2 || Gate.status == 4) && (Send_TextDisplayGateWay != 1 || Send_TextDisplayGateWay != 21)){
			Send_TextDisplayGateWay++;
			Gate.ID = 'A';
			NRF24_StopListening();
			NRF24_Write(&Gate,sizeof(Gate));
			NRF24_StartListening();
			Flash_Erase(Page_addres_Gate);
			Flash_Write_Array(Page_addres_Gate + 2,(uint8_t *)&Gate, 4);
			Flash_Write_Array(Page_addres_Gate + 2 + 4,(uint8_t *)&Send_TextDisplayGateWay,1);
		}
		Gate.ID = 'A';
		NRF24_StopListening();
		NRF24_Write(&Gate,sizeof(Gate));
		NRF24_StartListening();
		Flash_Erase(Page_addres_Gate);
		Flash_Write_Array(Page_addres_Gate + 2,(uint8_t *)&Gate, 4);
		Flash_Write_Array(Page_addres_Gate + 2 + 4,(uint8_t *)&Send_TextDisplayGateWay,1);
		
	}
}
char keyPad(void){
	  char keys[4][4] ={{'1', '2', '3', 'A'},
					  {'4', '5', '6', 'B'},
					  {'7', '8', '9', 'C'},
					  {'*', '0', '#', 'D'}};	
	for(int i = 0; i < 4; i++){
		// Set current column as output and low
		switch(i){
			case 0:
				HAL_GPIO_WritePin(COL_PORT, COL_1_Pin, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(COL_PORT, COL_2_Pin, GPIO_PIN_SET);
				HAL_GPIO_WritePin(COL_PORT, COL_3_Pin, GPIO_PIN_SET);
				HAL_GPIO_WritePin(COL_PORT, COL_4_Pin, GPIO_PIN_SET);
				break;
			case 1:
				HAL_GPIO_WritePin(COL_PORT, COL_1_Pin, GPIO_PIN_SET);
				HAL_GPIO_WritePin(COL_PORT, COL_2_Pin, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(COL_PORT, COL_3_Pin, GPIO_PIN_SET);
				HAL_GPIO_WritePin(COL_PORT, COL_4_Pin, GPIO_PIN_SET);
				break;
			case 2:
				HAL_GPIO_WritePin(COL_PORT, COL_1_Pin, GPIO_PIN_SET);
				HAL_GPIO_WritePin(COL_PORT, COL_2_Pin, GPIO_PIN_SET);
				HAL_GPIO_WritePin(COL_PORT, COL_3_Pin, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(COL_PORT, COL_4_Pin, GPIO_PIN_SET);
				break;
			case 3:
				HAL_GPIO_WritePin(COL_PORT, COL_1_Pin, GPIO_PIN_SET);
				HAL_GPIO_WritePin(COL_PORT, COL_2_Pin, GPIO_PIN_SET);
				HAL_GPIO_WritePin(COL_PORT, COL_3_Pin, GPIO_PIN_SET);
				HAL_GPIO_WritePin(COL_PORT, COL_4_Pin, GPIO_PIN_RESET);
				break;
		}
		// Read current rows
		if(HAL_GPIO_ReadPin(ROW_PORT, ROW_1_Pin) == GPIO_PIN_RESET)
		  return keys[0][i];
		if(HAL_GPIO_ReadPin(ROW_PORT, ROW_2_Pin) == GPIO_PIN_RESET)
		  return keys[1][i];
		if(HAL_GPIO_ReadPin(ROW_PORT_B, ROW_3_Pin) == GPIO_PIN_RESET)
		  return keys[2][i];
		if(HAL_GPIO_ReadPin(ROW_PORT_B, ROW_4_Pin) == GPIO_PIN_RESET)
		  return keys[3][i];
	}
	return 0;
}
void Check_CardID(void){
	key = keyPad();
	if(MFRC522_Check(CardID) == MI_OK){
		if(CardID[0]== Master_CardID[0] && CardID[1]== Master_CardID[1] && CardID[2]== Master_CardID[2] && CardID[3]== Master_CardID[3] && CardID[4]== Master_CardID[4] ){
			CardID_Status = Master_Card;
		}else{
			for(uint8_t ucount = 0; ucount < 5 ; ucount++){
				if(CardID[0] == Slaver_CardID[ucount][0] && CardID[1] == Slaver_CardID[ucount][1] && CardID[2] == Slaver_CardID[ucount][2] && CardID[3] == Slaver_CardID[ucount][3] && CardID[4] == Slaver_CardID[ucount][4]){
					CardID_Status = Slave_Card;
					memcpy(&CheckName, &Name_Slaver[ucount],sizeof(Name_Slaver[ucount]));
				}
				if(CardID_Status == Slave_Card) break;
			}
		}
	}else{
		CardID_Status = Do_Not_Card;
	}
	if(CardID_Status == Master_Card && key == 0x41){
				Master_Status = AddSlaveCard;
	}else if(CardID_Status == Master_Card && key == 0x42){
		Master_Status = RemoveSlaveCard;
	}
	if(CardID_Status == Master_Card && key == 0x43){
		if(Pass_Home[0] == 0xFF && Pass_Home[1] == 0xFF && Pass_Home[2] == 0xFF && Pass_Home[3] == 0xFF && Pass_Home[4] == 0xFF && Pass_Home[5] == 0xFF){
			Master_Status = CreatPass;
		}else if(Pass_Home[0] != 0xFF && Pass_Home[1] != 0xFF && Pass_Home[2] != 0xFF && Pass_Home[3] != 0xFF && Pass_Home[4] != 0xFF && Pass_Home[5] != 0xFF ){
			Master_Status = ChangePass;
		}
	}
}
void Check_PassID(void){
	bool Old_EnterPass;
	static uint8_t ucount =0;
	if(key == 0x31 || key == 0x32 || key == 0x33 || key == 0x34 || key == 0x35 || key == 0x36 || key == 0x37 || key == 0x38 || key == 0x39 || key == 0x30){
		LCD20X4_Clear();
		Display_CheckPass = !Display_CheckPass;
		while(1){
			key = keyPad();
			if(Pass_Home[0] == 0xFF && Pass_Home[1] == 0xFF && Pass_Home[2] == 0xFF && Pass_Home[3] == 0xFF && Pass_Home[4] == 0xFF && Pass_Home[5] == 0xFF){
				Display_CheckPassHome = !Display_CheckPassHome;
			}
			if(Pass_Home[0] != 0xFF && Pass_Home[1] !=  0xFF && Pass_Home[2] !=  0xFF && Pass_Home[3] !=  0xFF && Pass_Home[4] !=  0xFF && Pass_Home[5] !=  0xFF){
				key = keyPad();
				if(key && !Old_EnterPass){
					if(key == 0x31 || key == 0x32 || key == 0x33 || key == 0x34 || key == 0x35 || key == 0x36 || key == 0x37 || key == 0x38 || key == 0x39 || key == 0x30){
						ucount++;
					}
					if(ucount == 1 && Enter_Pass[0] == 0x00) Enter_Pass[0] = key;
					if(ucount == 2 && Enter_Pass[1] == 0x00) Enter_Pass[1] = key;
					if(ucount == 3 && Enter_Pass[2] == 0x00) Enter_Pass[2] = key;
					if(ucount == 4 && Enter_Pass[3] == 0x00) Enter_Pass[3] = key;
					if(ucount == 5 && Enter_Pass[4] == 0x00) Enter_Pass[4] = key;
					if(ucount == 6 && Enter_Pass[5] == 0x00){
						Enter_Pass[5] = key;
						ucount = 0;
					}
					/* xoa tung o mot */
					if(key == 0x41){
						if(ucount == 1 && Enter_Pass[0] != 0x00) Enter_Pass[0] = 0x00;
						if(ucount == 2 && Enter_Pass[1] != 0x00) Enter_Pass[1] = 0x00;
						if(ucount == 3 && Enter_Pass[2] != 0x00) Enter_Pass[2] = 0x00;
						if(ucount == 4 && Enter_Pass[3] != 0x00) Enter_Pass[3] = 0x00;
						if(ucount == 5 && Enter_Pass[4] != 0x00) Enter_Pass[4] = 0x00;
						if(ucount == 6 && Enter_Pass[5] != 0x00) Enter_Pass[5] = 0x00;
						ucount--;
					}
					if(ucount > 6) ucount = 0;
				}Old_EnterPass = key;
			}
			if(Enter_Pass[0] != 0x00 &&	Enter_Pass[1] != 0x00 && Enter_Pass[2] != 0x00 && Enter_Pass[3] != 0x00 && Enter_Pass[4] != 0x00 && Enter_Pass[5] != 0x00 ){
				if(Enter_Pass[0] == Pass_Home[0] && Enter_Pass[1] == Pass_Home[1] && Enter_Pass[2] == Pass_Home[2] && Enter_Pass[3] == Pass_Home[3] && Enter_Pass[4] == Pass_Home[4] && Enter_Pass[5] == Pass_Home[5]){
					memset(Enter_Pass, 0x00, sizeof(Enter_Pass));
					Correct_Pass =! Correct_Pass;
				}else if(Enter_Pass[0] != Pass_Home[0] || Enter_Pass[1] != Pass_Home[1] || Enter_Pass[2] != Pass_Home[2] || Enter_Pass[3] != Pass_Home[3] || Enter_Pass[4] != Pass_Home[4] || Enter_Pass[5] != Pass_Home[5]){
					memset(Enter_Pass, 0x00, sizeof(Enter_Pass));
					ERROR_Pass =! ERROR_Pass;
				}
			}
			if(ERROR_Pass){
				LCD20X4_Clear();
				Display_CheckPass_Corret_Error();
				LCD20X4_Clear();
				if(ERROR_Pass) ERROR_Pass =! ERROR_Pass;
			}
			if(key == 0x44 ||Display_CheckPassHome || Correct_Pass){
				LCD20X4_Clear();
				Display_CheckPass_Corret_Error();
				if(Display_CheckPassHome ) Display_CheckPassHome =! Display_CheckPassHome;
				Display_CheckPass = ! Display_CheckPass;
				LCD20X4_Clear();
				break;
			}
			Display_PassHome();
		}
	}
}
void Check_Add_Remove(void){
	if(countCard == 3){
		Old_Add_Remove =! Old_Add_Remove;
		countCard = 0;
	}
	if(Old_Add_Remove){
		LCD20X4_Clear();
		while(1){
			Check_CardID();
			Display_Check_Add_Remove();
			if(Master_Status == AddSlaveCard || Master_Status == RemoveSlaveCard){
				LCD20X4_Clear();
				Add_RemoveCard();
			}else{
				if(key == 0x44){
					LCD20X4_Clear();
					Old_Add_Remove = !Old_Add_Remove;
					break;
				}
			}	
		}
	}
}
void Check_Creat_Change_PassID(void){
	key = keyPad();
	if(countPass == 3){
		Old_PassID = !Old_PassID;
		countPass = 0;
	}
	if(Old_PassID){
		LCD20X4_Clear();
		while(1){
			Check_CardID();
			Display_Check_Creat_Change_Pass();
			if( Master_Status == CreatPass || Master_Status == ChangePass){
				LCD20X4_Clear();
				Creat_Change_PassID();
				if(Master_Status == Do_Nothing){
					Old_PassID =! Old_PassID;
					break;
				}
			}else{
				if(key == 0x44){
					LCD20X4_Clear();
					Old_PassID = !Old_PassID;
					break;
				}
			}
		}
	}
}
void Add_RemoveCard(void){
	static bool Old_RFID_static;
	uint8_t countSlaveAdd = 0;
	uint8_t countAddFlash = 0;
	uint8_t countREMOFlash = 0;
	while(1){
		key = keyPad();
		Display_Add_Remove();
		if(MFRC522_Check(CardID) == MI_OK && !Old_RFID_static){
			/* Add slave */
			if(Master_Status == AddSlaveCard){
				/* check slave co trung card khong */
				for(uint8_t ucount = 0;  ucount< 5; ucount++){
					if((Slaver_CardID[ucount][0] == CardID[0]) && (Slaver_CardID[ucount][1] == CardID[1]) && (Slaver_CardID[ucount][2] == CardID[2]) && (Slaver_CardID[ucount][3] == CardID[3]) && (Slaver_CardID[ucount][4] == CardID[4])){
						countSlaveAdd = 0; 
						break;	
					}else{
						countSlaveAdd++;
					}
				}
				/* check card xem co trung master card khong */
				if(countSlaveAdd > 0 && (CardID[0] != Master_CardID[0] && CardID[1] != Master_CardID[1] && CardID[2] != Master_CardID[2] && CardID[3] != Master_CardID[3] && CardID[4] !=
					Master_CardID[4])){
					for(uint8_t ucount = 0; ucount < 5; ucount++){
						for(uint8_t ucount = 0;  ucount< 5; ucount++){
							if((Slaver_CardID[ucount][0] == CardID[0]) && (Slaver_CardID[ucount][1] == CardID[1]) && (Slaver_CardID[ucount][2] == CardID[2]) && (Slaver_CardID[ucount][3] == CardID[3]) && (Slaver_CardID[ucount][4] == CardID[4])){
								countSlaveAdd = 0;
								break;
							}
						}
						if(countSlaveAdd == 0) break;	
						if((Slaver_CardID[ucount][0] == 0xFF) && (Slaver_CardID[ucount][1] == 0xFF) && (Slaver_CardID[ucount][2] == 0xFF) && (Slaver_CardID[ucount][3] == 0xFF) && (Slaver_CardID[ucount][4] == 0xFF)){
							for(uint8_t ucountAdd = 0; ucountAdd <5; ucountAdd++){
								Slaver_CardID[ucount][ucountAdd] = CardID[ucountAdd];
								countName++;
								countSlaveAdd = 0;
							}
							ucountCheck++;
							break;
						}
					}
					if(countName == 5){
						Slave_Name_Add();
						memset(Display_Name_slave, 0x00,sizeof(Display_Name_slave));
						countAddFlash++;
						countName = 0;
					}
					if(countAddFlash == 1){
						if(PageActive == Page2){
								Flash_Erase(Page_addres_2);
								Flash_Erase(Page_addres_Name);
								Flash_Erase(Page_addres_3);
								Flash_Write_Array(Page_addres_3 + 2, &PageActive, 1);
								Flash_Write_Array(Page_addres_2 + 2, (uint8_t *)Slaver_CardID, 100);
								Flash_Write_Array(Page_addres_Name + 2, (uint8_t*)Name_Slaver,30);
							}else if(PageActive == Page1){
								Flash_Erase(Page_addres_1);
								Flash_Erase(Page_addres_Name);
								Flash_Erase(Page_addres_3);
								Flash_Write_Array(Page_addres_3 + 2, &PageActive, 1);
								Flash_Write_Array(Page_addres_1 + 2, (uint8_t *)Slaver_CardID, 100);
								Flash_Write_Array(Page_addres_Name + 2, (uint8_t*)Name_Slaver,30);
						}
						Flash_Erase(Page_addres_QuantitySlave);
						Flash_Write_Array(Page_addres_QuantitySlave + 2, &ucountCheck, 1);
						countAddFlash = 0;
						Display_Add =! Display_Add;
					}	
				}
			}
			
			/* Remove slave */
			if(Master_Status == RemoveSlaveCard){
				for(uint8_t ucount = 0;  ucount< 20; ucount++){
					if((Slaver_CardID[ucount][0] == CardID[0]) && (Slaver_CardID[ucount][1] == CardID[1]) && (Slaver_CardID[ucount][2] == CardID[2]) && (Slaver_CardID[ucount][3] == CardID[3]) && (Slaver_CardID[ucount][4] == CardID[4])){
						memset(Slaver_CardID[ucount], 0xFF, sizeof(Slaver_CardID[ucount]));
						memset(Name_Slaver[ucount], 0xFF, sizeof(Name_Slaver[ucount]));
						countREMOFlash++;
						ucountCheck--;
					}
				}
				if(countREMOFlash == 1){
					if(PageActive == Page2){
						PageActive = Page1;
						Flash_Erase(Page_addres_1);
						Flash_Write_Array(Page_addres_1+ 2, (uint8_t *)Slaver_CardID, 100);
						Flash_Erase(Page_addres_3);
						Flash_Write_Array(Page_addres_3 + 2, &PageActive, 1);
						Flash_Erase(Page_addres_2);
						Flash_Erase(Page_addres_Name);
						Flash_Write_Array(Page_addres_Name + 2, (uint8_t *)Name_Slaver, 30);
						
					}else{
						PageActive = Page2;
						Flash_Erase(Page_addres_2);
						Flash_Write_Array(Page_addres_2 + 2, (uint8_t *)Slaver_CardID, 100);
						Flash_Erase(Page_addres_3);
						Flash_Write_Array(Page_addres_3 + 2, &PageActive, 1);
						Flash_Erase(Page_addres_1);
						Flash_Erase(Page_addres_Name);
						Flash_Write_Array(Page_addres_Name + 2, (uint8_t *)Name_Slaver, 30);
					}
					/* check so luong card slave */
					Flash_Erase(Page_addres_QuantitySlave);
					Flash_Write_Array(Page_addres_QuantitySlave + 2, &ucountCheck, 1);
					countREMOFlash = 0;
					Display_Remove = !Display_Remove;
				}
			}
		}else{
			if(key == 0x44){
					LCD20X4_Clear();
					Master_Status = Do_Nothing;
					CardID_Status = Do_Not_Card;
					break;
			}
		}
		Old_RFID_static = MFRC522_Check(CardID) ;
	}
	
}
void Creat_Change_PassID(void){
	static bool CheckPassNew;
	static bool Old_Enter_key, Old_XacNhan_key,Old_XacNhan_Pass;
	static uint8_t ucount;
	while(1){
		key = keyPad();
		if(Master_Status == CreatPass){
			if(key && !Old_Enter_key){
				if(key == 0x31 || key == 0x32 || key == 0x33 || key == 0x34 || key == 0x35 || key == 0x36 || key == 0x37 || key == 0x38 || key == 0x39 || key == 0x30){
					ucount++;
				}
				if(ucount == 1 && Pass_Home[0] == 0xFF) Pass_Home[0] = key;
				if(ucount == 2 && Pass_Home[1] == 0xFF) Pass_Home[1] = key;
				if(ucount == 3 && Pass_Home[2] == 0xFF) Pass_Home[2] = key;
				if(ucount == 4 && Pass_Home[3] == 0xFF) Pass_Home[3] = key;
				if(ucount == 5 && Pass_Home[4] == 0xFF) Pass_Home[4] = key;
				if(ucount == 6 && Pass_Home[5] == 0xFF){
					Pass_Home[5] = key;
					ucount = 0;
				}
				/* xoa tung o mot */
				if(key == 0x41){
					if(ucount == 1 && Pass_Home[0] != 0xFF) Pass_Home[0] = 0xFF;
					if(ucount == 2 && Pass_Home[1] != 0xFF) Pass_Home[1] = 0xFF;
					if(ucount == 3 && Pass_Home[2] != 0xFF) Pass_Home[2] = 0xFF;
					if(ucount == 4 && Pass_Home[3] != 0xFF) Pass_Home[3] = 0xFF;
					if(ucount == 5 && Pass_Home[4] != 0xFF) Pass_Home[4] = 0xFF;
					if(ucount == 6 && Pass_Home[5] != 0xFF) Pass_Home[5] = 0xFF;
					LCD20X4_Gotoxy(12,1);
					LCD20X4_PutString((char *)Pass_Home);
					ucount--;
				}
				if(ucount > 6) ucount = 0;
			}Old_Enter_key = key;
			if(Pass_Home[0] != 0xFF && Pass_Home[1] != 0xFF && Pass_Home[2] != 0xFF && Pass_Home[3] != 0xFF && Pass_Home[4] != 0xFF && Pass_Home[5] != 0xFF){
				EnterThePass =! EnterThePass;
			}
			if(EnterThePass){
				if(key != Pass_Home[5]){
					while(1){
						Display_Creat_Change_Pass();
						key = keyPad();
						if(key && !Old_XacNhan_key){
							if(key == 0x31 || key == 0x32 || key == 0x33 || key == 0x34 || key == 0x35 || key == 0x36 || key == 0x37 || key == 0x38 || key == 0x39 || key == 0x30){
								ucount++;
							}
							if(ucount == 1 && XacNhanLai[0] == 0xFF) XacNhanLai[0] = key;
							if(ucount == 2 && XacNhanLai[1] == 0xFF) XacNhanLai[1] = key;
							if(ucount == 3 && XacNhanLai[2] == 0xFF) XacNhanLai[2] = key;
							if(ucount == 4 && XacNhanLai[3] == 0xFF) XacNhanLai[3] = key;
							if(ucount == 5 && XacNhanLai[4] == 0xFF) XacNhanLai[4] = key;
							if(ucount == 6 && XacNhanLai[5] == 0xFF){
								XacNhanLai[5] = key;
								ucount = 0;
							}
							/* xoa tung o mot */
							if(key == 0x41){
								if(ucount == 1 && XacNhanLai[0] != 0xFF) XacNhanLai[0] = 0xFF;
								if(ucount == 2 && XacNhanLai[1] != 0xFF) XacNhanLai[1] = 0xFF;
								if(ucount == 3 && XacNhanLai[2] != 0xFF) XacNhanLai[2] = 0xFF;
								if(ucount == 4 && XacNhanLai[3] != 0xFF) XacNhanLai[3] = 0xFF;
								if(ucount == 5 && XacNhanLai[4] != 0xFF) XacNhanLai[4] = 0xFF;
								if(ucount == 6 && XacNhanLai[5] != 0xFF) XacNhanLai[5] = 0xFF;
								LCD20X4_Gotoxy(13,1);
								LCD20X4_PutString((char *)XacNhanLai);
								ucount--;
							}
							if(ucount > 6) ucount = 0;
						}Old_XacNhan_key = key;
						key = keyPad();
						if(XacNhanLai[0] != 0xFF && XacNhanLai[1] != 0xFF && XacNhanLai[2] != 0xFF && XacNhanLai[3] != 0xFF && XacNhanLai[4] != 0xFF && XacNhanLai[5] != 0xFF){
							CheckPassNew = !CheckPassNew;
						}
						if(CheckPassNew){
							key = keyPad();
							if(XacNhanLai[0] == Pass_Home[0] && XacNhanLai[1] == Pass_Home[1] && XacNhanLai[2] == Pass_Home[2] && XacNhanLai[3] == Pass_Home[3] && XacNhanLai[4] == Pass_Home[4] && XacNhanLai[5] == Pass_Home[5]){
								Flash_Erase(Page_addres_Pass);
								Flash_Write_Array(Page_addres_Pass + 2, (uint8_t *)&Pass_Home, 6);
								memset(&XacNhanLai, 0xFF, sizeof(XacNhanLai));
								CheckPassNew = !CheckPassNew;
								EnterThePass =!EnterThePass;
								Display_Pass_OK = true;
								break;
							}else if(XacNhanLai[0] != Pass_Home[0] || XacNhanLai[1] != Pass_Home[1] || XacNhanLai[2] != Pass_Home[2] || XacNhanLai[3] != Pass_Home[3] || XacNhanLai[4] != Pass_Home[4] || XacNhanLai[5] != Pass_Home[5]){ 
							/* xoa gia tri cua pass va nhaplaipass */
								memset(&Pass_Home, 0xFF, sizeof(Pass_Change));
								memset(&XacNhanLai, 0xFF, sizeof(XacNhanLai));
								CheckPassNew = !CheckPassNew;
								EnterThePass =!EnterThePass;
								Display_Pass_ERR = !Display_Pass_ERR;
								break;
							}
						}
					}
				}
			}
		}
		/* thay doi pass */
		if(Master_Status == ChangePass){
			if(key && !Old_Enter_key){
				if(key == 0x31 || key == 0x32 || key == 0x33 || key == 0x34 || key == 0x35 || key == 0x36 || key == 0x37 || key == 0x38 || key == 0x39 || key == 0x30){
					ucount++;
				}
				if(ucount == 1 && Pass_Change[0] == 0xFF) Pass_Change[0] = key;
				if(ucount == 2 && Pass_Change[1] == 0xFF) Pass_Change[1] = key;
				if(ucount == 3 && Pass_Change[2] == 0xFF) Pass_Change[2] = key;
				if(ucount == 4 && Pass_Change[3] == 0xFF) Pass_Change[3] = key;
				if(ucount == 5 && Pass_Change[4] == 0xFF) Pass_Change[4] = key;
				if(ucount == 6 && Pass_Change[5] == 0xFF){
					Pass_Change[5] = key;
					ucount = 0;
				}
				/* xoa tung o mot */
				if(key == 0x41){
					if(ucount == 1 && Pass_Change[0] != 0xFF) Pass_Change[0] = 0xFF;
					if(ucount == 2 && Pass_Change[1] != 0xFF) Pass_Change[1] = 0xFF;
					if(ucount == 3 && Pass_Change[2] != 0xFF) Pass_Change[2] = 0xFF;
					if(ucount == 4 && Pass_Change[3] != 0xFF) Pass_Change[3] = 0xFF;
					if(ucount == 5 && Pass_Change[4] != 0xFF) Pass_Change[4] = 0xFF;
					if(ucount == 6 && Pass_Change[5] != 0xFF) Pass_Change[5] = 0xFF;
					LCD20X4_Gotoxy(12,1);
					LCD20X4_PutString((char *)Pass_Change);
					ucount--;
				}
				if(ucount > 6) ucount = 0;
			}Old_Enter_key = key;
			key = keyPad();
			if(Pass_Change[0] != 0xFF && Pass_Change[1] != 0xFF && Pass_Change[2] != 0xFF && Pass_Change[3] != 0xFF && Pass_Change[4] != 0xFF && Pass_Change[5] != 0xFF){
				EnterThePass =! EnterThePass;
			}
			if(EnterThePass){
				if(key != Pass_Change[5]){
					LCD20X4_Clear();
					while(1){
						key = keyPad();
						Display_Creat_Change_Pass();
						if(key && !Old_XacNhan_key){
							if(key == 0x31 || key == 0x32 || key == 0x33 || key == 0x34 || key == 0x35 || key == 0x36 || key == 0x37 || key == 0x38 || key == 0x39 || key == 0x30){
								ucount++;
							}
							if(ucount == 1 && XacNhanLai[0] == 0xFF) XacNhanLai[0] = key;
							if(ucount == 2 && XacNhanLai[1] == 0xFF) XacNhanLai[1] = key;
							if(ucount == 3 && XacNhanLai[2] == 0xFF) XacNhanLai[2] = key;
							if(ucount == 4 && XacNhanLai[3] == 0xFF) XacNhanLai[3] = key;
							if(ucount == 5 && XacNhanLai[4] == 0xFF) XacNhanLai[4] = key;
							if(ucount == 6 && XacNhanLai[5] == 0xFF){
								XacNhanLai[5] = key;
								ucount = 0;
							}
							/* xoa tung o mot */
							if(key == 0x41){
								if(ucount == 1 && XacNhanLai[0] != 0xFF) XacNhanLai[0] = 0xFF;
								if(ucount == 2 && XacNhanLai[1] != 0xFF) XacNhanLai[1] = 0xFF;
								if(ucount == 3 && XacNhanLai[2] != 0xFF) XacNhanLai[2] = 0xFF;
								if(ucount == 4 && XacNhanLai[3] != 0xFF) XacNhanLai[3] = 0xFF;
								if(ucount == 5 && XacNhanLai[4] != 0xFF) XacNhanLai[4] = 0xFF;
								if(ucount == 6 && XacNhanLai[5] != 0xFF) XacNhanLai[5] = 0xFF;
								LCD20X4_Gotoxy(13,1);
								LCD20X4_PutString((char *)XacNhanLai);
								ucount--;
							}
							if(ucount > 6) ucount = 0;
						}Old_XacNhan_key = key;
						if(XacNhanLai[0] != 0xFF && XacNhanLai[1] != 0xFF && XacNhanLai[2] != 0xFF && XacNhanLai[3] != 0xFF && XacNhanLai[4] != 0xFF && XacNhanLai[5] != 0xFF){
							CheckPassNew = !CheckPassNew;
						}
						if(CheckPassNew){
							key = keyPad();
							if(XacNhanLai[0] == Pass_Change[0] && XacNhanLai[1] == Pass_Change[1] && XacNhanLai[2] == Pass_Change[2] && XacNhanLai[3] == Pass_Change[3] && XacNhanLai[4] == Pass_Change[4] && XacNhanLai[5] == Pass_Change[5]){
								Old_XacNhan_Pass =! Old_XacNhan_Pass;
							}
							if(XacNhanLai[0] != Pass_Change[0] || XacNhanLai[1] != Pass_Change[1] || XacNhanLai[2] != Pass_Change[2] || XacNhanLai[3] != Pass_Change[3] || XacNhanLai[4] != Pass_Change[4] || XacNhanLai[5] != Pass_Change[5]){
							/* xoa gia tri cua pass va nhaplaipass */
								memset(&Pass_Change, 0xFF, sizeof(Pass_Change));
								memset(&XacNhanLai, 0xFF, sizeof(XacNhanLai));
								CheckPassNew = !CheckPassNew;
								EnterThePass =!EnterThePass;
								Display_Pass_ERR = !Display_Pass_ERR;
								break;
							}
						}
						if(Old_XacNhan_Pass){
							if(XacNhanLai[0] != Pass_Home[0] || XacNhanLai[1] != Pass_Home[1] || XacNhanLai[2] != Pass_Home[2] || XacNhanLai[3] != Pass_Home[3] || XacNhanLai[4] != Pass_Home[4] || XacNhanLai[5] != Pass_Home[5]){
								memcpy(Pass_Home,XacNhanLai,sizeof(Pass_Home));
								Flash_Erase(Page_addres_Pass);
								Flash_Write_Array(Page_addres_Pass + 2, (uint8_t *)&Pass_Home, 6);
								memset(&Pass_Change, 0xFF, sizeof(Pass_Change));
								memset(&XacNhanLai, 0xFF, sizeof(XacNhanLai));
								CheckPassNew = !CheckPassNew;
								EnterThePass =!EnterThePass;
								Old_XacNhan_Pass = !Old_XacNhan_Pass;
								Display_Pass_OK = true;
								break;
							}else{
								/* trung voi pass thi nhap lai */
								memset(&Pass_Change, 0xFF, sizeof(Pass_Change));
								memset(&XacNhanLai, 0xFF, sizeof(XacNhanLai));
								CheckPassNew = !CheckPassNew;
								EnterThePass = !EnterThePass;
								Display_Loop = !Display_Loop;
								Old_XacNhan_Pass = !Old_XacNhan_Pass;
								break;
							}
						}
					}
				}
			}
		}
		if(Display_Pass_OK || Display_Pass_ERR || Display_Loop){
			LCD20X4_Clear();
			Display_Creat_Change_Pass_OK_ERR();
			if(Display_Pass_OK){
				Display_Pass_OK = false;
				Master_Status = Do_Nothing;
			}
			if(Display_Pass_ERR) Display_Pass_OK = false;
			if(Display_Loop) Display_Loop =! Display_Loop;
			LCD20X4_Clear();
		}
		Display_Creat_Change_Pass();
		if(key == 0x44 || Master_Status == Do_Nothing){
			LCD20X4_Clear();
			Master_Status = Do_Nothing;
			break;
		}
	}
}
void Read_FLashFrom(void){
	Flash_Read_Array(Page_addres_3 + 2, &checkPage, 1);
	Flash_Read_Array(Page_addres_Pass + 2, (uint8_t*)&Pass_Home, 6);
	Flash_Read_Array(Page_addres_Name + 2,(uint8_t *)Name_Slaver[0], 30);
	Flash_Read_Array(Page_addres_QuantitySlave + 2, &ucountCheck, 1);
	Flash_Read_Array(Page_addres_Gate + 2,(uint8_t *)&Gate, 4);
	Flash_Read_Array(Page_addres_Gate + 2 + 4,(uint8_t *)&Send_TextDisplayGateWay, 1);
	Flash_Read_Array(Page_addres_QuantitySlave + 100, (uint8_t *)Pass_Change, 6);
	Flash_Read_Array(Page_addres_QuantitySlave + 100, (uint8_t *)XacNhanLai, 6);
	if(Gate.count == 0xFF && Gate.ID == 0xFF && Gate.status == 0xFF && Send_TextDisplayGateWay == 0xFF){
		Gate.count = 0;
		Gate.ID = 0;
		Gate.status = 0;
		Send_TextDisplayGateWay = 1;
	}
	if(ucountCheck == 0xFF){
		ucountCheck = 0;
	}
  if(checkPage == Page1){
	  Flash_Read_Array(Page_addres_1 + 2,(uint8_t *)Slaver_CardID[0] , 100);
	  PageActive = Page1;
  }else if(checkPage == Page2){
	  Flash_Read_Array(Page_addres_2 + 2,(uint8_t *)Slaver_CardID[0] , 100);	  
	  PageActive = Page2;
  }else if(checkPage == 0xFF){
	  Flash_Read_Array(Page_addres_3 + 2,(uint8_t *)Slaver_CardID[0] , 100);
  }
}
void Slave_Name_Add(void){
	static bool Old_Enter_key;
	static uint8_t ucountAdd = 0;
	LCD20X4_Clear();
	while(1){
		key = keyPad();
		for(ucountName = 0; ucountName < 5; ucountName++){
			if(Name_Slaver[ucountName][0] == 0xFF && Name_Slaver[ucountName][1] == 0xFF && Name_Slaver[ucountName][2] == 0xFF){
				while(1){
					key = keyPad();
					Display_Slave_Name();
					if( key && !Old_Enter_key){
						if(key != 0x41){
							Name_Slaver[ucountName][ucountAdd] = key;
							ucountAdd++;
						}
						if(key == 0x41){
							if(ucountAdd == 1 && Name_Slaver[ucountName][0] != 0xFF) Name_Slaver[ucountName][0] = 0xFF;
							if(ucountAdd == 2 && Name_Slaver[ucountName][1] != 0xFF) Name_Slaver[ucountName][1] = 0xFF;
							if(ucountAdd == 3 && Name_Slaver[ucountName][2] != 0xFF){
								Name_Slaver[ucountName][2] = 0xFF;
								ucountAdd = 0;
							}
							ucountAdd--;
							Display_Name_slave[ucountAdd] = Name_Slaver[ucountName][ucountAdd];
							LCD20X4_Gotoxy(12,1);
							LCD20X4_PutString((char *)Display_Name_slave);
						}
						if(ucountAdd > 3) ucountAdd = 0;
						if(Name_Slaver[ucountName][0] != 0xFF || Name_Slaver[ucountName][1] != 0xFF || Name_Slaver[ucountName][2] != 0xFF || ucountAdd == 3){
							memcpy(Display_Name_slave, Name_Slaver[ucountName],sizeof(Name_Slaver[ucountName]));
						}
					}Old_Enter_key = key;
					if(Name_Slaver[ucountName][0] != 0xFF && Name_Slaver[ucountName][1] != 0xFF && Name_Slaver[ucountName][2] != 0xFF && ucountAdd == 3){
						break;
					}
				}
				if(ucountAdd == 3){
					break;
				}
			}
			if(ucountAdd == 3){
					break;
				}
		}
		if(key == 0x44 || ucountAdd == 3){
			ucountAdd = 0;
			break;
		}
	}
}
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}
static void MX_SPI1_Init(void)
{
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_TIM4_Init(void)
{
  TIM_SlaveConfigTypeDef sSlaveConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 71;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 6000;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sSlaveConfig.SlaveMode = TIM_SLAVEMODE_DISABLE;
  sSlaveConfig.InputTrigger = TIM_TS_ITR0;
  if (HAL_TIM_SlaveConfigSynchro(&htim4, &sSlaveConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 3000;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_TIM_MspPostInit(&htim4);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, CS_RFID_Pin|GPIO_PIN_8|GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_12|GPIO_PIN_13
                          |GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);
  
  GPIO_InitStruct.Pin = CS_RFID_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*Configure GPIO pins : PA8 PA9 */

   GPIO_InitStruct.Pin =GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  
  /*Configure GPIO pins : PB0 PB1 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
 /*Configure GPIO pins : PB12 PB13
                           PB14 PB15 */
 GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13
                          |GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  
  /* configure GPIO pin:13 */
  
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  
  
 /* Config GPIO KeyPad 4x4 */
	GPIO_InitStruct.Pin =ROW_3_Pin | ROW_4_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(ROW_PORT_B, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = ROW_1_Pin | ROW_2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(ROW_PORT, &GPIO_InitStruct);
  
	GPIO_InitStruct.Pin = COL_1_Pin | COL_2_Pin | COL_3_Pin | COL_4_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(COL_PORT, &GPIO_InitStruct);
 
 
}

void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
