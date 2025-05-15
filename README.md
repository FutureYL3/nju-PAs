# 南京大学PA全栈计算机系统实践项目

## 系统栈

<img src="./README.assets/%E7%B3%BB%E7%BB%9F%E6%A0%88%E6%9E%B6%E6%9E%84.png" alt="系统栈架构" style="zoom:50%;" />

## 实现功能罗列

### NEMU

- 单个 CPU、128 MB 内存

- 简易调试器 `sdb(simple debugger)`

  | 命令         | 格式          | 使用举例               | 说明                                                         |
  | ------------ | ------------- | ---------------------- | ------------------------------------------------------------ |
  | 帮助(1)      | `help`        | `help`                 | 打印命令的帮助信息                                           |
  | 继续运行(1)  | `c`           | `c`                    | 继续运行被暂停的程序                                         |
  | 退出(1)      | `q`           | `q`                    | 退出NEMU                                                     |
  | 单步执行     | `si [N]`      | `si 10`                | 让程序单步执行 `N` 条指令后暂停执行,<br/>当 `N` 没有给出时, 缺省为 `1` |
  | 打印程序状态 | `info SUBCMD` | `info r`<br />`info w` | 打印寄存器状态<br/>打印监视点信息                            |
  | 扫描内存(2)  | `x N EXPR`    | `x 10 $esp`            | 求出表达式 `EXPR` 的值, 将结果作为起始内存<br/>地址, 以十六进制形式输出连续的 `N` 个 4 字节 |
  | 表达式求值   | `p EXPR`      | `p $eax + 1`           | 求出表达式 `EXPR` 的值, `EXPR` 支持的运算请见下方            |
  | 设置监视点   | `w EXPR`      | `w *0x2000`            | 当表达式 `EXPR` 的值发生变化时, 暂停程序执行                 |
  | 删除监视点   | `d N`         | `d 2`                  | 删除序号为 `N` 的监视点                                      |

  备注：

  - (1) 命令已实现

  - (2) 命令与 GDB 相比进行了简化，更改了命令的格式

  - 以上表格来自 [PA1.4](https://nju-projectn.github.io/ics-pa-gitbook/ics2022/1.4.html)

  - `EXPR` 支持的运算（采用 BNF 范式定义）

    ```
    <expr> ::= <decimal-number>
      | <hexadecimal-number>    # 以"0x"开头
      | <reg_name>              # 以"$"开头
      | "(" <expr> ")"
      | <expr> "+" <expr>
      | <expr> "-" <expr>
      | <expr> "*" <expr>
      | <expr> "/" <expr>
      | <expr> "==" <expr>
      | <expr> "!=" <expr>
      | <expr> "&&" <expr>
      | "*" <expr>              # 指针解引用
    ```

- 50 条 RISC-V ISA 指令，涵盖

  - 全部 RV32I Base Instruction Set
  - 全部 RV32M Standard Extension
  - 部分 RV32/RV64 Zicsr Standard Extension

- 异常 / 中断响应处理机制

- 设备及其内存映射 (mmap)

  - 串口 serial
  - 时钟 timer
  - 键盘 keyboard
  - VGA (GPU)
  - 声卡 audio

- Sv32 分页及其 page table walk

### Abstract Machine - 裸机(bare-metal)运行时环境

**设计 AM 的目的：程序与架构（ISA 以及 platform）解耦**

- TRM(Turing Machine) - 图灵机, 最简单的运行时环境, 为程序提供基本的计算能力，包含如下 API

  ```c
  // ----------------------- TRM: Turing Machine -----------------------
  extern   Area        heap;																			// 堆，程序运行时动态分配的内存区域
  void     putch       (char ch);																	// 输出字符
  void     halt        (int code) __attribute__((__noreturn__));	// 停机
  void 		 _trm_init	 (void)																			// TRM初始化
  ```

- IOE(I/O Extension) - 输入输出扩展, 为程序提供输出输入的能力，包含如下 API

  ```c
  // -------------------- IOE: Input/Output Devices --------------------
  bool     ioe_init    (void);								// IOE初始化
  void     ioe_read    (int reg, void *buf);	// 从IO读入内容
  void     ioe_write   (int reg, void *buf);	// 将内容写入IO
  ```

- CTE(Context Extension) - 上下文扩展, 为程序提供上下文管理的能力，包含如下 API

  ```c
  // ---------- CTE: Interrupt Handling and Context Switching ----------
  bool     cte_init    (Context *(*handler)(Event ev, Context *ctx));		// CTE初始化
  void     yield       (void);																					// 程序自陷
  bool     ienabled    (void);																					// PA不使用
  void     iset        (bool enable);																		// PA不使用
  Context *kcontext    (Area kstack, void (*entry)(void *), void *arg);	// 内核进程上下文创建
  ```

- VME(Virtual Memory Extension) - 虚存扩展, 为程序提供虚存管理的能力，包含如下 API

  ```c
  // ----------------------- VME: Virtual Memory -----------------------
  bool     vme_init    (void *(*pgalloc)(int), void (*pgfree)(void *));			// VME初始化
  void     protect     (AddrSpace *as);																			// 为用户进程创建地址空间
  void     unprotect   (AddrSpace *as);																			// 销毁用户进程创建地址空间
  void     map         (AddrSpace *as, void *vaddr, void *paddr, int prot);	// 进行虚拟-物理内存映射
  Context *ucontext    (AddrSpace *as, Area kstack, void *entry);						// 用户进程上下文创建
  ```

- Klib (kernel library，兼容 libc 的常用库函数)

  - `stdio.c`

    ```c
    // stdio.h
    int    printf    (const char *format, ...);
    int    sprintf   (char *str, const char *format, ...);
    int    snprintf  (char *str, size_t size, const char *format, ...);
    int    vsprintf  (char *str, const char *format, va_list ap);
    int    vsnprintf (char *str, size_t size, const char *format, va_list ap);
    ```

  - `stdlib.c`

    ```c
    // stdlib.h
    void   srand     (unsigned int seed);
    int    rand      (void);
    void  *malloc    (size_t size);
    void   free      (void *ptr);
    int    abs       (int x);
    int    atoi      (const char *nptr);
    ```

  - `string.c`

    ```c
    // string.h
    void  *memset    (void *s, int c, size_t n);
    void  *memcpy    (void *dst, const void *src, size_t n);
    void  *memmove   (void *dst, const void *src, size_t n);
    int    memcmp    (const void *s1, const void *s2, size_t n);
    size_t strlen    (const char *s);
    char  *strcat    (char *dst, const char *src);
    char  *strcpy    (char *dst, const char *src);
    char  *strncpy   (char *dst, const char *src, size_t n);
    int    strcmp    (const char *s1, const char *s2);
    int    strncmp   (const char *s1, const char *s2, size_t n);
    char  *strtok    (char *str, const char *delim);
    ```

### Nanos-lite

- 内存磁盘 ramdisk 及其简易驱动程序

- Sv32 分页及虚拟内存管理

- 基于时间片的抢占分时多任务，支持最多4个用户进程和内核进程分时运行，使用时钟中断驱动

- 简易文件系统 sfs (simple file system)

  - 文件数量和大小固定

  - 无目录结构

  - 采用虚拟文件系统 (VFS) 和一切皆文件思想进行文件抽象

    - 标准输入输出错误流

    - 设备（串口、键盘、VGA、声卡）

    - 含如下 API

      ```c
      typedef struct {
        char 	 *name;         	// 文件名
        size_t 	size;        		// 文件大小
        size_t 	disk_offset;  	// 文件在ramdisk中的偏移
        ReadFn 	read;        		// 读函数指针
        WriteFn write;      		// 写函数指针
      } Finfo;
      
      int 		fs_open	(const char *pathname, int flags, int mode);
      size_t 	fs_read	(int fd, void *buf, size_t len);
      size_t 	fs_write(int fd, const void *buf, size_t len);
      size_t 	fs_lseek(int fd, size_t offset, int whence);
      int 		fs_close(int fd);
      ```

- 9 个系统调用 (open, read, write, lseek, close, gettimeofday, brk, exit, execve)

- 进程 scheduler

- 进程 loader

### Navy-apps

- `fsimg` 文件系统镜像

  ```
  bin/ -- 二进制文件(该目录默认未创建)
  share/  -- 平台无关文件
    files/ -- 用于文件测试
    fonts/ -- 字体文件
    music/ -- 示例音乐
    pictures/ -- 示例图像
    games/ -- 游戏数据(该目录默认未创建)
      nes/ -- NES Roms
      pal/ -- 仙剑奇侠传相关数据文件
  ```

- 各种用户程序运行时库
  
  - `libc`: C运行库, 移植自 newlib 3.3.0, 同时包含最小的手写 C++ 运行库(但不支持 libstdc++)
  - `libos`: 系统调用接口
  - `compiler-rt`: 移植自 llvm, 主要用于在 32 位 ISA 上提供 64 位除法的支持
  - `libfixedptc`: 提供定点数计算的支持, 包含 `sin`, `pow`, `log` 等初等函数功能, 可以替代范围不大的浮点数
  - `libndl`: NDL(NJU DirectMedia Layer), 提供时钟, 按键, 绘图, 声音的底层抽象
  - `libbmp`: 支持 32 位 BMP 格式文件的读取
  - `libbdf`: 支持 BDF 字体格式的读取
  - `libminiSDL`: 基于 NDL 的多媒体库, 提供部分兼容 SDL 库的 API
  - `libvorbis`: OGG 音频解码器, 仅支持解码成 `s16le` 格式的PCM编码, 移植自 stb 项目
  - `libam`: 通过 Navy 的运行时环境实现的 AM API, 可以运行 AM 程序, 目前仅支持 TRM 和 IOE
  - `libSDL_ttf`: 基于 libminiSDL 和 stb 项目中的 truetype 字体解析器, 支持 truetype 字体的光栅化
  - `libSDL_image`: 基于 libminiSDL 和 stb 项目中的图像解码器, 支持 JPG, PNG, BMP, GIF 的解码
  - `libSDL_mixer`: 基于 libminiSDL 和 libvorbis, 支持多通道混声, 目前仅支持 OGG 格式
  
- 多种应用程序
  - `nslider`: NJU 幻灯片播放器, 可以播放 4:3 的幻灯片
  - `menu`: 启动菜单, 可以选择需要运行的应用程序
  - `nterm`: NJU 终端, 包含一个简易內建 Shell, 也支持外部 Shell
  - `bird`: flappy bird, 移植自 sdlbird
  - `pal`: 仙剑奇侠传, 支持音效, 移植自 SDLPAL
  - `am-kernels`: 可在 libam 的支持下运行部分AM应用, 目前支持 coremark, dhrystone 和打字小游戏
  - `fceux`: 可在 libam 的支持下运行的红白机模拟器
  - `oslab0`: 学生编写的游戏集合, 可在 libam 的支持下运行
  - `nplayer`: NJU 音频播放器, 支持音量调节和可视化效果, 目前仅支持 OGG 格式
  - `lua`: LUA 脚本解释器
  - `busybox`: BusyBox 套件(版本 1.32.0), 系统调用较少时只能运行少量工具
  - `onscripter`: NScripter 脚本解释器, 在Navy环境中可支持 Planetarian, Clannad 等游戏的运行
  - `nwm`: NJU 窗口管理器(需要 mmap, newlib 中不支持, 故目前只能运行在 native)


### 基础设施

- sdb (simple debugger)
- tracer
  - itrace (instruction trace)
  - iringbuf (instruction ring buffer)
  - mtrace (memory trace)
  - ftrace (function trace)
  - dtrace (device trace)
  - etrace (exception trace)
  - strace (syscall trace)
- AM native (Abstract Machine with host hardware, not NEMU)
- Nanos-lite native (Nanos-lite with AM native)
- Navy native (Navy with Linux native)
- difftest (Differential Testing)
- snapshot
