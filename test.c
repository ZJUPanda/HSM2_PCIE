/*
 * @Author: your name
 * @Date: 2021-07-27 15:56:45
 * @LastEditTime: 2021-07-27 17:36:48
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /HSM2_PCIE/test.c
 */
#include "libHSM2.h"

void sign_test()
{
    device_t *dev;
    open_device(&dev);
    SM2_Init(dev, BASE_ADDR1);
    U32 rand[8] = {
        0x12345678, 0x12345678, 0x12345678, 0x12345678,
        0x12345678, 0x12345678, 0x12345678, 0x12345678};
    U32 pri_key[8] = {
        0x3ea27606, 0xca83bffa, 0x3b946b0c, 0xdb8ff076,
        0xe4dd3d1a, 0xaf2bd6e2, 0x7290cefd, 0xc4365cf6};
    U32 hash[8] = {
        0xfd93ea51, 0x6080a881, 0x1b3a16ff, 0x5f465ff7,
        0x2a1c94b6, 0xa55f6fa5, 0xcb7bd2b2, 0x501023c6};
    U32 sign[16];
    int flag = SM2_Sign(dev, BASE_ADDR1, rand, pri_key, hash, sign);
    printf("sign result: ");
    for (int i = 0; i < 16; i++)
    {
        printf("0x%.8x ", sign[i]);
    }
    printf("\n");
    close_device(dev);
}

void signAndVerify_test()
{
    device_t *dev;
    open_device(&dev);
    SM2_Init(dev, BASE_ADDR1);
    U32 rand[8] = {
        0x12345678, 0x12345678, 0x12345678, 0x12345678,
        0x12345678, 0x12345678, 0x12345678, 0x12345678};
    U32 pri_key[8] = {
        0x3ea27606, 0xca83bffa, 0x3b946b0c, 0xdb8ff076,
        0xe4dd3d1a, 0xaf2bd6e2, 0x7290cefd, 0xc4365cf6};
    U32 hash[8] = {
        0xfd93ea51, 0x6080a881, 0x1b3a16ff, 0x5f465ff7,
        0x2a1c94b6, 0xa55f6fa5, 0xcb7bd2b2, 0x501023c6};
    U32 sign[16];
    int flag = SM2_Sign(dev, BASE_ADDR1, rand, pri_key, hash, sign);
    printf("sign result: ");
    for (int i = 0; i < 16; i++)
    {
        printf("0x%.8x ", sign[i]);
    }
    printf("\n");

    U32 pub_key[16] = {
        0xaeec7b42, 0xb9b67ee4, 0x106a5695, 0x1bfdd0da,
        0x8d1038d3, 0xef5b308b, 0x1354ce6f, 0x43caf93a,
        0x1a37a2c4, 0x5bfd14a4, 0x438410e3, 0x48ae543f,
        0x60b047b8, 0x7f75c8bd, 0xabc4bf77, 0xcabb9530};
    int verify_res = SM2_Verify(dev, BASE_ADDR1, pub_key, hash, sign);
    printf("verify result: %d\n", verify_res);
    close_device(dev);
}

void encrypt_test()
{
    device_t *dev;
    open_device(&dev);
    SM2_Init(dev, BASE_ADDR1);
    U32 rand[8] = {
        0x12345678, 0x12345678, 0x12345678, 0x12345678,
        0x12345678, 0x12345678, 0x12345678, 0x12345678};
    U32 pub_key[16] = {
        0xaeec7b42, 0xb9b67ee4, 0x106a5695, 0x1bfdd0da,
        0x8d1038d3, 0xef5b308b, 0x1354ce6f, 0x43caf93a,
        0x1a37a2c4, 0x5bfd14a4, 0x438410e3, 0x48ae543f,
        0x60b047b8, 0x7f75c8bd, 0xabc4bf77, 0xcabb953a};
    U32 C1[16], S[16];
    int flag = SM2_Encrypt(dev, BASE_ADDR1, rand, pub_key, C1, S);
    printf("encrypt result: \n");
    for (int i = 0; i < 16; i++)
    {
        printf("0x%.8x ", C1[i]);
    }
    printf("\n");
    for (int i = 0; i < 16; i++)
    {
        printf("0x%.8x ", S[i]);
    }
    printf("\n");
    close_device(dev);
}

