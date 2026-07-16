# Hướng dẫn lắp kit và chạy game 2048 trên STM32F429

Tài liệu này hướng dẫn từng bước cách nối phần cứng, mở dự án bằng
STM32CubeIDE, biên dịch, nạp chương trình và chơi game.

## 1. Phần cứng cần chuẩn bị

- Kit STM32F429I-DISCO hoặc STM32F429I-DISC1 Discovery
- Màn hình ILI9341 240 x 320 có sẵn trên kit
- Joystick analog hai trục B103 có nút nhấn
- 5 dây nối cái-cái
- Cáp USB Mini-B **có truyền dữ liệu** cho ST-LINK trên kit
- Máy tính đã cài STM32CubeIDE

> Dự án sử dụng đúng sơ đồ kết nối LCD và SDRAM của kit STM32F429I Discovery.
> Một bo mạch chỉ có chip STM32F429ZITx sẽ không đủ nếu LCD và SDRAM không được
> nối giống kit Discovery.

## 2. Xác định chân joystick

Theo thông tin trên joystick của bạn, thứ tự chân khi nhìn từ mặt sau là:

```text
GND | +5V | VRx | VRy | SW
```

- `VRx`: tín hiệu analog của trục ngang.
- `VRy`: tín hiệu analog của trục dọc.
- `SW`: nối xuống GND khi nhấn cần joystick.
- `B103`: mỗi trục sử dụng biến trở tuyến tính 10 kΩ.

## 3. Ngắt nguồn trước khi nối dây

Rút cáp USB khỏi kit trước khi cắm, tháo hoặc thay đổi bất kỳ dây nối nào.

Không nối chân có nhãn `+5V` của joystick vào nguồn 5 V. Module joystick hoạt
động được với 3.3 V. Nếu cấp 5 V, hai đầu ra `VRx` và `VRy` có thể lên gần 5 V
và làm hỏng chân ADC của STM32.

## 4. Nối joystick với kit

| Chân joystick | Chân trên kit Discovery | Vị trí header | Chức năng trong chương trình |
|---|---|---|---|
| `GND` | Một chân `GND` bất kỳ | — | Mass chung |
| `+5V`/`VCC` | `3 V` hoặc `3.3 V` | — | Nguồn an toàn cho joystick |
| `VRx` | `PA5` | P2 chân 21 | ADC1 channel 5 |
| `VRy` | `PC3` | P2 chân 15 | ADC1 channel 13 |
| `SW` | `PE6` | P1 chân 11 | Nút nhấn mức thấp, có pull-up |

Sơ đồ nối dây:

```text
Joystick B103                     STM32F429I Discovery

GND  ---------------------------> GND
+5V  ---------------------------> 3.3 V     (không nối vào 5 V)
VRx  ---------------------------> PA5
VRy  ---------------------------> PC3
SW   ---------------------------> PE6
```

Kiểm tra chữ P1/P2 được in trên bo mạch. Cách bố trí vật lý có thể khác giữa
các phiên bản kit, vì vậy hãy ưu tiên đúng tên chân STM32 là `PA5`, `PC3` và
`PE6`. Tài liệu phần cứng chính thức của kit là ST UM1670.

### Kiểm tra điện áp bằng đồng hồ — không bắt buộc

Trước khi nối `VRx` và `VRy` vào STM32:

1. Chỉ nối GND và nguồn 3.3 V cho joystick.
2. Đo điện áp `VRx` so với GND.
3. Đẩy joystick hết sang trái rồi sang phải.
4. Đo tương tự với `VRy` khi đẩy lên và xuống.
5. Điện áp ở cả hai chân phải luôn nằm trong khoảng 0–3.3 V.

Khi joystick ở giữa, điện áp gần một nửa điện áp nguồn là bình thường. Chương
trình tự đo vị trí giữa nên giá trị không cần chính xác 1.65 V hoặc ADC 2048.

## 5. Chuẩn bị kit Discovery

1. Kiểm tra hai jumper CN4 ST-LINK/DISCOVERY đang được gắn ở vị trí bình thường.
2. Kiểm tra jumper đo dòng JP3 đang được gắn.
3. Giữ nguyên màn hình LCD có sẵn trên kit.
4. Nối cáp USB vào cổng **ST-LINK USB** của kit.
5. Không dùng cổng USB OTG để nạp chương trình.
6. Kiểm tra đèn nguồn trên kit đã sáng.

