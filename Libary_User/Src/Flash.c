#include <Flash.h>
#include "stm32f1xx_hal.h"

void Flash_Erase(uint32_t Address){
	HAL_FLASH_Unlock();
	FLASH_EraseInitTypeDef EraseInitStruct;
	EraseInitStruct.Banks = 1;
	EraseInitStruct.NbPages = 1;
	EraseInitStruct.PageAddress = Address;
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
	uint32_t PageError;
	HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
	HAL_FLASH_Lock();
}

void Flash_Write_Int(uint32_t Address, uint16_t i_Value){
	HAL_FLASH_Unlock();
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, Address, i_Value);
	HAL_FLASH_Lock();
}

void Flash_Write_Float(uint32_t Address, float f_Value){
	HAL_FLASH_Unlock();
	unsigned char Data[4];
	*(float*)Data = f_Value;
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, *(unsigned long*)Data);
	HAL_FLASH_Lock();
}

void Flash_Write_Array(uint32_t Address, uint8_t *Array, uint16_t Length){
	HAL_FLASH_Unlock();
	uint16_t *Data = (uint16_t*)Array;
	for(char i = 0; i < (Length+1) / 2; i++){
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, Address + 2 * i, *Data);
		Data++;
	}
	HAL_FLASH_Lock();
}

int Flash_Read_Int(uint32_t Address){
	return *(__IO uint16_t *)(Address);
}

float Flash_Read_Float(uint32_t Address){
	unsigned long Data = *(__IO uint32_t *)(Address);
	return *(float*)(&Data);
}

void Flash_Read_Array(uint32_t Address, uint8_t *Array, uint16_t Length){
	uint16_t *Data = (uint16_t *)Array;
	for(int i = 0; i < (Length + 1) / 2; i++){
		*Data = *(__IO uint16_t *)(Address	+ 2 * i);
		 Data++;
	}
}
