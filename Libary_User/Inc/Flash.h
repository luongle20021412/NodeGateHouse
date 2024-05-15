#ifndef _FLASH_H_
#define _FLASH_H_
#include <stdint.h>
#include <string.h>

// define page address0
#define _PAGE_63_ 		 	((uint32_t)0x0800FC00)		// 0x8000000 + 63 * 1024
#define _PAGE_62_ 		 	((uint32_t)0x0800F800)
#define _PAGE_61_ 		 	((uint32_t)0x0800F400)
#define _PAGE_60_ 		 	((uint32_t)0x0800F000)
#define _PAGE_59_ 		 	((uint32_t)0x0800EC00)
#define _PAGE_58_ 		 	((uint32_t)0x0800E800)
#define _PAGE_57_ 		 	((uint32_t)0x0800E400)
#define _PAGE_56_ 		 	((uint32_t)0x0800E000)
#define _PAGE_55_ 		 	((uint32_t)0x0800DC00)
#define _PAGE_54_ 		 	((uint32_t)0x0800D800)

#define Page_addres_1 		_PAGE_61_
#define Page_addres_2 		_PAGE_62_
#define Page_addres_3 		_PAGE_63_
#define Page_addres_Pass 	_PAGE_60_
#define Page_addres_Name	_PAGE_59_
#define Page_addres_QuantitySlave _PAGE_58_
#define Page_addres_Gate 	_PAGE_57_

void Flash_Erase(uint32_t Address);
void Flash_Write_Int(uint32_t Address, uint16_t i_Value);
void Flash_Write_Float(uint32_t Address,float  f_Value);
void Flash_Write_Array(uint32_t Address, uint8_t *Array, uint16_t Length);


int Flash_Read_Int(uint32_t Address);
float Flash_Read_Float(uint32_t Address);
void Flash_Read_Array(uint32_t Address, uint8_t *Array, uint16_t Length);

#endif
