# Modbus 发送被抢占导致断帧 —— 问题溯源与整改总结

> 适用工程：`adc_comm`（网关）、`motor_ctrl`、`mfc_ctrl`、`PZT`
> 编写日期：2026-06-16
> 关键词：Modbus RTU、FreeRTOS、HAL_UART_Transmit、ReceiveToIdle、IDLE 判帧、字节间空隙

---

## 一、问题概述

系统采用 `adc_comm` 作为 Modbus 网关，通过多路 UART 与 `motor_ctrl`、`mfc_ctrl`、`PZT` 三块从机通讯。调试中发现：用 03 功能码经网关读取从机寄存器时，**接收方偶发收到长度异常的"半帧"**，导致 CRC / 长度校验失败、通讯间歇性报错。

根因并非 `HAL_UARTEx_ReceiveToIdle_DMA` 本身有缺陷，而是**发送端在 FreeRTOS 任务上下文使用阻塞发送，被抢占时在 UART TX 线上产生字节间空隙，触发了接收端的 UART 空闲（IDLE）判帧，把一帧拆成多段。**

---

## 二、问题溯源（以 motor_ctrl 从机为例详细描述）

### 2.1 现象

- 网关用 FC03 读 motor_ctrl 的 **21 个寄存器**。
- 期望响应整帧 = `从机地址(1) + 功能码(1) + 字节数(1) + 数据(42) + CRC(2)` = **47 字节**，故网关侧 `rxFrameLen` 应为 47。
- 实测 `rxFrameLen` 漂移为 **33 / 38** 等不固定的短值 → CRC / 字节数校验不过 → 协议报错。
- 漂移、间歇性出现，是典型的"帧在不同位置被切断"特征。

> 提示：21 寄存器响应是 47 字节而非 45 —— 易漏算 2 字节 CRC（CRC 也是通过 UART 接收、计入 `rxFrameLen` 的）。

### 2.2 原因

从机 `modbus_slave.c` 的 `SendResponse()` 原本使用**阻塞轮询发送**：

```c
// 旧代码（有问题）
HAL_UART_Transmit(ctx->huart, ctx->txBuffer, frameLen, 100);
```

该函数在 `MB_Slave_Task`（FreeRTOS 任务，`osPriorityNormal`）上下文中被调用。而 motor_ctrl 跑的是**抢占式**调度。

### 2.3 机制（关键）

大多数 STM32F4 普通 UART **没有 TX FIFO**，发送数据寄存器 TDR 只有 1 字节深。阻塞发送的本质是循环"写 1 字节 → 轮询等 TXE 空 → 写下一字节"。在 RTOS 下，这个循环随时可能被：

- 更高优先级任务（被某中断唤醒后）抢占，或
- 一个耗时较长的 ISR / SysTick，或
- 同级任务的时间片轮转

打断。**一旦两次写 TDR 之间被打断超过 1 个字符时间**（115200、8N1 下约 87µs），UART 把前一字节移位发完后，TX 线就进入空闲电平，线上出现一个"空隙"。

接收端（网关）用的是 `HAL_UARTEx_ReceiveToIdle_DMA`，"线路空闲约 1 个字符时间即判定一帧结束"。于是：

```
从机TX:  [地址][FC][cnt][data... ~30字节] <—被抢占,空隙—> [剩余data][CRC]
网关RX:  IDLE 在空隙处就触发 → RxEventCallback(size=33) → 立即当成完整一帧
```

网关在第一个 IDLE 事件就把 33/38 字节当成整帧上报、唤醒任务，并丢弃随后到达的剩余数据。抢占点随机 → 短帧长度随机漂移。

> 本质上这违反了 Modbus RTU 规范——一帧内字节间隔必须 < 1.5 个字符时间。**违规方是从机发送端**；网关的 IDLE 逻辑反而是"按规范正确地"提前结束了帧。

### 2.4 后果

- 接收端收到残缺帧 → CRC / 长度校验失败。
- 对**响应路径**（从机→网关）：网关读操作间歇性失败、需重试。
- 对**请求路径**（网关→从机）：从机静默丢弃残缺请求、不回包 → 主机**间歇性超时**（症状不同，但同源）。
- 帧越长（如 FC10 多寄存器写、多寄存器读响应），暴露窗口越大，命中概率越高。
- 表现为"偶发、不可稳定复现"的通讯故障，排查成本高。

