# SeekCytometer Peripheral参数数据管理

## 需求概述

SeekCytometer Peripheral 负责交互式控制SeekCytometer 流式细胞分析设备的各类外部设备，包括进样系统，芯片位移系统，液路压力控制系统，光学系统，PZT压紧系统，温控系统等。这些外部设备控制时，需要设置不同的控制参数，且在不同的实验条件和芯片条件下要设置不同的参数。为了方便各项参数的设置和切换，在SeekCytometer Peripheral软件中需要开发以下功能：

1. 使用postgreSQL数据库管理所有外设模块的控制参数。
1. 每个功能模块使用不同的表记录参数，一张表包括多套参数。
1. 界面上可以在不同模块中方便的切换已经保存的不同参数，也提供新建和修改参数的功能。



## 表设计

- 芯片位置表

| 字段     | 类型   | 限制              |
| -------- | ------ | ----------------- |
| ID       | int    | 唯一标识          |
| 位置名称 | string | <64chars          |
| X轴位置  | int    | [-100000, +80000] |
| Y轴位置  | int    | [-50000, +700000] |
|          |        |                   |

- 镜头位置表(z)

| 字段        | 类型   | 限制              |
| ----------- | ------ | ----------------- |
| ID          | int    | 唯一标识          |
| 位置名称    | string | unique， <64chars |
| 镜头(Z)位置 | int    | [-10000, 35000]   |
|             |        |                   |



- 激光配置表

| 字段          | 类型   | 限制                |
| ------------- | ------ | ------------------- |
| ID            | int    | 唯一标识            |
| 激光配置名称  | string | unique, <64chars    |
| 638nm激光使能 | bool   | NOT NULL, DEFAULT 0 |
| 448nm激光使能 | bool   | NOT NULL, DEFAULT 0 |
| 白色LED使能   | bool   | NOT NULL, DEFAULT 0 |
| 638nm强度     | int    | [0, 100]            |
| 448nm强度     | int    | [0, 100]            |
| 白色LED强度   | int    | [0, 100]            |



- 液压控制参数表

| 字段             | 类型   | 限制               |
| ---------------- | ------ | ------------------ |
| ID               | int    | 唯一标识           |
| 控制参数名称     | string | NOT NULL, <64chars |
| 通道1kp          | real   | DEFAULT 2.0        |
| 通道1ki          | real   | DEFAULT 1.0        |
| 通道1feedforward | int    | DEFAULT 13000      |
| 通道2kp          | real   | DEFAULT 2.0        |
| 通道2ki          | real   | DEFAULT 1.0        |
| 通道2feedforward | int    | DEFAULT 13000      |
| 通道3kp          | real   | DEFAULT 2.0        |
| 通道3ki          | real   | DEFAULT 1.0        |
| 通道3feedforward | int    | DEFAULT 13000      |
| 通道4kp          | real   | DEFAULT 2.0        |
| 通道4ki          | real   | DEFAULT 1.0        |
| 通道4feedforward | int    | DEFAULT 13000      |
| 通道5kp          | real   | DEFAULT 2.0        |
| 通道5ki          | real   | DEFAULT 1.0        |
| 通道5feedforward | int    | DEFAULT 13000      |