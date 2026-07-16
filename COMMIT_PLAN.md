# Kế hoạch commit dự án STM32F429 2048

Tài liệu này chia dự án thành các commit nhỏ theo đúng phần việc của ba thành
viên. Chỉ người thực sự làm phần tương ứng mới thực hiện commit đó.

## 1. Danh tính Git

Trước khi commit phần của mình, từng người chạy:

```sh

```

## 2. Khởi tạo repository

Mở terminal tại thư mục gốc của dự án, sau đó chạy:

```sh
git init -b main
```

Không chạy `git init` trong thư mục cha; repository phải được khởi tạo ngay tại
thư mục gốc của dự án này.

## 3. Danh sách commit

### Commit 1 — Dũng — cấu hình repository

```sh
git add .gitignore .gitattributes LICENSE
git commit -m "chore(repo): initialize repository metadata"
```

File được commit:

- `.gitignore`
- `.gitattributes`
- `LICENSE`

### Commit 2 — Sơn — cấu hình STM32 và IDE

```sh
git add .mxproject .project 2048.ioc
git add Core/Inc/main.h Core/Inc/stm32f4xx_hal_conf.h Core/Inc/stm32f4xx_it.h
git add Core/Src/main.c Core/Src/stm32f4xx_hal_msp.c
git add Core/Src/stm32f4xx_it.c Core/Src/system_stm32f4xx.c
git add Drivers EWARM STM32CubeIDE
git commit -m "chore(stm32): add CubeMX and CubeIDE project scaffold"
```

Nội dung chính:

- Clock 180 MHz, HSE, ADC1, RNG và GPIO.
- Startup, linker script, HAL, CMSIS và BSP của kit.
- Cấu hình STM32CubeIDE và IAR EWARM có sẵn.
- `STM32CubeIDE/Debug` không được commit vì đã nằm trong `.gitignore`.

`Core/Src/main.c` là file dùng chung nhưng được đặt trong commit này vì phần
lớn nội dung là mã khởi tạo phần cứng do CubeMX sinh ra.

### Commit 3 — Sơn — joystick

```sh
git add Core/Inc/joystick.h Core/Src/joystick.c
git commit -m "feat(input): add calibrated analog joystick input"
```

File được commit:

- `Core/Inc/joystick.h`
- `Core/Src/joystick.c`

Commit này chứa ADC polling cho PA5/PC3, nút PE6, hiệu chuẩn tâm, dead-zone,
đảo trục, hold-repeat và phát hiện joystick mất kết nối.

### Commit 4 — Sơn — lưu best score

```sh
git add Core/Inc/high_score.h Core/Src/high_score.c
git commit -m "feat(storage): persist high score in reserved flash"
```

File được commit:

- `Core/Inc/high_score.h`
- `Core/Src/high_score.c`

Linker script đã được thêm trong commit STM32. Module này dùng flash sector 23,
CRC và trì hoãn ghi để giảm hao mòn flash.

### Commit 5 — Sơn — công cụ nạp firmware

```sh
git add flash_2048.sh
git commit -m "build: add portable OpenOCD flash helper"
```

File được commit:

- `flash_2048.sh`

Script tự tìm OpenOCD hoặc nhận `OPENOCD_BIN` và `OPENOCD_SCRIPTS`; không chứa
đường dẫn cố định theo tên người dùng.

### Commit 6 — Dũng — game engine 2048

```sh
git config user.name "Dũng"
git config user.email "dung@gmail.com"
git add Core/Inc/game2048.h Core/Src/game2048.c
git commit -m "feat(game): implement 2048 game engine"
```

File được commit:

- `Core/Inc/game2048.h`
- `Core/Src/game2048.c`

Commit này chứa board 4x4, di chuyển bốn hướng, ghép ô, tính điểm, sinh ô mới,
kiểm tra thắng và game over.

### Commit 7 — Dũng — unit test

```sh
git add tests/test_game2048.c
git commit -m "test(game): add deterministic game engine tests"
```

File được commit:

- `tests/test_game2048.c`

Chạy test sau commit:

```sh
gcc -std=c11 -Wall -Wextra -Werror -ICore/Inc \
  Core/Src/game2048.c tests/test_game2048.c \
  -o /tmp/test_game2048
/tmp/test_game2048
```

### Commit 8 — Thông — giao diện LCD

```sh
git config user.name "Thông"
git config user.email "thong@gmail.com"
git add Core/Inc/display2048.h Core/Src/display2048.c Utilities
git commit -m "feat(display): render board and animations on LCD"
```

File được commit:

- `Core/Inc/display2048.h`
- `Core/Src/display2048.c`
- `Utilities/Fonts/*`

Commit này chứa splash `2048`, lưới 4x4, màu tile, score/best score, hai
framebuffer, animation và overlay win/game-over/restart.

### Commit 9 — Dũng — application controller

```sh
git config user.name "Dũng"
git config user.email "dung@gmail.com"
git add Core/Inc/app2048.h Core/Src/app2048.c
git commit -m "feat(app): integrate game states and controls"
```

File được commit:

- `Core/Inc/app2048.h`
- `Core/Src/app2048.c`

Commit này kết nối game engine, joystick, LCD, RNG và high score; đồng thời xử
lý undo, restart confirmation, win và game over.

### Commit 10 — Thông — tài liệu tiếng Việt

```sh
git config user.name "Thông"
git config user.email "thong@gmail.com"
git add README.md KIT_SETUP_AND_RUN.md COMMIT_PLAN.md
git commit -m "docs: add Vietnamese hardware and run guide"
```

File được commit:

- `README.md`
- `KIT_SETUP_AND_RUN.md`
- `COMMIT_PLAN.md`

## 4. Kiểm tra trước khi push

```sh
git status --short
git log --format="%h %an <%ae> %s"
```

Kết quả mong đợi:

- Không còn source file chưa được commit.
- Không có `STM32CubeIDE/Debug`, `.elf`, `.map`, `.o`, `.d` hoặc file log trong
  lịch sử.
- Lịch sử hiển thị đủ Dũng, Sơn và Thông với đúng email.
- Host test báo `All game2048 tests passed.`
- Clean build STM32CubeIDE kết thúc với `0 errors, 0 warnings`.

Có thể kiểm tra file build bị commit nhầm bằng:

```sh
git ls-files | grep -E '(^|/)Debug/|\.(elf|map|o|d|su|cyclo|log)$'
```

Lệnh trên không được in ra kết quả nào.

## 5. Tạo repository GitHub và push

Dũng đăng nhập lại GitHub CLI bằng tài khoản `Dungpd666`:

```sh
gh auth login -h github.com
gh repo create Dungpd666/stm32f429-2048 \
  --public --source=. --remote=origin
git push -u origin main
```

Không push trước khi cả ba người đã hoàn thành commit của mình và toàn bộ test,
build đều thành công.
