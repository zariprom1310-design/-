import serial
import time

# Настройки порта (замените COM3 на ваш порт из Arduino IDE)
SERIAL_PORT = 'COM3'
BAUD_RATE = 115200

try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    time.sleep(2)  # Ждем инициализации Arduino
    print(f"Подключено к {SERIAL_PORT}")
except Exception as e:
    print(f"Ошибка подключения: {e}")
    exit()


def send_command(cmd_char):
    """Отправляет команду на пульт"""
    # Превращаем '1' в байт 1 и т.д.
    val = int(cmd_char).to_bytes(1, 'big')
    ser.write(val)
    print(f"Команда {cmd_char} отправлена")


def read_telemetry():
    """Читает данные от спутника, если они есть"""
    if ser.in_waiting > 0:
        line = ser.readline().decode('utf-8').strip()
        if line.startswith("DATA"):
            parts = line.split(',')
            print(f"[ТЕЛЕМЕТРИЯ] Режим: {parts[1]} | ГОР: {parts[2]}° | ВЕРТ: {parts[3]}°")


print("\nУправление спутником:")
print("1 - Горизонтальный скан")
print("2 - Вертикальный скан")
print("3 - Диагональ 1")
print("4 - Диагональ 2")
print("0 - Стоп/Сброс")
print("q - Выход")

try:
    while True:
        # Читаем ввод пользователя
        if ser.in_waiting == 0:  # Если в буфере пусто, просим ввод
            user_input = input("Введите команду: ")

            if user_input.lower() == 'q':
                break

            if user_input in ['0', '1', '2', '3', '4']:
                send_command(user_input)

        # Проверяем наличие телеметрии
        read_telemetry()
        time.sleep(0.1)

except KeyboardInterrupt:
    print("\nЗавершение работы...")
finally:
    ser.close()