---

## 三、解决措施（以 motor_ctrl 从机为例）

核心思路：**让 UART 由硬件（DMA 或发送中断）连续喂字节，不再依赖任务调度，从而消除帧内空隙。** 本项目从机统一采用 **DMA 发送 + 发送完成同步**。

### 3.1 CubeMX 侧

为对应 UART 增加一路 **TX DMA**（Normal 模式、Byte/Byte、Memory 自增、FIFO Disabled），并确认对应 DMA 流的全局中断已使能（`HAL_UART_Transmit_DMA` 依赖 DMA 传输完成中断触发 `HAL_UART_TxCpltCallback`）。

### 3.2 代码侧（三处）

**(1) `SendResponse()`：阻塞发送 → DMA 发送 + 等待完成**

```c
osThreadFlagsClear(MB_SLAVE_TXCPLT_FLAG);                 // 清陈旧标志
if (HAL_UART_Transmit_DMA(ctx->huart, ctx->txBuffer, frameLen) != HAL_OK) {
    return;
}
// 等真正发完（含最后一位移出），避免 txBuffer 在传输中被复用
uint32_t flags = osThreadFlagsWait(MB_SLAVE_TXCPLT_FLAG, osFlagsWaitAny, MB_SLAVE_TX_TIMEOUT_MS);
if (flags & osFlagsError) {
    HAL_UART_AbortTransmit(ctx->huart);                  // 超时兜底
}
```

**(2) 新增发送完成处理函数（按 UART 实例过滤）**

```c
void MB_SLAVE_UART_HandleTxCplt(UART_HandleTypeDef *huart)
{
    if (huart->Instance == mbSlave.huart->Instance && mbSlave.taskHandle != NULL) {
        osThreadFlagsSet(mbSlave.taskHandle, MB_SLAVE_TXCPLT_FLAG);
    }
}
```

**(3) `bsp_uart.c` 注册 HAL 回调**

```c
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    MB_SLAVE_UART_HandleTxCplt(huart);
}
```

### 3.3 完成中断链

```
DMA 传输完成 → DMA 流 IRQ → HAL 置 UART TC 中断
            → USARTx IRQ → HAL_UART_TxCpltCallback → 置线程标志 → 发送任务唤醒返回
```

DMA 保证整帧在线上连续无空隙，使接收端"单次 IDLE = 一帧"的假设重新成立。

### 3.4 效果

- 网关 `rxFrameLen` 恢复为正确的 47，FC03/06/10 不再 CRC / 长度报错。
- 抓 USART3 TX 波形确认整帧连续无空隙。

---

## 四、对今后开发工作的启示

1. **RTOS 下严禁在任务上下文做"长时间阻塞 + 逐字节"的外设输出。** 凡是对时序连续性有要求的发送（Modbus、其它按静默间隔/IDLE 判帧的协议），一律用 **DMA 或中断（IT）** 驱动，由硬件保证连续性。

2. **IT 与 DMA 的选型**：
   - 短帧（≤ ~10 字节）两者实际等效，**IT 免 CubeMX 改动**更省事。
   - 长帧或重 ISR 负载场景优先 **DMA**：抗空隙更彻底（不依赖逐字节中断延迟）、CPU 开销 O(1)。
   - 两者 `HAL_UART_TxCpltCallback` 都在 UART TC（最后一位移出）后触发，**完成同步代码通用**，IT↔DMA 互换仅改发送那一行 + CubeMX。

3. **异步发送必须做"发送完成同步"**：等到 TX 真正完成再复用发送缓冲区 / 切换收发方向（半双工）。利用 `HAL_UART_TxCpltCallback` + 线程标志/信号量，并加超时兜底。

4. **半双工 RS485**：`HAL_UART_Transmit_DMA/_IT` 在 TC 后才回调，总线翻转时机与阻塞版一致；若为硬件自动方向收发器则无需软件控制方向。

5. **接收端 IDLE 判帧是把双刃剑**：它依赖发送端遵守帧内连续性。排查"半帧/断帧"时，应同时怀疑**发送端时序**，而不仅是接收端配置。

