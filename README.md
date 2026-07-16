# Game 2048 trên STM32F429

Firmware game 2048 hoàn chỉnh cho kit STM32F429I-DISCO/STM32F429ZITx, màn
hình ILI9341 tích hợp và joystick analog hai trục B103.

Xem [KIT_SETUP_AND_RUN.md](KIT_SETUP_AND_RUN.md) để biết cách nối dây, import
dự án, build, nạp firmware và xử lý lỗi thường gặp.

## Mở và chạy dự án

1. Mở STM32CubeIDE.
2. Chọn **File > Import > Existing Projects into Workspace**.
3. Chọn thư mục `STM32CubeIDE` bên trong repository.
4. Build cấu hình **Debug**.
5. Nối kit qua cổng USB ST-LINK rồi chọn **Run** hoặc **Debug**.
6. Thả joystick ở vị trí giữa trong khi splash `2048` hiển thị để firmware tự
   hiệu chuẩn.

File firmware sau khi build nằm tại `STM32CubeIDE/Debug/2048.elf` và không
được lưu trong Git.

## Nối joystick

> Không cấp 5 V cho joystick khi VRx/VRy nối trực tiếp với ADC của STM32. Hãy
> nối chân có nhãn `+5V`/`VCC` của module vào nguồn `3 V` hoặc `3.3 V` của kit.

| Chân B103 | Chân STM32F429 | Chức năng |
|---|---|---|
| GND | GND | Mass chung |
| +5V/VCC | 3 V hoặc 3.3 V | Nguồn an toàn cho module |
| VRx | PA5 | ADC1 channel 5 |
| VRy | PC3 | ADC1 channel 13 |
| SW | PE6 | Nút active-low với internal pull-up |

Nếu hướng vật lý bị đảo, sửa `JOY_SWAP_XY`, `JOY_INVERT_X` hoặc
`JOY_INVERT_Y` ở đầu `Core/Src/joystick.c`, sau đó build lại.

## Điều khiển

- Đẩy joystick để di chuyển; giữ một hướng để tự lặp.
- Nhấn ngắn khi đang chơi để undo nước đi hợp lệ gần nhất.
- Giữ nút 0,8 giây để mở xác nhận chơi lại.
- Nhấn ngắn để xác nhận restart, tiếp tục sau khi thắng hoặc chơi lại sau game
  over.
- Di chuyển joystick để hủy màn hình xác nhận restart.

Best score được lưu trong flash sector 23 và vẫn còn sau khi mất nguồn. Việc
ghi được trì hoãn năm giây hoặc ép ghi trước restart/game-over để giảm hao mòn
flash. LCD sử dụng hai framebuffer ARGB8888 trong SDRAM ngoài và đổi buffer
theo VBlank để giảm xé hình.

## Test game engine

Chạy từ thư mục gốc repository:

```sh
gcc -std=c11 -Wall -Wextra -Werror -ICore/Inc \
  Core/Src/game2048.c tests/test_game2048.c \
  -o /tmp/test_game2048
/tmp/test_game2048
```

## Tác giả

- Dũng — game engine và application controller
- Sơn — cấu hình STM32, joystick, flash và công cụ build/nạp
- Thông — LCD, animation và tài liệu

Code do nhóm viết được phát hành theo giấy phép [MIT](LICENSE). Các file của
STMicroelectronics giữ nguyên giấy phép được ghi trong từng file hoặc thư mục.
