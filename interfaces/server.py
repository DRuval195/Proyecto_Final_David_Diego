import socket
import sys
from ctypes import *
import max30102
import hrcalc

""" This class defines a C-like struct """
class Payload(Structure):
    _fields_ = [("hr", c_float),
                ("spo2", c_float)]
m = max30102.MAX30102()

def main():
    PORT = 2300
    server_addr = ('localhost', PORT)
    ssock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    print("Socket created")

    try:
        # bind the server socket and listen
        ssock.bind(server_addr)
        print("Bind done")
        ssock.listen(3)
        print("Server listening on port {:d}".format(PORT))

        while True:
            csock, client_address = ssock.accept()
            #print("Accepted connection from {:s}".format(client_address[0]))
            buff = csock.recv(512)
            while buff:
                #print("\nReceived {:d} bytes".format(len(buff)))
                red, ir = m.read_sequential()
                HR_value = hrcalc.calc_hr_and_spo2(ir, red)
                #payload_in = Payload.from_buffer_copy(buff)
                #print("Received contents hr={:f}, spo2={:f}".format(payload_in.hr,
                #                                            payload_in.spo2))
                payload_out = Payload(HR_value[0],HR_value[2])
                nsent = csock.send(payload_out)
                print("Sent {:d} bytes".format(nsent))
                buff = csock.recv(512)

            #print("Closing connection to client")
            #print("----------------------------")
            #csock.close()

    except AttributeError as ae:
        print("Error creating the socket: {}".format(ae))
    except socket.error as se:
        print("Exception on socket: {}".format(se))
    except KeyboardInterrupt:
        ssock.close()
        m.shutdown()
    finally:
        print("Closing socket")
        ssock.close()
        m.shutdown()


if __name__ == "__main__":
    main()