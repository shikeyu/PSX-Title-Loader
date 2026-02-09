#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libapi.h>
#include <libcd.h>

// 配置：原本的游戏文件路径（注意必须大写，;1 表示版本号）
// 通常在光盘根目录下
// 如果无法启动，请尝试改为标准 PSX 启动文件名 (如 SLPS_031.77;1)
#define ORIGINAL_GAME_PATH "\\GAME.EXE;1"

// 双缓冲定义
DISPENV disp[2];
DRAWENV draw[2];
int db = 0; // 当前缓冲索引

// 加载纹理的辅助函数声明
void LoadTexture(unsigned long *tim, TIM_IMAGE *tparam);

// 初始化系统
void InitSystem() {
    // 重置图形系统
    ResetGraph(0);
    
    // 初始化几何运算库
    InitGeom();
    
    // 初始化双缓冲
    // 320x240 分辨率
    SetDefDispEnv(&disp[0], 0, 0, 320, 240);
    SetDefDispEnv(&disp[1], 0, 240, 320, 240);
    SetDefDrawEnv(&draw[0], 0, 240, 320, 240);
    SetDefDrawEnv(&draw[1], 0, 0, 320, 240);
    
    // 设置背景色为黑色
    draw[0].isbg = 1; draw[0].r0 = 0; draw[0].g0 = 0; draw[0].b0 = 0;
    draw[1].isbg = 1; draw[1].r0 = 0; draw[1].g0 = 0; draw[1].b0 = 0;
    
    // 应用绘制环境
    PutDrawEnv(&draw[0]);
    PutDispEnv(&disp[0]);
    
    SetDispMask(1); // 打开显示
    
    // 初始化手柄
    PadInit(0);
}

// 绘制全屏图片的辅助函数 (自动切分 TPage)
void DrawFullScreenSprite(TIM_IMAGE *tim, int brightness) {
    POLY_FT4 poly;
    int mode = tim->mode & 0x3;
    int width = tim->prect->w; 
    int pixel_w;
    int height;
    
    int off_x, off_y;
    int u_off, v_off;
    
    int current_x, remaining_w;
    int current_u;
    int current_vram_x;
    int poly_w, dist_to_edge;
    int pixel_processed = 0; // 已处理的像素数
    int segment_vram_x;
    int u0, u1;
    
    // 自动修正：如果检测到存在 CLUT (bit 3) 但模式是 16-bit (2)，
    // 这通常意味着这是一个 8-bit 图片但被错误标记或解释。
    // 症状：图片宽度被压缩一半，且颜色错乱。
    // if ((tim->mode & 0x8) && (mode == 2)) {
    //    mode = 1; // 强制修正为 8-bit 模式
    // }
    
    // 根据颜色模式计算像素宽度
    if (mode == 2) pixel_w = width;       // 16-bit
    else if (mode == 1) pixel_w = width * 2; // 8-bit
    else pixel_w = width * 4;                // 4-bit

    // 限制高度
    height = tim->prect->h;
    if (height > 240) height = 240;

    // 计算居中偏移 (屏幕分辨率 320x240)
    off_x = (320 - pixel_w) / 2;
    off_y = (240 - height) / 2;
    
    // 计算纹理在 TPage 内的起始 UV 偏移
    if (mode == 2) { // 16-bit
        u_off = (tim->prect->x & 0x3F);
    } else if (mode == 1) { // 8-bit
        u_off = (tim->prect->x & 0x3F) * 2;
    } else { // 4-bit
        u_off = (tim->prect->x & 0x3F) * 4;
    }
    v_off = tim->prect->y & 0xFF;

    // 循环切分绘制
    current_x = off_x;
    current_u = u_off;
    remaining_w = pixel_w;

    while (remaining_w > 0) {
        // 计算当前 TPage 剩余宽度
        // 关键：如果 current_u 已经是 0 (新页开始)，则 dist 为 256
        dist_to_edge = 256 - current_u;
        
        // 本次绘制宽度：取 剩余宽度 和 到边缘距离 的最小值
        // 只要不超过 TPage 边界，我们就尽可能多画
        poly_w = remaining_w;
        if (poly_w > dist_to_edge) poly_w = dist_to_edge;

        // 设置多边形
        SetPolyFT4(&poly);
        setRGB0(&poly, brightness, brightness, brightness);
        setXY4(&poly, 
               current_x, off_y,
               current_x + poly_w, off_y,
               current_x, off_y + height,
               current_x + poly_w, off_y + height);
        
        // 设置 UV
        // 关键修正：对于刚好到达 256 边界的情况，我们需要特殊处理
        // 因为 u_char 会溢出。
        // 但是，如果我们将 poly_w 分成两步画 (如之前 max 128 的逻辑)，会产生额外的 QuadTex。
        // 用户反馈有 3 个 QuadTex，说明切分太多了。
        // 
        // 让我们尝试使用 255 作为右边界，接受 1 像素的误差，以避免溢出和额外的切分。
        u0 = current_u;
        u1 = current_u + poly_w;
        
        // 如果 u1 溢出 (256)，则限制为 255。这会造成 1 像素的拉伸，但能避免镜像和切分。
        if (u1 >= 256) u1 = 255;
        
        setUV4(&poly,
               u0, v_off,
               u1, v_off,
               u0, v_off + height,
               u1, v_off + height);

        // 动态计算当前段对应的 VRAM 绝对 X 坐标
        segment_vram_x = tim->prect->x;
        
        // 加上当前的像素偏移转换成的 VRAM Word 偏移
        if (mode == 2) segment_vram_x += pixel_processed;       // 16-bit: 1 pixel = 1 word
        else if (mode == 1) segment_vram_x += pixel_processed / 2; // 8-bit: 2 pixels = 1 word
        else segment_vram_x += pixel_processed / 4;                // 4-bit: 4 pixels = 1 word

        poly.tpage = getTPage(mode, 0, segment_vram_x, tim->prect->y);
        
        if (tim->mode & 0x8) {
            poly.clut = getClut(tim->crect->x, tim->crect->y);
        }
        
        DrawPrim(&poly);

        // 更新状态
        current_x += poly_w;
        remaining_w -= poly_w;
        current_u += poly_w;
        pixel_processed += poly_w;

        // 如果到达 TPage 边缘，重置 U 坐标
        if (current_u >= 256) {
            current_u -= 256; 
        }
    }
}

