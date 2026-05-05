# ASSIT-NAT — ESP8266 网络工具箱

基于 NodeMCU 的便携式网络安全工具，集成了 WiFi 侦察、射频分析、无线攻击、加密工具、硬件诊断和 433MHz 射频收发等功能。一块板子，63 个模块，128×64 OLED 显示，三个按键操作。

> 这玩意是我一边学一边写出来的，代码风格比较随意，但功能都是实测能用的。有啥问题提 issue 就行。

## 硬件清单

| 元件 | 型号 | 用途 |
|------|------|------|
| 主控 | NodeMCU v2 (ESP8266, 4MB Flash) | 大脑 |
| 屏幕 | SSD1306 128×64 OLED I2C | 显示 |
| 按键 ×3 | 轻触开关 + 内部上拉 | UP / DOWN / OK |
| 433 接收 | SYN480R (晶振 13.560MHz) | 接收 433MHz 信号 |
| 433 发射 | F115 433M (晶振 6.7458MHz) | 发射 433MHz 信号 |

总成本大概二三十块钱，不算 USB 线。

## 接线

```
OLED:     VCC→3V3   GND→GND   SDA→D2(GPIO4)   SCL→D1(GPIO5)
按键:     BTN_UP→D3(GPIO0)   BTN_DOWN→D4(GPIO2)   BTN_OK→D5(GPIO14)
           每个按键一脚接 GPIO，另一脚接 GND（用内部上拉）
433 RX:   VCC→3V3   GND→GND   DATA→D6(GPIO12)   ANT→17.3cm 导线
433 TX:   VCC→3V3   GND→GND   DATA→D7(GPIO13)   ANT→17.3cm 导线
PWM 输出: D8(GPIO15)
ADC 输入: A0 (量程 0~1V，测更高电压需要分压)
```

**怎么区分 433 模块：** 看板子上的晶振。13.560MHz + 8 脚芯片的是 SYN480R 接收模块；6.7458MHz + 三极管的是 F115 发射模块。

天线直接用 17.3cm 的杜邦线焊到 ANT 焊盘就行（433MHz ¼波长）。

详细的接线图和引脚说明见 `接线图.txt`。

## 烧录

```bash
# 安装 PlatformIO
pip install platformio

# 编译
pio run

# 烧录（用 MicroUSB 线接上 NodeMCU）
pio run --target upload
```

需要手动进下载模式的话：按住 OK → 点一下 RST → 松开 OK。

## 按键操作

| 操作 | 功能 |
|------|------|
| 短按 UP/DOWN | 移动光标 / 调整数值 |
| 短按 OK | 确认 / 进入 / 执行 |
| 长按 OK (600ms) | 返回上一级 |
| 双击 OK | 返回（部分模块） |
| 双击 UP/DOWN | 跳到列表头/尾 |
| 攻击运行时按任意键 | 紧急停止 |

## 功能模块

### 网络侦察 (17 个)
WiFi 扫描、信道热力图、探针嗅探、主机发现、网关监控、端口扫描、路由追踪、网络统计、DHCP 发现、mDNS 枚举、UPnP 发现、DNS 查询、HTTP 客户端、Ping 监控、NTP 查询、网络唤醒、SSL 检查

### 射频分析 (9 个)
信标解码、帧计数器、RSSI 距离估算、信道质量、信号直方图、频谱视图、信道跳频、信号监控、射频噪声

### WiFi 攻击 (1 个统一模块)
集成扫描目标 → 选择目标 → 选择攻击模式 → 配置参数 → 执行的完整流程。

支持 8 种攻击：解除认证、信标洪水、PMKID 捕获、握手包捕获、伪基站、Karma 攻击、钓鱼热点、设备测试

> ⚠️ 仅用于授权测试，别拿别人的网络瞎搞。

### 加密工具 (8 个)
TOTP 动态口令、密码生成器、MAC 厂商查询、Base64 编解码、十六进制转换、哈希计算、异或加密、WPS Pin 计算

### 硬件诊断 (8 个)
逻辑探针、PWM 发生器、ADC 电压表、I2C 扫描、GPIO 控制、串口监视、SPI 扫描、频率发生器

### 网络诊断 (10 个)
网关发现、多网关检测、信道切换、RSSI 漂移、广播风暴监测、服务发现、WiFi 日志、网络快照、网速测试、子网计算器

### 433MHz 射频 (1 个统一模块)
扫描捕获 → 浏览已捕获码 → 发射/重放 → 原始位编辑 → 设备码库 → 温度传感器解码

### 系统工具 (9 个)
系统信息、文件管理、屏幕测试、按键测试、电池电压、休眠定时、自动配置、LED 测试、设置

完整模块说明见 `模块使用说明书.txt`。

## 中文字体

OLED 显示中文用的是逐字 XBM 位图方案（不是 U8g2 字体文件）。每个汉字渲染成 13×12 左右的小位图，存在 PROGMEM 里，drawCN() 逐字绘制。

`tools/gen_cn_glyphs.py` 是字形生成脚本，依赖 Pillow 和 Windows 下的 SimSun 字体。如果要改菜单文字或换字体，改脚本里的 `MENU_STRINGS` 然后重新跑一遍就行。

## 项目结构

```
src/
  main.cpp              # 入口
  config.h              # 全局配置
  chinese_glyphs.*      # 中文字形数据（生成）
  icons.h               # 图标位图
  utils.*               # 工具函数
  core/                 # 核心系统（菜单引擎、按键管理、显示、WiFi、事件总线）
  modules/              # 63 个功能模块
tools/
  gen_cn_glyphs.py      # 中文字形生成器
_old_modules/           # 已被合并的旧模块（wifi_attack / rf433_tool 统一后保留作参考）
platformio.ini          # PlatformIO 配置
接线图.txt               # 详细接线说明
模块使用说明书.txt         # 完整模块操作指南
```

## 编译环境

- PlatformIO + Arduino framework
- 依赖库：U8g2 (OLED)、rc-switch (433MHz)、ESP8266Ping
- ESP8266 160MHz, 4MB Flash, LittleFS
- 当前占用：RAM ~57%, Flash ~54%

## 说明

接线图和模块使用说明书由 AI 辅助整理生成。项目里的代码除 U8g2、rc-switch、ESP8266Ping 等第三方库外，其他都是自己写的。
