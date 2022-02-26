from distutils.file_util import write_file
from functools import reduce
import logging
import os
import threading


class GPS_Z9P:
    def __init__(self, device_path="/dev/ttyACM0"):
        self.file = open(device_path)
        self.write_file = open(device_path, "wb")
        self.thread = threading.Thread(target=self.__read_thread, daemon=True)
        self.last_line = ""
        self.lat = 0
        self.lon = 0
        self.reads_per_sec = 10

    def get_coords(self):
        return {"lat": self.lat, "lon": self.lon}

    def __parse(self, message_str):
        if isinstance(message_str, str):
            split = message_str.split(",")
            # look for lat/lon messages. We'll almost certainly get
            # GN messages (maybe only GN messages, but we could theoretically accept other talkers)
            if split[0].startswith("$") and split[0].endswith("GLL"):
                print(message_str)
                computed_checksum = 0
                for char in message_str:
                    if char == "*":
                        # marks the end of the portion to hash
                        break
                    elif char == "$":
                        # marks the beginning
                        computed_checksum = 0
                    else:
                        computed_checksum ^= ord(char)

                computed_checksum = format(computed_checksum, "X")
                message_checksum = (
                    message_str.split("*")[-1].replace("\n", "").replace("\r", "")
                )
                if computed_checksum != message_checksum:
                    logging.warning(
                        "`"
                        + format(computed_checksum, "X")
                        + "` did not match `"
                        + message_str.split("*")[-1]
                        + "`"
                    )
                    return

                # This is just a big pile of slicing up, the goal is to take:
                # $GNGLL,3511.93307,N,09721.15557,W,011244.00,A,D*64
                # and output the lat and lon.
                # It does this by:
                # - Parsing out the integer number of degrees (2 digits for lat, 3 digits for lon)
                # - Parsing out the minutes, and dividing them by 60, then adding them to the integer degrees
                # - Multiplying by -1 if the lat is in the south, or if the lon is in the west
                self.lat = (int(split[1][0:2]) + float(split[1][2:]) / 60) * (
                    -1 if split[2] == "S" else 1
                )
                self.lon = (int(split[3][0:3]) + float(split[3][3:]) / 60) * (
                    -1 if split[4] == "W" else 1
                )
                self.last_line = message_str
            elif not (
                message_str == "\n" or message_str == "\r\n" or message_str == ""
            ) and not message_str.startswith("$"):
                print(
                    "got weird message: " + message_str + " of len " + str(len(split))
                )

    def activate_high_precision(self):
        message = [0xB5, 0x62]  # The first two sync chars

        # now add the class and id
        message.append(0x06)
        message.append(0x8A)

        # we'll figure this out later,
        # for now, append two bytes
        message.append(0xFF)
        message.append(0xFF)

        # now for the payload

        # add the version, layer, and reserved bytes
        message.append(0x00)  # version 0
        message.append(0x01)  # Only update this in ram (i.e. layer 1)
        message.append(0x00)  # Set two reserved bytes to zero, as they must be
        message.append(0x00)

        # add the cfgData

        # set the key ID
        message.append(0x10)
        message.append(0x93)
        message.append(0x00)
        message.append(0x06)

        # set the value (true, in this case)
        message.append(0x01)

        # figure out the payload length, in bytes
        # we appended two bytes of sync chars,
        # a byte for class, a byte for message id,
        # two dummy bytes for length,
        # and everything else is our payload
        # so the byte length of the payload is the
        # length of message minus 6.

        num_bytes_in_payload = len(message) - 6

        # this is a two byte value, so we need to split that off (little endian).
        message[4] = num_bytes_in_payload & 0xFF  # strip off the first byte
        message[5] = (num_bytes_in_payload >> 8) & 0xFF  # strip off the second byte

        # now we need to figure out the checksum, the
        # manual gives us the algorithm, which we'll translate to python
        #
        #
        # CK_A = 0, CK_B = 0
        # For(I=0;I<N;I++)
        # {
        #     CK_A = CK_A + Buffer[I]
        #     CK_B = CK_B + CK_A
        # }
        #
        #

        CK_A = 0
        CK_B = 0
        # Why +4? We compute from the class byte to the end of the payload, in accordance to section 5.4 of the u-blox ZED-F9P Interface Description
        for i in range(0, num_bytes_in_payload + 4):
            CK_A += message[2 + i]
            CK_B += CK_A
            # Quoting the interface description:
            # "The two CK_ values are 8-Bit unsigned integers, only! If implementing with larger-sized integer
            # values, make sure to mask both CK_A and CK_B with 0xFF after both operations in the loop"
            # Okay, then we'll do that!
            CK_A &= 0xFF
            CK_B &= 0xFF
        # Now add the checksum bytes, and we're done!
        message.append(CK_A)
        message.append(CK_B)

        binary_value = reduce(
            lambda accumulator, element: (accumulator << 8) + element, message
        )

        self.write_file.write(bytes(message))
        print("sent" + str(hex(binary_value)))
        read = self.file.read(100)
        print("got" + str(read))

    def get_high_precision(self):
        message = [
            0xB5,
            0x62,
            0x06,
            0x8A,
            0x09,
            0x00,
            0x01,
            0x01,
            0x00,
            0x00,
            0x13,
            0x00,
            0x91,
            0x20,
            0x08,
        ]  # The first two sync chars

        num_bytes_in_payload = len(message) - 6

        # this is a two byte value, so we need to split that off (little endian).
        message[4] = num_bytes_in_payload & 0xFF  # strip off the first byte
        message[5] = (num_bytes_in_payload >> 8) & 0xFF  # strip off the second byte

        CK_A = 0
        CK_B = 0
        for i in range(0, 4 + num_bytes_in_payload):
            print("checking " + hex(message[2 + i]))
            # Quoting the interface description:
            # "The two CK_ values are 8-Bit unsigned integers, only! If implementing with larger-sized integer
            # values, make sure to mask both CK_A and CK_B with 0xFF after both operations in the loop"
            CK_A += message[2 + i] & 0xFF
            CK_A &= 0xFF
            CK_B += CK_A & 0xFF
            CK_B &= 0xFF

        # Now add the checksum bytes, and we're done!
        message.append(CK_A)
        message.append(CK_B)

        # binary_value = reduce(
        #     lambda accumulator, element: (accumulator << 8) + element, message
        # )

        as_byte = bytes(message)
        for b in as_byte:
            print("sent " + hex(b))
        self.write_file.write(as_byte)

    def __read_thread(self):
        while True:
            try:
                line_read = self.file.readline()
                self.__parse(line_read)
            except UnicodeDecodeError:
                print("Unicode decode error, hopefully a ubx message")
            
            
            threading.Event().wait(0.01)

    def start_thread(self):
        print("starting thread...")
        self.thread.start()


def set_interval(func, sec):
    def func_wrapper():
        set_interval(func, sec)
        func()

    t = threading.Timer(sec, func_wrapper)
    t.start()
    return t


def read_line():
    print(gps.get_coords())


gps = GPS_Z9P()


gps.start_thread()
set_interval(read_line, 1)


def wri():
    print("\n\nwriting....\n\n")
    gps.write_file.write(bytes(
        [
            0xb5,
            0x62,
            0x06,
            0x8a,
            0x09,
            0x00,
            0x01,
            0x04,
            0x00,
            0x00,
            0x68,
            0x00,
            0x91,
            0x20,
            0x00,
            0xb7,
            0x4D
        ]
    ))

set_interval(wri, 10)