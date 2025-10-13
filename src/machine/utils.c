#include <stdint.h>
#include <string.h>
#include "machine/utils.h"

#define MSB1 0
#define MSB2 1
#define MSB3 2
#define MSB4 3

void full_hex_to_binary(uint32_t hex, char *full_binary)
{
    unsigned char binary_buff[32] = {0};
    for (int i = 0; i < 8; i++)
    {
        convert_to_binary((uint8_t)((hex >> (28 - (4*i))) & 0xF), &binary_buff[4*i]);
    }
    memcpy(full_binary, binary_buff, 32);
}

void convert_to_binary(uint8_t hexdec, unsigned char *binary)
{
    char binary_buff[4] = {0};
    switch(hexdec)
    {
        case 1:
            binary_buff[MSB4] = 1;
        break;
        case 2:
            binary_buff[MSB3] = 1;
        break;
        case 3:
            binary_buff[MSB4] = 1;
            binary_buff[MSB3] = 1;
        break;
        case 4:
            binary_buff[MSB2] = 1;
        break;
        case 5:
            binary_buff[MSB2] = 1;
            binary_buff[MSB1] = 1;
        break;
        case 6:
            binary_buff[MSB3] = 1;
            binary_buff[MSB2] = 1; 
        break;
        case 7:
            binary_buff[MSB4] = 1;
            binary_buff[MSB3] = 1;
            binary_buff[MSB2] = 1;
        break;
        case 8:          
            binary_buff[MSB1] = 1; 
        break;
        case 9:
            binary_buff[MSB4] = 1;
            binary_buff[MSB1] = 1;
        break;
        case 10:
            binary_buff[MSB3] = 1;
            binary_buff[MSB1] = 1;     
        break;
        case 11:
            binary_buff[MSB4] = 1;
            binary_buff[MSB3] = 1;
            binary_buff[MSB1] = 1;
        break;
        case 12:
            binary_buff[MSB2] = 1;
            binary_buff[MSB1] = 1;
        break;
        case 13:
            binary_buff[MSB4] = 1;
            binary_buff[MSB3] = 1;
            binary_buff[MSB1] = 1;
        break;
        case 14:
            binary_buff[MSB3] = 1;
            binary_buff[MSB2] = 1;
            binary_buff[MSB1] = 1;
        break;
        case 15:   
            binary_buff[MSB4] = 1;
            binary_buff[MSB3] = 1;
            binary_buff[MSB2] = 1;
            binary_buff[MSB1] = 1; 
        break;
        default:
            for (int i = 0; i < 4; i++) binary_buff[i] = 0;
        
    }
    
    memcpy(binary, binary_buff, 4);
}