# New PC

依赖：基本上只有 verilator

使用方法：

```bash
make sim # 构建二进制文件，运行，使用默认参数

# 具体通过二进制和参数运行，方法需要阅读 monitor.C 和 sdb.C
# 进入 REPL 后
help 查看支持的命令
```

---

## 最小 CPU DPI-C 接口说明文档

本文档用于说明 `Top.sv` 中使用的 **DPI-C 接口**及其作用，帮助队友理解：

* CPU 如何与仿真器交互
* DPI-C 在仿真框架中的作用
* 每个接口的功能和实现方式

当前 CPU 是一个 **最小可仿真的演示 CPU**，只支持少量指令：

* `addi`
* `jalr`
* `ebreak`

CPU 逻辑使用 **SystemVerilog** 实现，而 **内存模型、程序加载、仿真控制**等由 **C++ 仿真器**负责。
两者通过 **DPI-C（Direct Programming Interface for C）** 连接。

---

## 整体架构

```C
+------------------------+
|        C++ 仿真器       |
|------------------------|
| 程序加载               |
| 内存模型 (pmem)       |
| 仿真退出控制           |
| 调试/trace             |
+-----------^------------+
            |
            | DPI-C
            v
+------------------------+
|        Top.sv CPU       |
|------------------------|
| PC                     |
| RegFile                |
| 指令译码               |
| addi / jalr / ebreak   |
| commit 信息            |
+------------------------+
```

CPU 不直接实现 SRAM 或总线，而是通过 **DPI-C 调用 C++ 函数访问内存**。

这样可以：

* 快速搭建一个可仿真的 CPU
* 复用已有 NPC 仿真框架
* 减少 RTL 复杂度

---

## DPI-C 接口分类

当前接口分为两类。

### import "DPI-C"

SystemVerilog 调用 C++ 函数。

用途：

* 读内存
* 写内存
* 通知仿真结束

### export "DPI-C"

C++ 调用 SystemVerilog 函数。

用途：

* 读取寄存器
* 获取指令提交信息
* 调试 CPU 状态

---

## SV 调用 C++ 的接口（import）

### sim_exit

声明：

```systemverilog
import "DPI-C" function void sim_exit(
  input int unsigned sim_event,
  input int unsigned pc
);
```

作用：

通知 C++ 仿真器 **CPU 请求结束仿真**。

通常发生在：

* 执行到 `ebreak`
* 遇到非法指令
* 程序返回码不正确

退出类型：

```systemverilog
typedef enum int {
  EXIT_GOOD     = 0,
  EXIT_BAD_CODE = 1,
  EXIT_BAD_INST = 2
} sim_event_t;
```

| 类型 | 含义 |
| ------------- | ------- |
| EXIT_GOOD | 程序正常结束 |
| EXIT_BAD_CODE | 程序返回非 0 |
| EXIT_BAD_INST | 非法指令 |

示例：

```systemverilog
sim_exit(EXIT_GOOD, pc);
```

C++ 侧典型实现：

```cpp
extern "C" void sim_exit(uint32_t event, uint32_t pc) {
    printf("Simulation exit: event=%d pc=%x\n", event, pc);
    sim_running = false;
}
```

---

### sim_mem_read

声明：

```systemverilog
import "DPI-C" pure function int unsigned sim_mem_read(
  input int unsigned addr,
  input int len
);
```

作用：

从 C++ 仿真器的内存模型读取数据。

当前 CPU 使用它 **取指令**：

```systemverilog
inst = sim_mem_read(pc, 4);
```

参数：

| 参数 | 含义 |
| ---- | ----- |
| addr | 读取地址 |
| len | 读取字节数 |

当前最小 CPU 中：

```C
len = 4
```

表示读取一条 32 位指令。

返回值：

返回读取到的 **32 位数据**。

C++ 侧典型实现：

```cpp
extern "C" uint32_t sim_mem_read(uint32_t addr, int len) {
    return pmem_read(addr);
}
```

---

### sim_mem_write

声明：

```systemverilog
import "DPI-C" function void sim_mem_write(
  input int unsigned addr,
  input int unsigned wmask,
  input int unsigned data
);
```

作用：

向仿真器内存写入数据。

未来用于实现：

* `sw`
* `sb`
* `sh`

参数：

| 参数 | 含义 |
| ----- | ---- |
| addr | 写入地址 |
| wmask | 写掩码 |
| data | 写入数据 |

写掩码示例：

| wmask | 含义 |
| ----- | ------- |
| 0001 | 写低 8 位 |
| 0011 | 写低 16 位 |
| 1111 | 写 32 位 |

C++ 侧示例：

```cpp
extern "C" void sim_mem_write(uint32_t addr, uint32_t wmask, uint32_t data) {
    pmem_write(addr, wmask, data);
}
```

---

## C++ 调用 SV 的接口（export）

这些接口用于 **调试和追踪 CPU 状态**。

### sim_regfile_read

声明：

```systemverilog
export "DPI-C" function sim_regfile_read;
```

实现：

```systemverilog
function automatic int unsigned sim_regfile_read(input int unsigned addr);
  return regfile[addr];
endfunction
```

作用：

读取 CPU 寄存器。

参数：

| 参数 | 含义 |
| ---- | ----- |
| addr | 寄存器编号 |

示例：

```cpp
uint32_t a0 = sim_regfile_read(10);
```

---

### sim_commit_valid

声明：

```systemverilog
export "DPI-C" function sim_commit_valid;
```

实现：

```systemverilog
function automatic int unsigned sim_commit_valid();
  return {31'b0, commitValid};
endfunction
```

作用：

表示当前周期是否有指令提交。

返回值：

| 返回值 | 含义 |
| --- | ----- |
| 0 | 无提交 |
| 1 | 有指令提交 |

---

### sim_commit_context

声明：

```systemverilog
export "DPI-C" function sim_commit_context;
```

实现：

```systemverilog
function automatic void sim_commit_context(
  output int unsigned pc,
  output int unsigned dnpc,
  output int unsigned inst
);
```

作用：

返回当前提交指令的信息。

输出：

| 参数 | 含义 |
| ---- | ------ |
| pc | 指令 PC |
| dnpc | 下一条 PC |
| inst | 指令编码 |

C++ 示例：

```cpp
if (sim_commit_valid()) {
    uint32_t pc, dnpc, inst;
    sim_commit_context(&pc, &dnpc, &inst);
}
```

---

## CPU 执行流程

每个周期 CPU 执行以下步骤。

### 1. 取指

```C
inst = sim_mem_read(pc, 4)
```

### 2. 译码

识别指令类型：

* addi
* jalr
* ebreak

### 3. 执行

更新寄存器或计算跳转地址。

### 4. 提交

更新：

```C
commitPc
commitInst
commitDnpc
commitValid
```

### 5. 结束（可选）

```C
sim_exit(...)
```

---

## 设计优点

该设计具有以下优点：

* CPU RTL 非常简单
* 内存模型完全复用 C++
* 易于演示 CPU 执行过程
* 易于扩展新指令

---

## 未来扩展

后续可以逐步扩展：

### 指令

* lw
* sw
* beq

### 调试能力

* commit trace
* register dump

### 硬件接口

* SRAM interface
* AXI-lite
* MMIO

---

## 总结

该最小 CPU 使用 **DPI-C** 将：

* SystemVerilog CPU
* C++ 仿真器

连接起来。

SV 负责：

* CPU 执行逻辑

C++ 负责：

* 内存
* 程序加载
* 仿真控制

这种方式可以 **快速实现一个可仿真的 CPU 原型**。
