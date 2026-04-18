import serial
import time

PORT = "/dev/ttyUSB0"
BAUD = 115200

ser = serial.Serial(PORT, BAUD, timeout=0.1)

buffer = bytearray()


# -----------------------------
# UART READ
# -----------------------------
def read_uart():
    data = ser.read(512)
    if data:
        buffer.extend(data)


# -----------------------------
# STATE MACHINE
# -----------------------------
STATE_WAIT_HEADER = 0
STATE_READ_DATA = 1

state = STATE_WAIT_HEADER
expected_len = 0


def process_stream():
    global state, expected_len, buffer

    while True:
        read_uart()

        # -------------------------
        # STATE 1: WAIT FOR HEADER
        # -------------------------
        if state == STATE_WAIT_HEADER:

            start = buffer.find(b'!')
            second = buffer.find(b'!', start + 1 if start != -1 else 0)

            if start == -1 or second == -1:
                continue

            length_str = buffer[start + 1:second].decode(errors="ignore")

            if not length_str.isdigit():
                # discard bad byte and retry
                del buffer[start + 1:start + 2]
                continue

            expected_len = int(length_str)

            print(f"Found image length: {expected_len} bytes")

            # consume header only
            del buffer[:second + 1]

            state = STATE_READ_DATA

        # -------------------------
        # STATE 2: READ IMAGE DATA
        # -------------------------
        elif state == STATE_READ_DATA:

            if len(buffer) < expected_len:
                continue

            img = buffer[:expected_len]
            del buffer[:expected_len]

            print("Image complete")

            with open("/tmp/image.jpg", "wb") as f:
                f.write(img)

            return  # done


# -----------------------------
# USAGE
# -----------------------------
print("Probing ESP32-CAM...")

ser.write(b"#ok#")

while True:
    read_uart()
    if b"@ok@" in buffer:
        print("ESP32-CAM READY")
        buffer.clear()
        break
    time.sleep(0.2)

print("Requesting image...")
ser.write(b"#i:c#")

process_stream()