## 6. Mở STM32CubeIDE

Mở STM32CubeIDE bằng menu ứng dụng. Nếu chưa cài, tải STM32CubeIDE từ trang
chính thức của STMicroelectronics và cài driver ST-LINK phù hợp với hệ điều
hành. Khi CubeIDE yêu cầu workspace, chọn một thư mục có quyền ghi. Cho phép
cập nhật firmware ST-LINK nếu CubeIDE đề nghị.

## 7. Import dự án đã hoàn thành

Không cần tạo dự án CubeMX mới. Clock, ADC polling, RNG, GPIO, LCD BSP, game engine
và linker đã được cấu hình sẵn.

1. Trong CubeIDE, chọn **File > Import**.
2. Chọn **General > Existing Projects into Workspace**.
3. Nhấn **Next**.
4. Ở mục root directory, chọn:

   ```text
   <thu-muc-repository>/STM32CubeIDE
   ```

5. Kiểm tra dự án tên `2048` đã được đánh dấu.
6. Bỏ chọn **Copy projects into workspace**.
7. Nhấn **Finish**.

Phải import thư mục con `STM32CubeIDE`, không chỉ chọn thư mục cha `2048`.
Thư mục con chứa cấu hình dự án CubeIDE và linker script cần thiết.

## 8. Biên dịch chương trình

1. Chọn dự án `2048` trong cửa sổ **Project Explorer**.
2. Nhấp chuột phải và chọn **Refresh**.
3. Chọn cấu hình build **Debug** nếu CubeIDE hỏi.
4. Chọn **Project > Clean** và clean dự án `2048`.
5. Chọn **Project > Build Project** hoặc nhấn biểu tượng cái búa.
6. Mở cửa sổ **Console** và chờ quá trình build hoàn tất.

Build thành công sẽ kết thúc với thông báo tương tự:

```text
Build Finished. 0 errors, 0 warnings.
```

Firmware được tạo tại:

```text
STM32CubeIDE/Debug/2048.elf
```

Dự án hiện tại đã được build thử thành công. Chương trình dùng khoảng 50 KiB
Flash và 3 KiB RAM trong chip. Hai framebuffer của LCD nằm trong SDRAM ngoài.

## 9. Nạp và chạy chương trình

### Lần debug đầu tiên

1. Giữ kit Discovery kết nối với máy tính qua cổng ST-LINK USB.
2. Chọn dự án `2048`.
3. Chọn **Run > Debug As > STM32 C/C++ Application**.
4. Nếu cửa sổ cấu hình xuất hiện, chọn ứng dụng `2048.elf`.
5. Giữ giao tiếp debug là `SWD` và chế độ reset mặc định.
6. Nhấn **Debug**.
7. Đồng ý chuyển sang Debug perspective nếu CubeIDE hỏi.
8. Sau khi nạp, chương trình thường dừng tại hàm `main()`.
9. Nhấn **Resume** hoặc phím `F8` để chương trình chạy.

Ở những lần sau, dùng nút Run màu xanh để nạp và chạy ngay. Dùng nút Debug khi
cần breakpoint hoặc kiểm tra biến.

## 10. Hiệu chuẩn joystick tự động khi khởi động

Mỗi lần reset, chương trình sẽ đo vị trí giữa của joystick:

1. Thả tay khỏi joystick và không nhấn nút SW.
2. Đặt joystick ở chính giữa trước khi nhấn Run hoặc Resume.
3. Giữ nguyên khi màn hình chỉ hiển thị logo **2048**.
4. Sau khoảng 0.6 giây, bàn game sẽ xuất hiện với hai ô số.

Nếu joystick bị giữ lệch trong lúc hiệu chuẩn, hướng điều khiển sẽ không đúng.
Nhấn nút reset màu đen trên kit và hiệu chuẩn lại khi joystick ở giữa.

## 11. Cách điều khiển

| Thao tác joystick | Hoạt động trong game |
|---|---|
| Đẩy trái, phải, lên hoặc xuống | Trượt và ghép các ô số |
| Giữ joystick theo một hướng | Tự lặp nước đi sau một khoảng trễ ngắn |
| Nhấn ngắn khi đang chơi | Undo nước đi hợp lệ gần nhất |
| Giữ nút SW trong 0.8 giây | Mở xác nhận chơi lại |
| Nhấn ngắn tại màn hình xác nhận | Bắt đầu game mới |
| Di chuyển joystick tại màn hình xác nhận | Hủy chơi lại |
| Nhấn ngắn sau khi đạt 2048 | Tiếp tục game hiện tại |
| Nhấn ngắn khi game over | Bắt đầu game mới |

