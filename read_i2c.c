#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX_MSG_LEN 200

/*

    The default device and addresses are from the very good ZED-F9P integration manual
    (https://cdn.sparkfun.com/assets/learn_tutorials/8/5/6/ZED-F9P_Integration_Manual.pdf)
    Pages 35-37 are of most interest
    
    This code is based off of the example from the kernel docs
    (https://www.kernel.org/doc/Documentation/i2c/dev-interface)

*/


// int file;
// int adapter_num = 1; /* should be dynamically determined */
// char filename[20];

struct GPS_Data{
    int fd;
    int device_address;
};

struct GPS_Data init_gps(int adapter_num, int device_address){
    struct GPS_Data data;

    char filename[20];
    snprintf(filename, 19, "/dev/i2c-%d", adapter_num);
    data.fd = open(filename, O_RDWR);
    if (data.fd < 0)
    {
        /* ERROR HANDLING; you can check errno to see what went wrong */
        fprintf(stderr, "Error opening i2c device %s \n", filename);
        exit(1);
    }

    // select which device on the bus that we want
    // I2C_SLAVE is the ioctl command that says we're
    // targeting a specific address, given by addr
    if (ioctl(data.fd, I2C_SLAVE, device_address) < 0)
    {
        /* ERROR HANDLING; you can check errno to see what went wrong */
        fprintf(stderr, "Error using ioctl to set address of GPS module %s \n", filename);
        exit(1);
    }

    return data;

}

void print_data(struct GPS_Data gps_data)
{

    const unsigned read_addr = 0xff;
    const unsigned char num_bytes_available_lowbyte_addr = 0xfe;
    const unsigned char num_bytes_available_highbyte_addr = 0xfd;

    // find how many bytes we have to read
    int low_byte = i2c_smbus_read_byte_data(gps_data.fd, num_bytes_available_lowbyte_addr);
    int high_byte = i2c_smbus_read_byte_data(gps_data.fd, num_bytes_available_highbyte_addr);
    int num_to_read = low_byte + (high_byte << 8);
    printf("%i bytes available \n", num_to_read);

    // this will hold the entire output of the GPS,
    // which we'll parse later. It's important that you
    // read data as fast as possible, and then handle it
    // so that you don't read a bit of data, parse it,
    // and then try to read some more data by the time
    // a new cycle has begun and then all the data is wrong
    unsigned char output[num_to_read + 1];
    memset(output, '\0', num_to_read + 1);

    char first_read = i2c_smbus_read_byte_data(gps_data.fd, read_addr);
    output[0] = first_read;
    int chars_read = 1; 

    // 0xff indicates no data, according to the integration manual
    if (first_read != 0xff)
    {
        while (chars_read <= num_to_read)
        {
            unsigned char buf[33];
            memset(buf, '\0', 33);
            i2c_smbus_read_i2c_block_data(gps_data.fd, read_addr, 32, buf);
            strncpy((char * restrict)output + chars_read, (const char *)buf, 32);
            chars_read += 32;
        }
    }

    // this whole block verifies the checksum of each message
    // it parses the output character by character.
    // Each message starts with a $, and ends with
    // a *, two characters representing the hex value of the checksum
    // and finally \r\n.
    // The checksum is the xor of all characters between $ and * (exclusive)
    char calculated_checksum = '0';
    int message_start_index = 0;
    for (int i = 0; i <= chars_read; i++)
    {
        // we read data in 32 bit chunks, ocassionally
        // we'll get some dummy data when it doesn't quite
        // line up, so we'll discard the junk (0xff is always junk)
        if (output[i] == 0xff)
            continue;
        if (output[i] == '$')
        {
            // $ denotes the beginning of a message, so
            // we reset all of our bookkeeping data
            calculated_checksum = '0';
            message_start_index = i;
        }
        else if (output[i] == '*')
        {
            // * marks the end of a message, 
            // so we're done computing the checksum

            char message_checksum[3];
            message_checksum[0] = output[i + 1];
            message_checksum[1] = output[i + 2];
            message_checksum[2] = '\0';

            char calculated_string[3];
            snprintf(calculated_string, 3, "%02X", calculated_checksum);
            bool valid_checksum = strcmp(calculated_string, message_checksum) == 0;
            if (!valid_checksum)
            {
                printf("Message corrupted of type: ");
                for (int c = 1; c <= 5; c++)
                {
                    printf("%c", output[message_start_index + c]);
                }
                printf("\n");
            }
            else
            {
                for (int c = message_start_index; c <= i + 4; c++)
                {
                    printf("%c", output[c]);
                }
            }
        }
        else
        {
            // this character isn't a * or a $,
            // so it must be part of the checksum range,
            // so update the checksum with it.
            // (it may not be within that range, for example the \r\n at the end of every message,
            // but when we read a $ we'll reset the cheksum, so it doesn't matter)
            if (calculated_checksum == '0')
                calculated_checksum = output[i];
            else
                calculated_checksum ^= output[i];
        }
    }

}


void read_a_bunch(struct GPS_Data gps_data)
{

    const unsigned read_addr = 0xff;
    const unsigned char num_bytes_available_lowbyte_addr = 0xfe;
    const unsigned char num_bytes_available_highbyte_addr = 0xfd;

    int low_byte = i2c_smbus_read_byte_data(gps_data.fd, num_bytes_available_lowbyte_addr);
    int high_byte = i2c_smbus_read_byte_data(gps_data.fd, num_bytes_available_highbyte_addr);
    int num_to_read = low_byte + (high_byte << 8);
    printf("%i bytes available \n", num_to_read);

    unsigned char output[num_to_read + 1];
    memset(output, '\0', num_to_read + 1);

    char first_read = i2c_smbus_read_byte_data(gps_data.fd, read_addr);
    output[0] = first_read;
    int chars_read = 1; 

    if (first_read != 0xff){
        for(int i = 0; i < 10000; i += 32){
            unsigned char buf[33];
            memset(buf, '\0', 33);
            i2c_smbus_read_i2c_block_data(gps_data.fd, read_addr, 32, buf);
            for(int c = 0; c < 32; c++){
                if(buf[c] == 0xff){
                    printf("_");
                }else printf("%c", buf[c]);
            }
        }
    }else{
        printf("No data to read \n");
    }

}


int main()
{

    // 0x42 is the default address for device, refer to integration guide (page 35, section 4.5.5)  
    // the adapter number should be dynamically determined by inspecting /dev/spi-*
    struct GPS_Data gps_data = init_gps(1, 0x42); 
    // print_data(gps_data);
    print_data(gps_data);
}
