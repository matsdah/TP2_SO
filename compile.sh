#!/bin/bash
set -euo pipefail

# Uso: ./compile.sh [TARGET]
#   TARGET: qemu (default), vbox, usb
#
# Memory manager (variable de entorno MM):
#   MM=FF     -> First-Fit (default)
#   MM=BUDDY  -> Buddy System
#
# Ejemplos:
#   ./compile.sh              # First-Fit, imagen QEMU
#   MM=BUDDY ./compile.sh     # Buddy System, imagen QEMU
#   MM=BUDDY ./compile.sh vbox  # Buddy System, imagen VirtualBox

NAME="TP_SO_2"
TARGET_ARG="${1:-${TARGET_ARG:-qemu}}"
MM_ARG="${MM:-FF}"

if ! command -v docker >/dev/null 2>&1; then
	echo "Docker no está disponible. Instale Docker y vuelva a intentarlo."
	exit 1
fi

if ! docker ps -a --format '{{.Names}}' | grep -qx "${NAME}"; then
	echo "Contenedor ${NAME} no encontrado. Ejecute create.sh para crearlo."
	exit 1
fi

echo "Arrancando (si hace falta) el contenedor ${NAME}..."
docker start "${NAME}" >/dev/null || true

echo "Compilando dentro del contenedor (${NAME}) con TARGET='${TARGET_ARG}' MM='${MM_ARG}'..."

docker exec -u root "${NAME}" bash -lc \
	"cd /root && make clean || true && cd Toolchain && make clean || true && make all || exit 1 && cd /root && make TARGET=\"${TARGET_ARG}\" MM=\"${MM_ARG}\" all"

echo "Compilación finalizada. Memory manager: ${MM_ARG}."