// 交换缓冲
void Display() {
    DrawSync(0); // 等待GPU完成绘制
    VSync(0);    // 等待垂直同步
    
    PutDispEnv(&disp[db]);
    PutDrawEnv(&draw[db]);
    
    db = !db; // 切换缓冲索引
}


// 定义 PSX 可执行文件头结构体 (全局定义)
// struct EXEC 已经在 libapi.h 中定义，无需重复定义

// 辅助函数：设置 GP 寄存器
void SetGp(unsigned long gp) {
    __asm__ volatile ("move $gp, %0" : : "r" (gp));
}

int main() {
    int pad;
    int brightness = 128; // 128 是标准亮度 (0-128-255)
    int state = 0; // 0: 等待按键释放, 1: 等待按键按下, 2: 淡出
    TIM_IMAGE timInfo;
    SPRT sprite;
    
    // 2. 加载图片使用的变量
    // 增加缓冲区大小以支持全屏 16-bit 图片 (320x240x2 = ~150KB)
    // 256KB buffer (65536 * 4 bytes)
    static u_long texture_buffer[65536]; 
    CdlFILE filePos;
    int hasTexture = 0; // 标记是否成功加载纹理

    // 手动加载游戏所需变量
    struct EXEC *header;
    void (*ExecuteGame)();
    int next_sector;
    CdlLOC next_pos;
    int bytes_to_read;
    int sectors_to_read;

    // 1. 系统初始化
    InitSystem();
    CdInit();

    if (CdSearchFile(&filePos, "\\TITLE.TIM;1")) {
        CdControl(CdlSetloc, (u_char *)&filePos.pos, 0);
        CdRead((filePos.size + 2047) / 2048, texture_buffer, CdlModeSpeed);
        CdReadSync(0, 0);
        LoadTexture(texture_buffer, &timInfo);
        hasTexture = 1;
    } else {
        printf("TITLE.TIM not found on CD\n");
    }

    // 3. 初始化用于显示的精灵 (Sprite)
    // 逻辑移到了 DrawFullScreenSprite 中

    // 主循环
    while (1) {
        // 读取手柄状态
        pad = PadRead(0);

        // 状态机逻辑
        if (state == 0) {
            // 状态0：等待初始按键释放 (防止启动时残留的按键输入导致直接跳过)
            if (pad == 0 || pad == 0xFFFF) {
                state = 1;
            }
        } else if (state == 1) {
            // 状态1：显示图片，等待按键按下
             if ((pad & 0xFFFF) != 0xFFFF && pad != 0) {
                 state = 2;
             }
        } else if (state == 2) {
            // 状态2：淡出
            brightness -= 4; // 调节淡出速度
            if (brightness <= 0) {
                brightness = 0;
                if (hasTexture) DrawFullScreenSprite(&timInfo, brightness);
                Display();
                break; // 退出循环，准备跳转
            }
        }

        // 绘制全屏图片
        if (hasTexture) {
            DrawFullScreenSprite(&timInfo, brightness);
        }
        Display();
    }

    // 4. 清理
    DrawSync(0);
    VSync(0);       // 确保当前场结束
    SetDispMask(0); // 关闭显示
    
    // PadStop 和 StopCallback 移到加载完成后执行
    // EnterCriticalSection 和 ResetGraph 也移到加载完成后
    
    printf("Launching %s...\n", ORIGINAL_GAME_PATH);

    // 5. 手动加载原游戏 (Manual Load & Exec)
    // 这种方法比 LoadExec 更底层，可以绕过 LoadExec 在高内存地址时的潜在问题
    // 步骤：读取头 -> 读取代码到内存 -> 停止系统 -> 跳转
    
    // 查找文件
    if (CdSearchFile(&filePos, ORIGINAL_GAME_PATH)) {
        printf("Found %s, loading...\n", ORIGINAL_GAME_PATH);
        
        // 1. 读取头部 (PSX-EXE Header is 2048 bytes)
        CdControl(CdlSetloc, (u_char *)&filePos.pos, 0);
        CdRead(1, texture_buffer, CdlModeSpeed);
        CdReadSync(0, 0);
        
        // PSX EXE 头部的结构：
        // 0x00-0x07: "PS-X EXE" (Magic)
        // 0x08-0x0F: 0 (Padding)
        // 0x10: PC0 (struct EXEC 开始)
        // 因此我们需要跳过前 16 字节 (4个 u_long)
        header = (struct EXEC *)(texture_buffer + 4);
        
        printf("PC: %08x, GP: %08x, SP: %08x\n", header->pc0, header->gp0, header->s_addr);
        printf("Text: %08x (%d bytes)\n", header->t_addr, header->t_size);
        
        // 2. 读取游戏代码主体
        // 计算下一个扇区的位置 (跳过头部 2048 字节)
        // 使用 CdPosToInt 和 CdIntToPos 进行扇区计算
        
        next_sector = CdPosToInt(&filePos.pos) + 1;
        CdIntToPos(next_sector, &next_pos);
        
        // 定位到代码开始处
        CdControl(CdlSetloc, (u_char *)&next_pos, 0);
        
        // 读取剩余扇区到目标内存地址
        // 注意：我们要读取整个 text 段 + data 段等。
        // 简单起见，我们读取 (文件总大小 - 2048) 字节
        // 向上取整到扇区
        bytes_to_read = filePos.size - 2048;
        sectors_to_read = (bytes_to_read + 2047) / 2048;
        
        printf("Reading %d sectors to %08x...\n", sectors_to_read, header->t_addr);
        
        // 检查是否会覆盖当前 Loader (Loader at 0x80100000)
        if (header->t_addr + bytes_to_read > 0x80100000) {
            printf("WARNING: Game size too large, might overwrite loader!\n");
            // 这里如果发生覆盖，程序会崩溃。
            // 解决方案需要将 Loader 搬得更高，或者使用 Stub。
            // 但先尝试运行。
        }

        CdRead(sectors_to_read, (unsigned long *)header->t_addr, CdlModeSpeed);
        CdReadSync(0, 0);
        
        printf("Load Complete. Preparing to launch...\n");
        
        // 3. 关闭系统服务
        PadStop();
        StopCallback();
        
        // 4. 进入临界区，禁止中断
        EnterCriticalSection();
        
        // 5. 重置图形系统
        ResetGraph(3);
        FlushCache();
        
        // 6. 设置寄存器并跳转
        // 设置栈指针 (SP)
        // 注意：有些游戏头部的 s_addr/s_size 是 0，这时需要使用默认值或者不设置
        if (header->s_addr != 0) {
             SetSp(header->s_addr + header->s_size);
        } else {
             // 如果未指定，通常指向 0x801FFFF0 (但我们在这里运行 Loader，可能会冲突)
             // 安全起见，指向 RAM 顶端
             SetSp(0x801FFFF0); 
        }
        
        // 设置全局指针 (GP)
        SetGp(header->gp0);
        
        // 跳转执行
        ExecuteGame = (void (*)())header->pc0;
        ExecuteGame();
        
    } else {
        printf("Game file not found!\n");
    }

    return 0;
}

// TIM加载函数实现
void LoadTexture(unsigned long *tim, TIM_IMAGE *tparam) {
    // 解析TIM头
    OpenTIM(tim);
    ReadTIM(tparam);
    
    // 上传像素数据到VRAM
    LoadImage(tparam->prect, tparam->paddr);
    DrawSync(0);
    
    // 如果有CLUT (调色板)，也上传
    if (tparam->mode & 0x8) {
        LoadImage(tparam->crect, tparam->caddr);
        DrawSync(0);
    }
}
