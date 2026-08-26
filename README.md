编译环境，Windows vscode，esp-idf v5.5.4

## 配置

设备身份、WiFi 和服务器参数改为通过 menuconfig 配置，不再直接写死在源码里：

```bash
idf.py menuconfig
```

进入 `UbiBot WS1B Configuration` 菜单，按需修改：

- **WiFi Configuration**：WiFi 名称 / 密码 / 国家码
- **Server Configuration**：服务器地址 / 端口
- **Device Identity**：设备 PID / 序列号（SN，每台设备应使用各自唯一的序列号）

配置结果保存在本地的 `sdkconfig` 文件中（已在 `.gitignore` 中排除，不会被提交），首次构建或运行 `idf.py menuconfig` 后会自动生成。默认值定义在 [main/Kconfig.projbuild](main/Kconfig.projbuild)。

## 许可证

本项目基于 [MIT License](LICENSE) 开源。
