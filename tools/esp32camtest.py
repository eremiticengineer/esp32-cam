import serial
import time

PORT = "/dev/ttyUSB0"
BAUD = 115200

ser = serial.Serial(PORT, BAUD, timeout=0.1)

buffer = bytearray()


# -----------------------------
# LOW LEVEL READ
# -----------------------------
def read_uart():
    data = ser.read(512)
    if data:
        buffer.extend(data)


# -----------------------------
# FIND IMAGE FRAME
# -----------------------------
def extract_image():
    """
    Robust resync parser:
    - ignores garbage
    - finds !LEN!<data>
    - recovers from desync safely
    """

    global buffer

    while True:
        # 1. find start of frame
        start = buffer.find(b'!')
        if start == -1:
            buffer.clear()
            return None, None

        # drop junk before frame
        if start > 0:
            del buffer[:start]

        # 2. find second !
        second = buffer.find(b'!', 1)
        if second == -1:
            return None, None  # need more data

        # 3. parse length safely
        length_str = buffer[1:second].decode(errors="ignore")

        if not length_str.isdigit():
            # bad frame → slide forward by 1 byte only
            del buffer[0:1]
            continue

        length = int(length_str)

        # 4. check full payload availability
        total_size = second + 1 + length
        if len(buffer) < total_size:
            return None, None  # wait for more bytes

        # 5. extract image
        img = buffer[second + 1:total_size]

        # 6. consume frame
        del buffer[:total_size]

        return length, img


# -----------------------------
# WAIT FOR READY
# -----------------------------
print("Probing ESP32-CAM...")

while True:
    ser.write(b"#ok#")
    read_uart()

    if b"@ok@" in buffer:
        print("ESP32-CAM READY")
        buffer.clear()
        break

    time.sleep(0.2)


# -----------------------------
# REQUEST IMAGE
# -----------------------------
print("Requesting image...")
ser.write(b"#i:c#")


# -----------------------------
# RECEIVE IMAGE STREAM
# -----------------------------
while True:
    read_uart()

    length, img = extract_image()

    if img is not None:
        print(f"Found image length: {length} bytes")

        with open("/tmp/image.jpg", "wb") as f:
            f.write(img)

        print("Saved image.jpg")
        break

    time.sleep(0.01)