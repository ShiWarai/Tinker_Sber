#!/bin/bash

# Скрипт для сборки ROS 2 проекта Tinker_Sber
# Использование: ./scripts/build.sh

set -e  # Остановка при ошибке

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Получаем путь к корню workspace
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Сборка ROS 2 проекта Tinker_Sber${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""

# Переходим в корень workspace
cd "$WORKSPACE_DIR"
echo "Рабочая директория: $WORKSPACE_DIR"
echo ""

# Проверяем наличие colcon
if ! command -v colcon &> /dev/null; then
    echo -e "${RED}Ошибка: colcon не найден!${NC}"
    echo "Установите colcon: sudo apt install python3-colcon-common-extensions"
    exit 1
fi

# Выполняем сборку (исключая stress_test)
# Colcon автоматически делает инкрементальную сборку - пересобирает только измененные пакеты
# Если пересобираются все пакеты, проверьте:
# 1. Не изменились ли package.xml или CMakeLists.txt в зависимостях
# 2. Не изменились ли зависимости пакетов
# 3. Используйте --event-handlers console_direct+ для детального вывода
echo -e "${YELLOW}Сборка всех пакетов${NC}"
echo ""

# Используем --symlink-install для оптимизации (создает симлинки вместо копирования, быстрее для Python скриптов)
# Используем --base-paths src для сборки только пакетов из папки src (игнорируя old_packages)
if colcon build --symlink-install --base-paths src; then
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}Сборка успешно завершена!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    # Source новосозданных пакетов
    if [ -f "$WORKSPACE_DIR/install/setup.bash" ]; then
        echo -e "${YELLOW}Загрузка окружения собранных пакетов...${NC}"
        source "$WORKSPACE_DIR/install/setup.bash"
        echo -e "${GREEN}Окружение загружено!${NC}"
    else
        echo -e "${YELLOW}Предупреждение: install/setup.bash не найден${NC}"
    fi
else
    echo ""
    echo -e "${RED}========================================${NC}"
    echo -e "${RED}Ошибка при сборке!${NC}"
    echo -e "${RED}========================================${NC}"
    exit 1
fi
