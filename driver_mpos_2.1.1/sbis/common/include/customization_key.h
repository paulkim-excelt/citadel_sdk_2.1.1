#include <zephyr/types.h>

#define ENCRYPT_RSA2048 0
#define ENCRYPT_RSA4096 1
#define ENCRYPT_EC_P256 2

extern int encryption;
extern uint8_t RSA2KpublicKeyModulus[256];
extern uint8_t RSA4KpublicKeyModulus[512];
extern uint8_t ECDSApubKey[64];
extern uint32_t CustomerID;
extern uint32_t CustomerSBLConfiguration;
extern uint16_t CustomerRevisionID;
extern uint16_t SBIRevisionID;
