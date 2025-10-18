> [!NOTE]
> This README is in Vietnamese. The English README can be read [here](README.md).

<div align="center" class="grid cards" style="display: flex; align-items: center; justify-content: center; text-decoration: none;" markdown>
    <img src="assets/App/AstrocelerateLogo.png" alt="Logo" width="80%">
    <p>Bản quyền © 2024-2025 <a href="https://www.linkedin.com/in/minhduong-thechosenone/">Dương Duy Nhật Minh</a>, kinh doanh dưới tên <b>Hàng không Vũ trụ Oriviet</b>.</p>
    </br>
    <p>Hãy tham gia những cộng đồng Astrocelerate!</p>
    <a href="https://www.reddit.com/r/astrocelerate" target="_blank"><img alt="Reddit" src="https://img.shields.io/badge/Reddit-grey?style=flat&logo=reddit&logoColor=%23FF4500&logoSize=auto"></a>
    <a href="https://discord.gg/zZNNhW65DC" target="_blank"><img alt="Discord" src="https://img.shields.io/badge/Discord-grey?style=flat&logo=discord&logoColor=%235865F2&logoSize=auto"></a>
</div>



# Mục lục
* [Giới thiệu](#giới-thiệu)
    * [Mục tiêu](#mục-tiêu)
    * [Tầm nhìn](#tầm-nhìn)
        * [Tầm nhìn ngắn hạn (MVP)](#tầm-nhìn-ngắn-hạn-mvp)
    * [Triết lý thiết kế](#triết-lý-thiết-kế)
* [Tính năng](#tính-năng)
    * [Đã triển khai](#đã-triển-khai)
    * [Sắp có](#sắp-có)
* [Cài đặt](#cài-đặt)
    * [Phần mềm thiết yếu](#phần-mềm-thiết-yếu)
    * [Cài đặt lần đầu](#cài-đặt-lần-đầu)
* [Lịch sử](#lịch-sử)
    * [Kho ảnh](#kho-ảnh)

---

# Giới thiệu
Astrocelerate là công cụ mô phỏng cơ học quỹ đạo và chuyến bay vũ trụ hiệu suất cao đầu tiên được phát triển tại Việt Nam, nhằm nâng cao năng lực quốc gia và tiềm lực khoa học - công nghệ trong lĩnh vực Hàng không Vũ trụ và Thiên văn học.

Dự án được phát triển bởi người Việt cho người Việt, trong công cuộc xây dựng nền móng tự chủ công nghệ của quốc gia.

## Mục tiêu
Được phát triển bằng C++ với pipeline đồ họa dựa trên Vulkan và mẫu thiết kế phân phối và thành phần (Entity-Component-System), Astrocelerate được thiết kế để cung cấp hình ảnh trực quan theo thời gian thực, chính xác về mặt vật lý cho động học vệ tinh, quỹ đạo phóng và mô phỏng cơ động.

Nhưng vượt xa hiệu suất kỹ thuật, Astrocelerate đại diện cho một điều vĩ đại hơn nhiều:

>Sự khẳng định rằng các công cụ hàng không vũ trụ đẳng cấp thế giới có thể bắt nguồn từ Việt Nam và trong bàn tay của người Việt, và được xây dựng không phải bởi các nhà thầu ngoại nước, mà bởi một thế hệ kỹ sư mới của Việt Nam.

Nhiệm vụ của Astrocelerate là trao quyền cho các tổ chức học thuật, nhà quy hoạch ứng phó thảm họa, nhà công nghệ không gian và nhà nghiên cứu quốc phòng một nền tảng mô phỏng minh bạch, có khả năng mở rộng và tự chủ. Hơn thế nữa, nó đại diện cho chủ quyền công nghệ trong một lĩnh vực từ lâu đã bị chi phối bởi các hệ thống bên ngoài.

Phát triển bởi Hàng không Vũ trụ Oriviet. Nguồn gốc tại Việt Nam. Định mệnh tại các vì sao.

## Tầm nhìn
### Tầm nhìn ngắn hạn (MVP)
Tầm nhìn ngắn hạn của Astrocelerate là tạo ra một Sản phẩm khả dụng tối thiểu (MVP) đáp ứng các tính năng cốt lõi. Cụ thể, MVP phải đạt được các yêu cầu tối thiểu sau:
- Hình ảnh trực quan chính xác, thời gian thực và tương tác, minh họa các kịch bản đơn giản (ví dụ: quỹ đạo vệ tinh-Trái đất, mô phỏng cơ động cơ bản, chuỗi "phóng, vào quỹ đạo, tách tầng, tái nhập khí quyển, thu hồi").
- Một giao diện người dùng rõ ràng, trau chuốt.
- Thiết kế dựa trên mẫu thiết kế ECS.

## Triết lý thiết kế
Triết lý thiết kế của Astrocelerate lấy cảm hứng từ công cụ trò chơi Unity và công cụ mô phỏng chuyến bay vũ trụ GMAT (General Mission Analysis Tool) của NASA.
Các nguyên tắc này có thể thay đổi theo thời gian, khi Astrocelerate (và tôi) trở nên trưởng thành hơn.

| Nguyên tắc thiết kế cốt lõi                          | Mô tả                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
|------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Mã nguồn mở                                          | Astrocelerate hướng tới sự minh bạch và phát triển mạnh mẽ nhờ phản hồi, chỉnh sửa và mở rộng từ cộng đồng. Do đó, để tối đa hóa sự đóng góp và phát triển, đã quyết định rằng công cụ này nên là mã nguồn mở.                                                                                                                                                                                                                                                                                 |
| Kiến trúc dựa trên ECS, mô-đun & có khả năng mở rộng | Astrocelerate được cấu trúc để cho phép tính linh hoạt và mô-đun hóa, cho phép người dùng thiết lập và phát triển môi trường, đối tượng, trọng lực, v.v. của riêng họ với các "mô-đun" tích hợp, của bên thứ ba và tùy chỉnh. Đây là một phần trong việc tích hợp các nguyên tắc Hệ thống thực thể-thành phần của Astrocelerate.    Astrocelerate được xây dựng để cho phép các plug-in và tiện ích mở rộng của bên thứ ba, cho phép các nhà nghiên cứu tích hợp các mô hình hoặc bộ giải mới. |
| Ngôn ngữ kịch bản không độc quyền                    | Astrocelerate cho phép người dùng lập trình các mô-đun riêng của họ và ghi đè các mô-đun tích hợp sẵn bằng C++.                                                                                                                                                                                                                                                                                                                                                                                |
| Hỗ trợ các nhiệm vụ thực tế                          | Astrocelerate có thể được sử dụng để lập kế hoạch nhiệm vụ vận hành (ví dụ: chuyển giao Mặt trăng và liên hành tinh, quy trình phóng và thu hồi tàu vũ trụ).                                                                                                                                                                                                                                                                                                                                   |


# Tính năng
## Đã triển khai
- Lan truyền quỹ đạo thời gian thực
- Mô phỏng 2 vật thể chính xác
- Bước thời gian vật lý có thể cấu hình
- Hệ thống khung tham chiếu với tỷ lệ mô phỏng cố định (duy trì độ chính xác vật lý trên các tỷ lệ phân cực)
- Kiến trúc dựa trên ECS tùy chỉnh với bộ nhớ tập hợp thưa thớt (cho phép mô phỏng hiệu suất cao và luồng dữ liệu hiệu quả giữa GUI và phần phụ trợ)
- Dữ liệu đo xa trực tiếp, bảng điều khiển, v.v.; cài đặt mô phỏng có thể điều chỉnh
- Kết xuất ngoài màn hình (cho phép chế độ xem tùy chỉnh, xử lý hậu kỳ, v.v.)
- Khả năng mô phỏng N-vật thể
- Bộ nạp mô hình nâng cao hơn, với các texture được ánh xạ tốt hơn, chính xác hơn và độ chân thực hình ảnh cao hơn
- Bộ giải (solvers), hệ tọa độ (coordinate systems), kỷ nguyên (epochs)

## Sắp có
- Bộ tích hợp số có thể hoán đổi (Symplectic Euler, RK4)
- Bộ đệm dữ liệu trong ECS để tăng hiệu suất hơn nữa
- Compute shaders và việc chuyển các quy trình song song sang GPU
- Chia tỷ lệ động để chuyển đổi liền mạch (ví dụ: từ địa hình sang hành tinh)
- Một loạt các phương pháp tích hợp số đa dạng hơn (Verlet, Quy tắc Simpson, tích phân Gauss)
- Kịch bản C++ để cho phép người dùng tạo mô phỏng
- Tuần tự hóa GUI và dữ liệu mô phỏng, với khả năng xuất cơ bản


# Cài đặt
> [!WARNING]
> Astrocelerate mới chỉ được thử nghiệm trên Windows, mặc dù công cụ này hướng tới khả năng tương thích đa nền tảng.


## Phần mềm thiết yếu
- Vulkan SDK (Vulkan 1.2+)
- CSPICE Toolkit N0067
- Vcpkg dependency manager
- CMake 3.30+
- Python 3.9+
- C++20

## Cài đặt lần đầu
- Mở `CMakePresets.json` và cài đặt các biến môi trường cho cả cấu hình Debug và Release (VD: `SPICE_ROOT_DIR`).
- Chạy `SetupDebug.*` để thiết lập cấu hình Debug hoặc `SetupRelease.*` để thiết lập cấu hình Release, tùy theo hệ điều hành của bạn.
- Ngoài ra, bạn có thể chạy thủ công `GenerateDirectories.bat` để đảm bảo danh sách file mã nguồn được cập nhật, sau đó chạy `scripts/Bootstrap.py` và làm theo hướng dẫn trên màn hình.


# Lịch sử
Astrocelerate thực hiện commit đầu tiên vào ngày 28 tháng 11 năm 2024. Tính đến ngày 18 tháng 10 năm 2025, công cụ đã được phát triển trong 11 tháng:
- Phiên bản cũ (OpenGL): 2 tháng
- Phiên bản mới (Vulkan): 9 tháng


## Kho ảnh
Các ảnh chụp màn hình sau đây ghi lại quá trình phát triển của Astrocelerate (Format ngày: ngày/tháng/năm).

### 18/10/2025 (sau 2 tháng ngừng phát triển)
<img width="1919" height="1030" alt="2025-10-18" src="https://github.com/user-attachments/assets/723a596f-fc3f-4507-9b71-cc26dbe89690" />

### 16/08/2025
<img width="1919" height="1031" alt="2025-08-16" src="https://github.com/user-attachments/assets/d0979618-2afa-4734-81da-76b5b4051492" />

### 10/07/2025
![2025-07-10](https://github.com/user-attachments/assets/f86a19d1-0204-4aa8-9122-41ffcf0d268c)

### 03/07/2025
![2025-07-03](https://github.com/user-attachments/assets/60805ea1-2385-41f6-97dd-79cd571188f0)

### 21/06 - 22/06/2025
![2025-06-22](https://github.com/user-attachments/assets/2c5361d0-848e-479e-b3eb-a0c578d079ee)
![2025-06-21](https://github.com/user-attachments/assets/7c4e388d-0296-4222-aa37-aa3e346985ca)

### 08/06/2025
![2025-06-08](https://github.com/user-attachments/assets/8b4ba3e0-a3b3-4434-83dc-818e1e4f1828)

### 03/06/2025
![2025-06-03](https://github.com/user-attachments/assets/7757061d-c7f4-4e8a-9faf-d0fd595bcec1)
![2025-06-03](https://github.com/user-attachments/assets/b9d7e9eb-0035-405f-bc4d-deff89e718b8)

### 20/05 - 21/05/2025
![2025-05-21](https://github.com/user-attachments/assets/190a28e6-0b11-4355-bdf6-54978e707660)
![2025-05-20](https://github.com/user-attachments/assets/23634f9f-067f-4235-a932-015feccdfa0b)

### 16/05/2025
![2025-05-16](https://github.com/user-attachments/assets/f5c2d98c-7b1b-4b93-a833-b132181dd948)

### 14/05/2025
![2025-05-14](https://github.com/user-attachments/assets/22cfff2a-6a79-4b6f-a889-0931ed3235ec)

### 04/05/2025
![2025-05-04](https://github.com/user-attachments/assets/ef1fa152-f5c0-4e16-a6a5-40a53180fe14)

### 01/04/2025
![2025-04-01](https://github.com/user-attachments/assets/f7368b9e-8606-4182-a882-192753768ba5)

### 17/03/2025
![2025-03-17](https://github.com/user-attachments/assets/19f4eb20-e56b-4275-b877-eb5a6e027f51)

### 09/12/2024 (Astrocelerate phiên bản cũ)
![2024-12-09](https://github.com/user-attachments/assets/419ba7f0-8b80-4663-bb30-a6815ffc8af4)
