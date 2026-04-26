#!/bin/bash
set -euo pipefail

# Uso: ./compile.sh [TARGET]
# Ejecuta la compilación dentro del contenedor Docker 'TP_ARQUI_04'.

NAME="TP_ARQUI_04"
TARGET_ARG="${1:-${TARGET_ARG:-qemu}}"

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

echo "Compilando dentro del contenedor (${NAME}) con TARGET='${TARGET_ARG}'..."

docker exec -u root "${NAME}" bash -lc \
	"cd /root && make clean || true && cd Toolchain && make clean || true && make all || exit 1 && cd /root && make TARGET=\"${TARGET_ARG}\" all"

echo "Compilación finalizada."

