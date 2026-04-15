import serial
import struct
import time

# ESP32-CAM is on FTDI
PORT = "/dev/ttyUSB0"
BAUD = 115200
OUTPUT_FILE = "/tmp/image.jpg"
COMMAND = b"pic:capture#"

# Connect each time to ignore boot noise
def try_connect():
    ser = serial.Serial(PORT, BAUD, timeout=0.3)
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    return ser

# Send the probe message. If no response, disconnect and try again and again...
def wait_for_esp():
    while True:
        try:
            ser = try_connect()

            print("Probing ESP32...")

            for _ in range(10):  # small burst of attempts
                ser.write(b"ok#")
                resp = ser.read(2)

                if resp == b"ok":
                    print("ESP32 ready")
                    return ser

                time.sleep(0.2)

            print("No response, reconnecting...")

        except serial.SerialException:
            print("Serial error, retrying...")

        try:
            ser.close()
        except:
            pass

        time.sleep(2)  # give ESP time to settle/reset

def wait_for_header(ser, header=b"IMG:"):
    buffer = b""
    while True:
        byte = ser.read(1)
        if not byte:
            raise RuntimeError("Timeout waiting for header")

        buffer += byte

        if buffer.endswith(header):
            return

def read_exact(ser, size):
    """Read exactly 'size' bytes from serial."""
    data = b""
    while len(data) < size:
        chunk = ser.read(size - len(data))
        if not chunk:
            raise RuntimeError("Timeout or connection lost")
        data += chunk
    return data


def main():
    with serial.Serial(PORT, BAUD, timeout=10) as ser:
        ser = wait_for_esp()

        ser.reset_input_buffer()

        ser.write(COMMAND)

        print("Waiting for header...")
        wait_for_header(ser)

        print("Header found")

        raw_len = read_exact(ser, 4)
        data_len = struct.unpack("<I", raw_len)[0]

        print(f"Incoming data length: {data_len}")

        payload = read_exact(ser, data_len)

        with open(OUTPUT_FILE, "wb") as f:
            f.write(payload)

        print(f"Saved to {OUTPUT_FILE}")


if __name__ == "__main__":
    main()
