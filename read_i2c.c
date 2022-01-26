#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

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

    u_int8_t read_addr = 0xff;
    u_int8_t num_bytes_available_lowbyte_addr = 0xfd;
    u_int8_t num_bytes_available_highbyte_addr = 0xfe;
    int result = i2c_smbus_read_byte_data(file, read_addr);


    // find how many bytes we have to read
    int low_byte = i2c_smbus_read_byte_data(file, num_bytes_available_lowbyte_addr);
    int high_byte = i2c_smbus_read_byte_data(file, num_bytes_available_highbyte_addr);
    int num_to_read = low_byte + (high_byte << 8);
    printf("%i bytes available \n", num_to_read);


    for(int i = 0; i < num_to_read - 1; i++){
        int b_read = i2c_smbus_read_byte_data(file, read_addr);
        // printf("%x ", b_read);
    }

    printf("completed read\n");

    


}