Chương trình tự đo nhiễu và khoảng dead-zone của từng joystick khi khởi động.
Nếu cả hai trục bị kẹt gần 0 hoặc 4095 liên tục, input sẽ bị chặn cho đến khi
tín hiệu hợp lệ trở lại.

## 12. Sửa hướng joystick bị ngược

Tùy theo chiều lắp joystick, một hoặc cả hai trục có thể bị ngược. Mở file:

```text
Core/Src/joystick.c
```

Gần đầu file có các thiết lập:

```c
#define JOY_SWAP_XY 0
#define JOY_INVERT_X 0
#define JOY_INVERT_Y 0
```

Chỉ đổi giá trị cần thiết từ `0` thành `1`:

| Hiện tượng | Thiết lập cần đổi |
|---|---|
| Đẩy lên/xuống nhưng game đi trái/phải | `JOY_SWAP_XY` |
| Trái và phải bị đảo | `JOY_INVERT_X` |
| Lên và xuống bị đảo | `JOY_INVERT_Y` |

Lưu file, build lại và nạp lại chương trình.

## 13. Xử lý lỗi thường gặp

### CubeIDE không hiển thị dự án

Import thư mục `STM32CubeIDE` bên trong repository, không import thư mục cha.

### Báo lỗi `No ST-LINK detected`

- Kiểm tra cáp USB có hỗ trợ dữ liệu, không phải cáp chỉ sạc.
- Dùng cổng ST-LINK USB, không dùng USB OTG.
- Kiểm tra hai jumper CN4 đã được gắn.
- Thử cổng USB hoặc cáp khác.
- Chấp nhận cập nhật firmware ST-LINK nếu CubeIDE yêu cầu.

### LCD chỉ có màu trắng hoặc màu đen

- Kiểm tra bo mạch đúng là STM32F429I Discovery có LCD tích hợp.
- Chọn Clean rồi build lại trước khi nạp.
- Nhấn Resume (`F8`) nếu debugger vẫn đang dừng ở `main()`.
- Nhấn reset và quan sát splash **2048**.

### Joystick không hoạt động

- Kiểm tra `VRx -> PA5` và `VRy -> PC3`.
- Kiểm tra GND của joystick đã nối chung với GND của kit.
- Kiểm tra joystick được cấp nguồn 3.3 V.
- Reset và giữ joystick ở giữa trong toàn bộ quá trình hiệu chuẩn.

### Joystick quá nhạy hoặc không nhận hướng

Firmware tự tính dead-zone từ tâm và độ nhiễu khi khởi động. Hãy reset kit và
không chạm vào joystick trong lúc splash `2048` hiển thị. Nếu vẫn cần chỉnh,
thay đổi `JOY_MIN_TRIGGER_DELTA` và `JOY_MAX_TRIGGER_DELTA` gần đầu file
`Core/Src/joystick.c`, sau đó build và nạp lại.

### Nạp xong nhưng game không chạy

Debugger có thể vẫn đang tạm dừng. Nhấn **Resume** hoặc phím `F8`.

## 14. Thông tin quan trọng của dự án

- CPU chạy ở 180 MHz từ thạch anh HSE 8 MHz.
- ADC1 đọc riêng `PA5` và `PC3` bằng polling mỗi 10 ms; không dùng ADC DMA.
- RNG phần cứng chọn vị trí và giá trị ô số mới.
- Hai framebuffer LCD nằm tại `0xD0000000` và `0xD0050000`, đổi trong VBlank.
- Flash sector 23 lưu điểm cao nhất; ghi trễ 5 giây để giảm hao mòn.
- Nhấn ngắn khi chơi để Undo; giữ hướng joystick để tự lặp nước đi.
- Linker giới hạn chương trình ở 1920 KiB để không ghi đè vùng lưu điểm.
- Không tạo lại toàn bộ dự án CubeIDE nếu không cần thiết, vì các liên kết BSP
  và include path tùy chỉnh đã được cấu hình sẵn.

Thông tin CubeMX, sơ đồ nối dây, cách build và troubleshooting đều được giữ
trong tài liệu này để repository có thể sử dụng độc lập trên máy khác.