6. **协议层面建议**：主站对超时/CRC 失败应有**重试**机制，作为偶发链路异常的兜底。

7. **CubeMX 重新生成的副作用（关联教训）**：本次为加 TX DMA 多次重新生成代码；MX 升级曾把手写在生成文件里的 `TIM7` 软件 PWM 配置（位于 `USER CODE` 区）整段清除，导致链接失败。**应对：**
   - 手写外设配置尽量集中放在最稳的 `USER CODE` 区（实测 `tim.c` 的 `USER CODE 1` 区在升级中存活，而 `GV`/`MspInit` 等区被清空）；
   - 每次 `GENERATE CODE` 前先 `git commit` 留干净基线，生成后 `git diff` 复查有无误删。

---

## 五、全工程整改简略总结（便于后续核对）

### 5.1 从机（自研 modbus_slave，发送响应帧）

| 工程 | 从机 UART | 通讯对象 | 整改方式 | TX DMA 流 |
|---|---|---|---|---|
| motor_ctrl | USART3 | 网关 adc_comm | 阻塞 → **DMA 发送** + 完成同步 | DMA1 Stream3 / Ch4 |
| mfc_ctrl | USART1 | 网关 adc_comm | 阻塞 → **DMA 发送** + 完成同步 | DMA2 Stream7 / Ch4 |
| adc_comm | USART6 | 上位机 PC/HMI | 阻塞 → **DMA 发送** + 完成同步 | DMA2 Stream6 / Ch5 |
| PZT | —（FreeMODBUS） | 网关 adc_comm | **无需整改**：本就用 `HAL_UART_Transmit_IT`（中断发送），天然连续 | — |

### 5.2 主机（自研 modbus_master，发送请求帧）

| 工程 | 主机 UART | 通讯对象 | 整改方式 | 备注 |
|---|---|---|---|---|
| adc_comm | USART5 / USART1 / UART4 | 三块从机 MCU | 阻塞 → **IT 发送** + 完成同步 | 多为短读，IT 免 CubeMX |
| motor_ctrl | UART4 | 三个伺服电机（RS485） | 阻塞 → **DMA 发送** + 完成同步 | 用 FC10 长帧，DMA 抗空隙更优；DMA1 Stream4 / Ch4；硬件自动方向 |
| mfc_ctrl | — | — | 无自己的 master | 仅作从机 |
| PZT | — | — | 无自己的 master | 仅作从机 |

### 5.3 公共代码骨架（主从一致）

- 发送：`HAL_UART_Transmit_DMA` 或 `HAL_UART_Transmit_IT`（非阻塞）。
- 同步：发送前 `osThreadFlagsClear`，发起后 `osThreadFlagsWait(TXCPLT, 超时)`，由 `HAL_UART_TxCpltCallback` → `MB_(SLAVE_)UART_HandleTxCplt`（按 UART 实例过滤）置标志；超时 `HAL_UART_AbortTransmit(_IT)` 兜底。
- 主机两个标志位分离：`RESP(0x01)` 收到响应、`TXCPLT(0x02)` 请求发完；流程为"发→等发完→等响应"。
- `bsp_uart.c` 的 `HAL_UART_TxCpltCallback` 同时分发给 master 与 slave 处理函数（各按实例过滤，互不干扰）。

### 5.4 相关提交（git 历史，便于追溯）

| commit | 说明 |
|---|---|
| `8aa4003` | motor_ctrl 增加 USART3_TX DMA（CubeMX regen） |
| `0fcf20b` | mfc_ctrl / adc_comm 从机增加 UART_TX DMA（CubeMX regen） |
| `71afff2` | motor_ctrl 增加 UART4_TX DMA（CubeMX regen，主机用） |
| `65337f8` | 恢复被 MX 重新生成清除的 TIM7 软件 PWM 配置 |
| `d64220a` | 修复从机断帧：DMA 发送替代阻塞发送（motor_ctrl/mfc_ctrl/adc_comm 从机） |
| `748c268` | adc_comm 主机改用 IT 发送 |
| （待提交） | motor_ctrl 主机改用 DMA 发送 + 完成同步 |

> 注：PZT 工程使用 FreeMODBUS 协议栈，收发已是中断驱动，未在本次整改范围内改动。
