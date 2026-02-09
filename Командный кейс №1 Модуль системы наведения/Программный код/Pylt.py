import serial
import time
import threading

# ПОРТ ПУЛЬТА (посмотрите в Диспетчере устройств)
PORT = 'COM10'


def serial_listener(ser):
    while True:
        try:
            if ser.in_waiting:
                line = ser.readline().decode(errors='ignore').strip()
                if "DATA:" in line:
                    print(f"\n[ТЕЛЕМЕТРИЯ]: {line}")
                    with open("satellite_log.txt", "a") as f:
                        f.write(f"{time.strftime('%Y-%m-%d %H:%M:%S')} - {line}\n")
                else:
                    print(f"\n[СИСТЕМА]: {line}")
        except:
            break


try:
    ser = serial.Serial(PORT, 9600, timeout=1)
    time.sleep(2)
    threading.Thread(target=serial_listener, args=(ser,), daemon=True).start()

    print("--- УПРАВЛЕНИЕ СПУТНИКОМ ЗАПУЩЕНО ---")
    print("Команды: 1-4 (Сканирование), 9 (Центровка), 0 (Стоп)")

    while True:
        command = input("Введите команду >> ")
        if command:
            ser.write(command.encode())

except Exception as e:
    print(f"Ошибка подключения: {e}")