import logging
import os
import threading;

class GPS_Z9P:

    def __init__(self, device_path="/dev/ttyACM0"):
        self.file = open(device_path)
        self.thread = threading.Thread(target=self.__read_thread, daemon=True)
        self.last_line = ""
        self.lat = 0
        self.lon = 0
        self.reads_per_sec = 4

    def get_coords(self):
        return {
            "lat": self.lat,
            "lon": self.lon
        }
    
    def __parse(self, message_str):
        if isinstance(message_str, str):
            split = message_str.split(",") 
            # look for lat/lon messages. We'll almost certainly get 
            # GN messages (maybe only GN messages, but we could theoretically accept other talkers)
            if split[0].startswith('$') and split[0].endswith("GLL"):
                computed_checksum = 0
                for char in message_str:
                    if char == '*':
                        # marks the end of the portion to hash
                        break
                    elif char == '$':
                        # marks the beginning
                        computed_checksum = 0
                    else:
                        computed_checksum ^= ord(char)

                computed_checksum = format(computed_checksum, 'X')
                message_checksum = message_str.split("*")[-1].replace("\n", "").replace("\r", "")
                if(computed_checksum != message_checksum):
                    logging.warning("`" + format(computed_checksum, 'X') + "` did not match `" + message_str.split("*")[-1] + "`")
                    return
                
                # This is just a big pile of slicing up, the goal is to take:
                # $GNGLL,3511.93307,N,09721.15557,W,011244.00,A,D*64
                # and output the lat and lon.
                # It does this by:
                # - Parsing out the integer number of degrees (2 digits for lat, 3 digits for lon)
                # - Parsing out the minutes, and dividing them by 60, then adding them to the integer degrees
                # - Multiplying by -1 if the lat is in the south, or if the lon is in the west
                self.lat = (int(split[1][0:2]) + float(split[1][2:]) / 60) * (-1 if split[2] == "S" else 1)
                self.lon = (int(split[3][0:3]) + float(split[3][3:]) / 60) * (-1 if split[4] == "W" else 1)
                self.last_line = message_str
                



    def __read_thread(self):
        while True:
            line_read = self.file.readline()
            self.__parse(line_read)
            threading.Event().wait(self.reads_per_sec / 1000)
    
    

    def start_thread(self):
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