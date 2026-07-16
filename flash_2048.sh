#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ELF_FILE="${ELF_FILE:-$PROJECT_DIR/STM32CubeIDE/Debug/2048.elf}"

find_openocd() {
  if [[ -n "${OPENOCD_BIN:-}" ]]; then
    [[ -x "$OPENOCD_BIN" ]] && return 0
    echo "OPENOCD_BIN không chạy được: $OPENOCD_BIN" >&2
    return 1
  fi

  if command -v openocd >/dev/null 2>&1; then
    OPENOCD_BIN="$(command -v openocd)"
    return 0
  fi

  shopt -s nullglob
  local candidates=(
    "$HOME"/st/stm32cubeide_*/plugins/com.st.stm32cube.ide.mcu.externaltools.openocd.*linux64_*/tools/bin/openocd
    "$HOME"/STMicroelectronics/STM32Cube/STM32CubeIDE_*/plugins/com.st.stm32cube.ide.mcu.externaltools.openocd.*linux64_*/tools/bin/openocd
  )
  shopt -u nullglob
  for OPENOCD_BIN in "${candidates[@]}"; do
    [[ -x "$OPENOCD_BIN" ]] && return 0
  done
  echo "Không tìm thấy OpenOCD. Hãy đặt biến OPENOCD_BIN." >&2
  return 1
}

find_scripts() {
  if [[ -n "${OPENOCD_SCRIPTS:-}" ]]; then
    [[ -d "$OPENOCD_SCRIPTS" ]] && return 0
    echo "OPENOCD_SCRIPTS không tồn tại: $OPENOCD_SCRIPTS" >&2
    return 1
  fi

  local system_scripts
  for system_scripts in /usr/share/openocd/scripts /usr/local/share/openocd/scripts; do
    if [[ -d "$system_scripts" ]]; then
      OPENOCD_SCRIPTS="$system_scripts"
      return 0
    fi
  done

  shopt -s nullglob
  local candidates=(
    "$HOME"/st/stm32cubeide_*/plugins/com.st.stm32cube.ide.mcu.debug.openocd_*/resources/openocd/st_scripts
    "$HOME"/STMicroelectronics/STM32Cube/STM32CubeIDE_*/plugins/com.st.stm32cube.ide.mcu.debug.openocd_*/resources/openocd/st_scripts
  )
  shopt -u nullglob
  for OPENOCD_SCRIPTS in "${candidates[@]}"; do
    [[ -d "$OPENOCD_SCRIPTS" ]] && return 0
  done
  echo "Không tìm thấy OpenOCD scripts. Hãy đặt biến OPENOCD_SCRIPTS." >&2
  return 1
}

find_openocd
find_scripts

if [[ ! -f "$ELF_FILE" ]]; then
  echo "Chưa có firmware: $ELF_FILE" >&2
  echo "Hãy build cấu hình Debug trong STM32CubeIDE trước." >&2
  exit 1
fi

echo "Đang nạp: $ELF_FILE"
echo "Giữ joystick ở vị trí giữa trong lúc kit khởi động."

exec "$OPENOCD_BIN" \
  -s "$OPENOCD_SCRIPTS" \
  -f interface/stlink-dap.cfg \
  -f target/stm32f4x.cfg \
  -c "adapter speed 1800" \
  -c "program {$ELF_FILE} verify reset exit"
