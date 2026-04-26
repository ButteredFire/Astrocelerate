> [!NOTE]
> This README is in Vietnamese. The English README can be read [here](README.md).

<div align="center" class="grid cards" style="display: flex; align-items: center; justify-content: center; text-decoration: none;" markdown>
    <img src="assets/App/AstrocelerateLogo.png" alt="Logo" width="80%">
    <p>Logo © 2024-2025 <a href="https://www.linkedin.com/in/minhduong-thechosenone/">Minh Dương</a>, kinh doanh dưới tên <b>Oriviet Aerospace</b>.</p>
    </br>
    <p>Hãy tham gia những cộng đồng Astrocelerate!</p>
    <a href="https://www.reddit.com/r/astrocelerate" target="_blank"><img alt="Reddit" src="https://img.shields.io/badge/Reddit-grey?style=flat&logo=reddit&logoColor=%23FF4500&logoSize=auto"></a>
    <a href="https://discord.gg/zZNNhW65DC" target="_blank"><img alt="Discord" src="https://img.shields.io/badge/Discord-grey?style=flat&logo=discord&logoColor=%235865F2&logoSize=auto"></a>
</div>



# Mục lục
* [Mục tiêu & Tầm nhìn](#mục-tiêu--tầm-nhìn)
* [Tính năng](#tính-năng)
    * [Đã triển khai](#đã-triển-khai)
    * [Sắp có](#sắp-có)
* [Cài đặt](#cài-đặt)
    * [Yêu cầu hệ thống](#yêu-cầu-hệ-thống)
    * [Cài đặt lần đầu](#cài-đặt-lần-đầu)
* [Lịch sử](#lịch-sử)
    * [Kho ảnh](#kho-ảnh)

---

Astrocelerate là một engine mô phỏng cơ học quỹ đạo và du hành không gian hiệu suất cao. Dự án được thiết kế như một công cụ linh hoạt và trực quan, giúp xóa bỏ rào cản giữa các phần mềm mô phỏng vũ trụ chuyên dụng "khô khan" và sự dễ tiếp cận của các engine hiện đại.

# Mục tiêu & Tầm nhìn
Hầu hết các phần mềm mô phỏng vũ trụ hiện nay đã được đơn giản hóa theo hướng trò chơi (ví dụ: Kerbal Space Program) hoặc là các công cụ cũ kỹ dựa trên mã nguồn Fortran/C từ nhiều thập kỷ trước (ví dụ: GMAT/STK). Astrocelerate hướng tới trở thành một công cụ:

- Chính xác về mặt khoa học: Sử dụng SPICE kernels của NASA để đạt độ chính xác cao về lịch thiên văn (ephemeris) và hệ tọa độ;
- Dễ tiếp cận: Cho phép những người đam mê và những người không chuyên về lập trình có thể thiết kế các mission/mô phỏng phức tạp thông qua giao diện UI hiện đại và hệ thống Lập trình Trực quan (Visual Scripting);
- Đa năng: Các công cụ mô phỏng đáp ứng hầu hết các nhu cầu thực tế;
- Hiệu suất cao: Được xây dựng cho các mô phỏng quy mô lớn (mạng lưới vệ tinh, vành đai tiểu hành tinh) bằng cách sử dụng multithreaded C++ và GPU acceleration;
- Mã nguồn mở: Đóng góp vào hệ sinh thái nguồn mở và sự phát triển chung của cộng đồng hàng không vũ trụ với mã nguồn công khai, documentation chi tiết và hỗ trợ plugin.

Astrocelerate được phát triển với niềm tin rằng "chân trời lớn nhất của nhân loại" nên được mở rộng, hiện đại hóa và không phụ thuộc vào các hệ sinh thái độc quyền chi phí cao. Đây là công cụ dành cho sinh viên, nhà nghiên cứu và kỹ sư để xây dựng tương lai của thiên văn học.

*Astrocelerate là sáng kiến và dự án đầu tiên của [Oriviet Aerospace](https://www.oriviet.org), một startup giai đoạn đầu tại Việt Nam với mục tiêu xây dựng các công cụ mô phỏng vũ trụ thế hệ mới cho các kỹ sư, nhà nghiên cứu và nhà thiết kế.*


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
- Lập trình Trực quan (Visual Scripting) để cho phép người dùng tạo mô phỏng

## Sắp có
- Bộ tích hợp số có thể hoán đổi (Symplectic Euler, RK4)
- Bộ đệm dữ liệu trong ECS để tăng hiệu suất hơn nữa
- Compute shaders và việc chuyển các quy trình song song sang GPU
- Chia tỷ lệ động để chuyển đổi liền mạch (ví dụ: từ địa hình sang hành tinh)
- Một loạt các phương pháp tích hợp số đa dạng hơn (Verlet, Quy tắc Simpson, tích phân Gauss)
- Tuần tự hóa GUI và dữ liệu mô phỏng, với khả năng xuất cơ bản


# Cài đặt
> [!WARNING]
> Astrocelerate mới chỉ được thử nghiệm trên Windows, mặc dù công cụ này hướng tới khả năng tương thích đa nền tảng.


## Yêu cầu hệ thống
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
Astrocelerate có bản commit đầu tiên vào ngày 28 tháng 11 năm 2024. Tính đến ngày 28 tháng 12 năm 2025, dự án đã được phát triển trong 395 ngày. Trong hai tháng đầu tiên, Astrocelerate được viết bằng OpenGL, nhưng sau đó đã chuyển sang Vulkan do những hạn chế của mô hình lập trình máy trạng thái (state-machine) trong OpenGL.


## Kho ảnh
Các ảnh chụp màn hình sau đây ghi lại quá trình phát triển của Astrocelerate (Format ngày: YYYY-MM-DD).

### 2026-04-12
<img width="1919" height="1031" alt="2026-04-12" src="https://github.com/user-attachments/assets/03028d1b-e055-4e6d-b202-7e734e6bdf75" />

### 2025-12-17
<img width="1919" height="1031" alt="2025-12-17" src="https://github.com/user-attachments/assets/82a93f52-c7ea-45d6-a5eb-f03534892a36" />

### 2025-10-20
<img width="1919" height="1029" alt="2025-10-20" src="https://github.com/user-attachments/assets/6a0309a0-6af9-4c2a-a9a3-4774f811d2b6" />

### 2025-10-18 (sau 2 tháng ngừng phát triển)
<img width="1919" height="1030" alt="2025-10-18" src="https://github.com/user-attachments/assets/723a596f-fc3f-4507-9b71-cc26dbe89690" />

### 2025-08-16
<img width="1919" height="1031" alt="2025-08-16" src="https://github.com/user-attachments/assets/d0979618-2afa-4734-81da-76b5b4051492" />

### 2025-07-25
<img width="1919" height="1031" alt="2025-07-25" src="https://github.com/user-attachments/assets/fb586a14-5ddc-4442-a18a-12a35fca709e" />
<img width="1919" height="1032" alt="2025-07-25" src="https://github.com/user-attachments/assets/958aa972-6ba2-4740-b331-321d3da697ed" />

### 2025-07-10
![2025-07-10](https://github.com/user-attachments/assets/fb0ea6af-8cf1-43de-9279-71a61d7c1744)

### 2025-07-03
![2025-07-03](https://github.com/user-attachments/assets/be22da3f-f431-42d8-ab0a-7d358e144a9e)

### 2025-06-22
![2025-06-22](https://github.com/user-attachments/assets/c27da2fd-ae07-403f-96c3-36c6fb1297b7)
![2025-06-22](https://github.com/user-attachments/assets/0b8aa252-3068-4da2-b433-3886e4e54c60)

### 2025-06-08
![2025-06-08](https://github.com/user-attachments/assets/645da5cf-9c5b-44c2-aa0b-dfc4be91b824)

### 2025-06-03
![2025-06-03](https://github.com/user-attachments/assets/11605157-d576-4e81-a4e1-1356a1696cd4)
![2025-06-03](https://github.com/user-attachments/assets/dd6024d3-3849-4048-b391-93b7ef03b0ae)

### 2025-05-21
![2025-05-21](https://github.com/user-attachments/assets/8a92729e-3945-47cd-aa3a-69c8a8d73f0d)
![2025-05-20](https://github.com/user-attachments/assets/2a31b5e5-0b75-41ee-8aea-68cc65536e8d)

### 2025-05-16
![2025-05-16](https://github.com/user-attachments/assets/bc5a983d-f2a5-40c2-8812-15edb9bc5ac2)

### 2025-05-14
![2025-05-14](https://github.com/user-attachments/assets/5d99422e-e6bf-4539-84af-b6a8eeedac7f)

### 2025-05-04
![2025-05-04](https://github.com/user-attachments/assets/5e75e451-e4f2-4f9e-825f-2cb2edf9af55)

### 2025-04-01
![2025-04-01](https://github.com/user-attachments/assets/4df183fd-ec53-4ec0-a751-0016e6e23de3)

### 2025-03-17
![2025-03-17](https://github.com/user-attachments/assets/1fca2070-ff43-4595-bd6a-74c8c43ae982)

### 2024-12-09 (Astrocelerate phiên bản cũ)
![2024-12-09](https://github.com/user-attachments/assets/db1f0232-cab3-4022-95f8-75ab503b029c)
