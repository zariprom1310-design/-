import serial
import threading
import time
from datetime import datetime

PORT = "COM13"
BAUD = 9600
LOG_FILE = "log.txt"

stop_flag = False


def reader(ser: serial.Serial):
    with open(LOG_FILE, "a", encoding="utf-8") as f:
        while not stop_flag:
            try:
                line = ser.readline().decode("utf-8", errors="ignore").strip()
                if not line:
                    continue

                print(line)

                if line.startswith("DATA:"):
                    ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                    f.write(f"{ts} {line}\n")
                    f.flush()

            except Exception as e:
                print(f"READ_ERR: {e}")
                time.sleep(0.2)


def main():
    global stop_flag
    print(f"Opening {PORT} @ {BAUD}...")
    ser = serial.Serial(PORT, BAUD, timeout=1)

    t = threading.Thread(target=reader, args=(ser,), daemon=True)
    t.start()

    print("READY. Type 0..9 and press Enter. Type q to quit.")
    try:
        while True:
            cmd = input("> ").strip()
            if not cmd:
                continue
            if cmd.lower() == "q":
                break

            c = cmd[0]
            if c < "0" or c > "9":
                print("Only commands 0..9 (or q)")
                continue

            ser.write(c.encode("ascii"))

    finally:
        stop_flag = True
        time.sleep(0.2)
        ser.close()
        print("Closed.")


if __name__ == "__main__":
    main()
