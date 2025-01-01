# 正式编译烧录前的 SDK 配置说明
在开始编译和烧录之前，请按照以下步骤配置 `sdkconfig`:  
你可以在项目目录,使用**ESP-IDF命令行**运行
```bash
idf.py menuconfig
```
或使用VScode的 **SDK配置编辑器**


# 分区配置
进入 **Serial flasher config**:  
- 找到 **Flash size** 选项,设置为你的 Flash 大小,例如 `4MB`

   
进入 **Partiton Table** -> **Partition Table**  
- 选中 **Single factory app (large), no OTA**


# TLS配置  
进入 **Compoent config** -> **EPS-TLS** 并勾选
- **ESP-TLS Server: Set minimum Certificate Verification mode to Optional**(可能非必须)
- **Allow potentially insecure options**
- **Skip server certificate verification by default (WARNING: ONLY FOR TESTING PURPOSE, READ HELP)**
