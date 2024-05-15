#include <LCD20X4.h>
void LCD20X4_Enable(void)
{
	HAL_GPIO_WritePin(LCD_EN_PORT, LCD_EN, GPIO_PIN_SET);
    HAL_Delay(1);
	HAL_GPIO_WritePin(LCD_EN_PORT, LCD_EN, GPIO_PIN_RESET);
    HAL_Delay(1);
}


 void LCD20X4_Send4Bit (uint8_t  Data ){
	 HAL_GPIO_WritePin(LCD_D4_PORT, LCD_D4, (GPIO_PinState)(Data & 0x01));
	 HAL_GPIO_WritePin(LCD_D5_PORT, LCD_D5, (GPIO_PinState)((Data >> 1) & 0x01));
	 HAL_GPIO_WritePin(LCD_D6_PORT, LCD_D6, (GPIO_PinState)((Data >> 2) & 0x01));
	 HAL_GPIO_WritePin(LCD_D7_PORT, LCD_D7, (GPIO_PinState)((Data >> 3) & 0x01));
}

void LCD20X4_SendCommand (uint8_t  Command,uint8_t reg ){
	 if(reg == 0)	HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS, GPIO_PIN_RESET);
	 else				HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS, GPIO_PIN_SET);
		LCD20X4_Send4Bit  ( Command >>4 );   /* Gui 4 bit cao */
		LCD20X4_Enable () ;
		LCD20X4_Send4Bit  ( Command  );      /* Gui 4 bit thap*/
		LCD20X4_Enable () ;
}

void LCD20X4_Clear(){
    LCD20X4_SendCommand(0x01,0);// goi lenh dieu khien LCD
    HAL_Delay(1);
}

 void LCD20X4_Init ( void ){
    LCD20X4_Send4Bit(0x00);
	HAL_Delay(1);
	// HAL_GPIO_WritePin(LCD_PORT, LCD_RW, GPIO_PIN_RESET);
	 HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS, GPIO_PIN_RESET);
	 HAL_GPIO_WritePin(LCD_EN_PORT, LCD_EN, GPIO_PIN_RESET);
    LCD20X4_Send4Bit(0x03);
    LCD20X4_Enable();
    HAL_Delay(1);
    LCD20X4_Send4Bit(0x02);
    LCD20X4_Enable();
    HAL_Delay(1);
    LCD20X4_SendCommand(0x28,0);
    LCD20X4_SendCommand(0x0C,0);
    LCD20X4_Clear();           
	LCD20X4_SendCommand(0x06,0);
}

void LCD20X4_Gotoxy(uint8_t x, uint8_t  y){
  uint8_t  Address;
	switch (y)
	{
		case 0:{	Address = 0x00; break; }//Starting address of 0st line
		case 1:{	Address = 0x40; break; }//Starting address of 1st line
		case 2:{  Address = 0x14; break; }//Starting address of 2st line
		case 3:{  Address = 0x54; break; }//Starting address of 3st line
	}
	Address+=x;
  LCD20X4_SendCommand(0x80|Address, 0);
}

 void LCD20X4_PutChar (char Data ){
    LCD20X4_SendCommand(Data,1);// ghi du lieu
}

void LCD20X4_PutString (char *s){
    while (*s){
        LCD20X4_PutChar(*s);
        s++;
    }
}

void LCD20X4_SendInteger(int iNumber){
	char i, CacKyTu[10];
	if(iNumber != 0){
		if(iNumber < 0) LCD20X4_SendCommand(0x2D, 1);  // dau tru
		for(i = 0; iNumber != 0; i++){
			if(iNumber > 0)
				CacKyTu[i] = iNumber  % 10;
			else
				CacKyTu[i] = (iNumber * -1) % 10;
			iNumber = iNumber / 10;
		}
		while(i > 0){
			LCD20X4_SendCommand(CacKyTu[i - 1] | 0x30, 1);
			i--;
		}
	}
	else{
		LCD20X4_SendCommand(0 + 0x30, 1);
	}
}

void LCD20X4_SendFloat(float fNumber){
	int a, b;
	a = (int)fNumber;
	if (fNumber >= 0.0) b = (fNumber - a) * 10;
	else{
		b = (fNumber - a) * (-10);
		if(fNumber <= 0.0 && fNumber > -1.0)
			LCD20X4_SendCommand(0x2D,1);
	}
	LCD20X4_SendInteger(a);
	if(b >= 0){
		LCD20X4_SendCommand(0x2E,1);
		LCD20X4_SendInteger(b);
	}
}
