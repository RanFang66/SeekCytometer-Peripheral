#include "MCP4728.h"
#include "bsp_simu_i2c.h"

#define MCP4728_ADDR 0x60
uint8_t mcp4728_write_single(uint8_t chn, uint16_t mv)
{
    uint16_t Dn;
    uint8_t byte[3];
    Dn = ( uint16_t )(mv);      // Vout*4096/2.048/2

    switch( chn ){
    case 1:
        byte[0] = 0x58; // UDAC = 0,upload output
        break;
    case 2:
        byte[0] = 0x5A; // UDAC = 0
        break;
    case 3:
        byte[0] = 0x5C; // UDAC = 0
        break;
    case 4:
        byte[0] = 0x5E; // UDAC = 0
        break;
    default:
        byte[0] = 0x58; // default channel A
        break;
    }

    Dn = Dn&0x0FFF;
    Dn = 0x9000|Dn;     // vref=1, PD0=0,PD1=0,Gx=0
    byte[1] = ( uint8_t )(Dn >> 8);
    byte[2] = ( uint8_t )(Dn);

//    IIC_Start();
//    IIC_Send_Byte( byte1 );
//    IIC_Wait_Ack();
//    IIC_Send_Byte( byte2 );
//    IIC_Wait_Ack();
//    IIC_Send_Byte( byte3 );
//    IIC_Wait_Ack();
//    IIC_Send_Byte( byte4 );
//    IIC_Wait_Ack();
//    IIC_Stop();

   return  SI2C_writeData(MCP4728_ADDR, byte, 3);
 //   return 0;
}

