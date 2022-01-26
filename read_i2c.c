#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_MSG_LEN 200

/*

    The default device and addresses are from the very good ZED-F9P integration manual
    (https://cdn.sparkfun.com/assets/learn_tutorials/8/5/6/ZED-F9P_Integration_Manual.pdf)
    Pages 35-37 are of most interest
    
    This code is based off of the example from the kernel docs
    (https://www.kernel.org/doc/Documentation/i2c/dev-interface)

*/

int main()
{
    int file;
    int adapter_num = 1; /* should be dynamically determined */
    char filename[20];

    // printf("hello, world \n");

    snprintf(filename, 19, "/dev/i2c-%d", adapter_num);
    file = open(filename, O_RDWR);
    if (file < 0)
    {
        /* ERROR HANDLING; you can check errno to see what went wrong */
        fprintf(stderr, "Error opening i2c device %s \n", filename);
        exit(1);
    }

    int addr = 0x42; // default for device, refer to integration guide (page 35, section 4.5.5)

    // select which device on the bus that we want
    // I2C_SLAVE is the ioctl command that says we're
    // targeting a specific address, given by addr
    if (ioctl(file, I2C_SLAVE, addr) < 0)
    {
        /* ERROR HANDLING; you can check errno to see what went wrong */
        fprintf(stderr, "Error using ioctl to set address of GPS module %s \n", filename);
        exit(1);
    }

    unsigned read_addr = 0xff;
    unsigned char num_bytes_available_lowbyte_addr = 0xfe;
    unsigned char num_bytes_available_highbyte_addr = 0xfd;

    // int result = i2c_smbus_read_byte_data(file, read_addr);

    // find how many bytes we have to read
    int low_byte = i2c_smbus_read_byte_data(file, num_bytes_available_lowbyte_addr);
    int high_byte = i2c_smbus_read_byte_data(file, num_bytes_available_highbyte_addr);
    int num_to_read = low_byte + (high_byte << 8);
    printf("%i bytes available \n", num_to_read);

    char output[num_to_read+1];
    memset(output, '\0', num_to_read+1);

    char first_read = i2c_smbus_read_byte_data(file, read_addr);

    int read = 0;
    if (first_read != 0xff)
    {
        while (read <= num_to_read)
        {
            unsigned char buf[33];
            memset(buf, '\0', 33);
            i2c_smbus_read_i2c_block_data(file, read_addr, 32, buf);
            strncpy(output+read, buf, 32);
            read += 32;
        }
    }

    char csum = '0';
    for(int i = 0; i < read; i++){
        if(output[i] == 0xff) continue;
        if(output[i] == '$'){
            csum = '0';
        }else if(output[i] == '*'){
            printf("(csum: %x)", csum);
        }else{
            if(csum == '0') csum = output[i];
            else csum ^= output[i];
        }
        printf("%c", output[i]);
    }

    // if (first_read == 0xff)
    // {
    //     printf("No data awaiting\n");
    //     // return;
    // }
    // else
    // {
    //     char message[MAX_MSG_LEN];
    //     message[0] = first_read;
    //     size_t message_length = 1;
    //     for (int i = 0; i < num_to_read; i++)
    //     {
    //         char b_read = i2c_smbus_read_byte_data(file, read_addr);

    //         if (b_read == '\r')
    //         {
    //             continue;
    //         }
    //         else if (b_read == '\n')
    //         {
    //             printf("%s\n\t", message);
    //             char csum = message[1];
    //             printf("%c", message[1]);
    //             for(int i = 2; i < message_length - 2; i++){
    //                 if(message[i] == '*') break;
    //                 printf("%c", message[i]);
    //                 csum = csum ^ message[i];
    //             }
    //             printf("csum: (%x)\n", csum);
    //             message_length = 0;
    //             memset(message, '\0', MAX_MSG_LEN);
    //         }
    //         else
    //         {
    //             message[message_length] = b_read;
    //             message_length++;
    //         }
    //     }
    //     printf("\n");

    //     printf("completed read\n");

    //     int extra_byte = i2c_smbus_read_byte_data(file, read_addr);
    //     printf(" one more byte is %x \n", extra_byte);
    // }

    // return 0;
}