void decrypt_test()
{
    device_t *dev;
    open_device(&dev);
    SM2_Init(dev, BASE_ADDR1);
    U32 pri_key[8] = {
        0x3ea27606, 0xca83bffa, 0x3b946b0c, 0xdb8ff076,
        0xe4dd3d1a, 0xaf2bd6e2, 0x7290cefd, 0xc4365cf6};
    U32 C1[16] = {
        0x012ce1ec, 0x6e8f4872, 0xf9e46dff, 0xd5e7faf2,
        0x5468f2c3, 0x9c98a243, 0xde91cc36, 0xbf869688,
        0x0587174e, 0x000cfa24, 0xaa0a2e70, 0xa774c199,
        0x9a31fcfa, 0x1c294fb0, 0x1d5c638a, 0xb8cf7f3b};
    U32 S[16];
    int flag = SM2_Decrypt(dev, BASE_ADDR1, pri_key, C1, S);
    printf("decrypt result: ");
    for (int i = 0; i < 16; i++)
    {
        printf("0x%.8x ", S[i]);
    }
    printf("\n");
    close_device(dev);
}

void keyExchange_test()
{
    device_t *dev;
    open_device(&dev);
    SM2_Init(dev, BASE_ADDR1);
    U32 da[8] = {
        0xd641683b, 0x4cc9079a, 0x1424fba5, 0xdc5c4dde,
        0xdc50ee75, 0x3f4607fa, 0x3bf3b78d, 0xb8535b42
    }; // self_d

    U32 kda[8] = {
        0x611781F8, 0xA654915F, 0x9D7CB6B6, 0xB1CABA42,
        0xEF19E024, 0x7175C413, 0x053445F9, 0xE12888BA
    }; // self_r
    U32 kxa[8] = {
        0x87D9C262, 0x9CB8AA30, 0xCC42468F, 0x21A0FF8B,
        0x7EB7DD64, 0x42008764, 0x2AB42D32, 0xADA6F7AC
        // 0x055374A7, 0xA462E44D, 0x48757B83, 0xEC71787E,
        // 0x5D35A8F6, 0x5AC81C09, 0xDF840B53, 0x0A9BFA65
    }; // self_Rx
    U32 kdb[8] = {
        0x2F11D774, 0x676BBADE, 0xF6767F73, 0xA92B4DC4,
        0x1638BA6F, 0x8D6F9A4C, 0x22E866B8, 0xEDD539A7
    };
    U32 kxb[16] = {
        0x407E6AFE, 0x88D2FFE4, 0x30FC9DF8, 0x7302E403,
        0x09B9D5CB, 0xDF9C8250, 0x6E45A964, 0xB6FEA134,
        0x473787CD, 0x6BA30BC4, 0xC78C5B57, 0xBDDE61D9,
        0x438AF01A, 0x3FAFA596, 0x13D1F230, 0x90C1ED9E
    }; // other_R
    U32 xb[16] = {
        0x5F30A2EF, 0x163C5EBF, 0x190946A7, 0x0F4984E6,
        0xFCA2C3BB, 0x0FC593B0, 0x890B4378, 0x778ABB58,
        0x7B561634, 0xFE451B27, 0x7C4A3E5E, 0xABA08156,
        0xDC91A66B, 0x0542E09F, 0xB44EC7B2, 0x923B940C
    }; // other_P

    U32 UV[16];
    SM2_KeyExchange(dev, BASE_ADDR1, kda, kxa, da, kxb, xb, UV);
    printf("UV: ");
    for (int i = 0; i < 16; i++)
    {
        printf("0x%.8x ", UV[i]);
    }
    printf("\n");
    close_device(dev);
}


int main(void)
{

    signAndVerify_test();
    encrypt_test();
    decrypt_test();
    keyExchange_test();
    return 0;
}