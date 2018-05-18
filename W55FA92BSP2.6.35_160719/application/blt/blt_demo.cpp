#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <linux/fb.h>
#include <getopt.h>
#include <termios.h>

#include "w55fa92_blt.h"

#define _BITBLT_SRC_BGMAP_SIZE_ (480*272*2)
#define _BITBLT_SRC_SPMAP_SIZE_ (736*160*2)

#define DEVMEM_GET_STATUS   _IOR('m', 4, unsigned int)
#define DEVMEM_SET_STATUS   _IOW('m', 5, unsigned int)
#define DEVMEM_GET_PHYSADDR _IOR('m', 6, unsigned int)
#define DEVMEM_GET_VIRADDR  _IOR('m', 7, unsigned int)

#define IOCTL_LCD_ENABLE_INT    _IO('v', 28)
#define IOCTL_LCD_DISABLE_INT   _IO('v', 29)

struct S_DEMO_FPS
{
    S_DEMO_FPS() :
        fps(25)
    {
        // Fix this app may sleep indefinitively at start.
        //tv_prev.tv_sec = 0;
        //tv_prev.tv_usec = 0;
        gettimeofday(&tv_prev, NULL);
    }
    
    void wait_intvl()
    {
        int intvl = 1000 / fps; // Frame update interval in ms.
        while (true) {
            gettimeofday(&tv_cur, NULL);
            int et = (tv_cur.tv_sec - tv_prev.tv_sec) * 1000 + (tv_cur.tv_usec - tv_prev.tv_usec) / 1000;   // Elapsed time in ms.
            int rmn = intvl - et;
            if (rmn > 0) {
                usleep(rmn * 1000);
                continue;
            }
            
            tv_prev = tv_cur;
            break;
        }
    }
    
    int                 fps;
    
private:
    struct timeval      tv_prev;
    struct timeval      tv_cur;
};

S_DEMO_FPS g_sDemo_Fps;

struct S_DEMO_BITBLT
{
    S_DEMO_BITBLT() :
        blt_fd(-1),
        
        dest_fd(-1),
        pDestBuffer(NULL),
        DestGPUAddr(0),
        bitblt_dest_map_size(0),
        
        srcBG_fd(-1),
        pSrcBGBuffer(NULL),
        SrcBGGPUAddr(0),
        
        srcSP_fd(-1),
        pSrcSPBuffer(NULL),
        SrcSPGPUAddr(0),
        
        u16Index(0),
        
        go(false),
        signo(-1),
        
        mmu(1),
        color_key(1),
        animate_alpha(0)
    {
    }
    
    ~S_DEMO_BITBLT()
    {
        if (mmu) {
            free(pDestBuffer);
            pDestBuffer = NULL;
            DestGPUAddr = 0;
        
            free(pSrcBGBuffer);
            pSrcBGBuffer = NULL;
            SrcBGGPUAddr = 0;
        
            free(pSrcSPBuffer);
            pSrcSPBuffer = NULL;
            SrcSPGPUAddr = 0;
        }
        else {
            if (pDestBuffer != NULL && pDestBuffer != MAP_FAILED) {
                munmap(pDestBuffer, bitblt_dest_map_size);
            }
            pDestBuffer = NULL;
            close(dest_fd);
            dest_fd = -1;
        
            if (pSrcBGBuffer != NULL && pSrcBGBuffer != MAP_FAILED) {
                munmap(pSrcBGBuffer, _BITBLT_SRC_BGMAP_SIZE_);
            }
            pSrcBGBuffer = NULL;
            close(srcBG_fd);
            srcBG_fd = -1;
        
            if (pSrcSPBuffer != NULL && pSrcSPBuffer != MAP_FAILED) {
                munmap(pSrcSPBuffer, _BITBLT_SRC_SPMAP_SIZE_);
            }
            pSrcSPBuffer = NULL;
            close(srcSP_fd);
            srcSP_fd = -1;
        
            close(blt_fd);
            blt_fd = -1;
        }
        
        if (signo >= 0) {
            const char *sigstr = strsignal(signo);
            printf("\nReceived signal: %s(%d)\n",  sigstr ? sigstr : "Unknown" , signo);
            fflush(stdout);
        }
    }
    
    bool set_colkey(unsigned short u16RGB565ColorKey) {
        if ((ioctl(blt_fd, BLT_IOCTRGB565COLORKEY, u16RGB565ColorKey)) < 0) {
            fprintf(stderr, "set RGB565 color key parameter failed: %s\n", strerror(errno));

            return false;
        }

        return true;
    }

    bool ena_colkey(bool bEnabled)
    {
        if (bEnabled) {
            //printf( "### Set RGB565 color key enable\n" );

            if ((ioctl(blt_fd, BLT_ENABLE_RGB565_COLORCTL, NULL)) < 0) {
                fprintf(stderr, "enable RGB565 color key failed: %s\n", strerror(errno));

                return false;
            }
        }
        else {
            //printf( "### Set RGB565 color key disable\n" );

            if ((ioctl(blt_fd, BLT_DISABLE_RGB565_COLORCTL, NULL)) < 0) {
                fprintf(stderr, "disable RGB565 color key failed: %s\n", strerror(errno));

                return false;
            }
        }

        return true;
    }

    bool init()
    {
        blt_fd = open("/dev/blt", O_RDWR);
        if (blt_fd == -1) {
            printf( "Open BLT device failed: %s\n", strerror(errno));
            return false;
        }

        u16Index = 0;
        go = true;
    
        if (ioctl(blt_fd, BLT_IOCTMMU, mmu) == -1) {
            printf("### Error: failed to configure BLT device to MMU mode: %s\n", strerror(errno));
            return false;
        }
    
        return true;
    }

    bool set_dest_buff(int width, int height, int dpp)
    {
        if (mmu) {  // Go MMU mode. GPU addr = virt addr.
            dest_fd = -1;
            bitblt_dest_map_size = width * height * (dpp / 8);
            pDestBuffer = NULL;
            DestGPUAddr = 0;
            int err = posix_memalign(&pDestBuffer, 4, bitblt_dest_map_size);
            if (err) {
                printf("posix_memalign failed: %s\n", strerror(err));
                return false;
            }
            DestGPUAddr = (unsigned long) pDestBuffer;
        }
        else {
            dest_fd = open("/dev/devmem", O_RDWR);
            if (dest_fd == -1) {
                printf( "Open /dev/devmem failed: %s\n", strerror(errno));
                return false;
            }

            bitblt_dest_map_size = width * height * (dpp / 8);

            pDestBuffer = mmap(NULL, bitblt_dest_map_size, PROT_READ|PROT_WRITE, MAP_SHARED, dest_fd, 0);
            if (pDestBuffer == MAP_FAILED) {
                printf("mmap /dev/mem failed: %s\n", strerror(errno));
                return false;
            }
    
            ioctl(dest_fd, DEVMEM_GET_PHYSADDR, &DestGPUAddr);
        }
        
        return true;
    }

    bool set_srcbg_buff (const char *pszImgFileName)
    {
        if (mmu) {  // Go MMU mode. GPU addr = virt addr.
            srcBG_fd = -1;
            pSrcBGBuffer = NULL;
            SrcBGGPUAddr = 0;
            int err = posix_memalign(&pSrcBGBuffer, 4, _BITBLT_SRC_BGMAP_SIZE_);
            if (err) {
                printf("posix_memalign failed: %s\n", strerror(err));
                return false;
            }
            SrcBGGPUAddr = (unsigned long) pSrcBGBuffer;
        }
        else {
            srcBG_fd = open("/dev/devmem", O_RDWR );
            if (srcBG_fd == -1) {
                printf("Open /dev/devmem failed: %s\n", strerror(errno));
                return false;
            }
        
            pSrcBGBuffer = mmap(NULL, _BITBLT_SRC_BGMAP_SIZE_, PROT_READ|PROT_WRITE, MAP_SHARED, srcBG_fd, 0);
            if (pSrcBGBuffer == MAP_FAILED) {
                printf( "Open /dev/devmem failed: %s\n", strerror(errno));
                return false;
            }
    
            ioctl(srcBG_fd, DEVMEM_GET_PHYSADDR, &SrcBGGPUAddr);
        }
        
        FILE *fpBGImg = fopen(pszImgFileName, "r");
        if (! fpBGImg) {
            printf("Open %s failed: %s\n", pszImgFileName, strerror(errno));
            return false;
        }
        
        if (fread(pSrcBGBuffer, _BITBLT_SRC_BGMAP_SIZE_, 1, fpBGImg) <= 0) {
            printf("Cannot read the BG Image File: %s\n", strerror(errno));
            fclose(fpBGImg);
            return false;
        }

        fclose(fpBGImg);
        fpBGImg = NULL;

        return true;
    }

    bool set_srcsp_buff(const char *pszImgFileName)
    {
        if (mmu) {  // Go MMU mode. GPU addr = virt addr.
            srcSP_fd = -1;
            pSrcSPBuffer = NULL;
            SrcSPGPUAddr = 0;
            int err = posix_memalign(&pSrcSPBuffer, 4, _BITBLT_SRC_SPMAP_SIZE_);
            if (err) {
                printf("posix_memalign failed: %s\n", strerror(err));
                return false;
            }
            SrcSPGPUAddr = (unsigned long) pSrcSPBuffer;
        }
        else {
            srcSP_fd = open("/dev/devmem", O_RDWR);
            if (srcSP_fd == -1) {
                printf( "Open /dev/devmem failed: %s\n", strerror(errno));
                return false;
            }

            pSrcSPBuffer = mmap( NULL, _BITBLT_SRC_SPMAP_SIZE_, PROT_READ|PROT_WRITE, MAP_SHARED, srcSP_fd, 0 );
            if (pSrcSPBuffer == MAP_FAILED) {
                printf("mmap /dev/devmem failed: %s\n", strerror(errno));
                return false;
            }

            ioctl(srcSP_fd, DEVMEM_GET_PHYSADDR, &SrcSPGPUAddr);
        }
        
        FILE *fpSPImg = fopen(pszImgFileName, "r");
        if (! fpSPImg) {
            printf("Open %s failed: %s\n", pszImgFileName, strerror(errno));
            return false;
        }
        
        if (fread(pSrcSPBuffer, _BITBLT_SRC_SPMAP_SIZE_, 1, fpSPImg) <= 0) {
            printf("Cannot read the SP Image File: %s\n", strerror(errno));
            fclose(fpSPImg);
            return false;
        }

        fclose(fpSPImg);
        fpSPImg = NULL;

        return true;
    }

    bool start_fill(S_DRVBLT_FILL_OP &fillop)
    {
        if ((ioctl(blt_fd, BLT_IOCSFILL, &fillop)) == -1) {
            fprintf(stderr, "set FILL parameter failed: %s\n", strerror(errno));
            return false;
        }

        if ((ioctl(blt_fd, BLT_IOCTRIGGER, NULL)) == -1) {
            fprintf(stderr, "trigger BLT failed: %s\n", strerror(errno));
            return false;
        }
        
        return true;
    }
    
    bool start_blit(S_DRVBLT_BLIT_OP &blitop)
    {
        if ((ioctl(blt_fd, BLT_IOCSBLIT, &blitop)) == -1) {
            fprintf(stderr, "set BLIT parameter failed: %s\n", strerror(errno));
            return false;
        }

        if ((ioctl(blt_fd, BLT_IOCTRIGGER, NULL)) == -1) {
            fprintf( stderr, "trigger BLT failed: %s\n", strerror(errno));
            return false;
        }
        
        return true;
    }
    
    bool flush()
    {
        // FIXME
        //if ( ( read( g_sDemo_BitBlt.blt_fd, NULL, 0 ) ) < 0 )
        //{
        //  fprintf( stderr, "read data error errno=%d\n", errno );
        //}
        while (ioctl(blt_fd, BLT_IOCFLUSH) == -1) {
            if (errno == EINTR) {
                continue;
            }
            
            fprintf(stderr, "ioctl BLT_IOCFLUSH failed: %s\n", strerror(errno));
            return false;
        }
        
        return true;
    }   

    int             blt_fd;

    int             dest_fd;
    void *          pDestBuffer;
    unsigned long   DestGPUAddr;        // Actual addr passed to BLT. Virt addr in MMU mode and phys addr in non-MMU mode.
    int             bitblt_dest_map_size;

    int             srcBG_fd;
    void *          pSrcBGBuffer;
    unsigned long   SrcBGGPUAddr;       // Actual addr passed to BLT. Virt addr in MMU mode and phys addr in non-MMU mode.

    int             srcSP_fd;
    void *          pSrcSPBuffer;
    unsigned long   SrcSPGPUAddr;       // Actual addr passed to BLT. Virt addr in MMU mode and phys addr in non-MMU mode.

    unsigned short  u16Index;
    volatile bool   go;
    volatile int    signo;
    
    int             mmu;                // Go MMU mode or not.
    int             color_key;          // color_key or not.
    int             animate_alpha;      // Animate alpha or not.
};

static S_DEMO_BITBLT g_sDemo_BitBlt;

struct S_DEMO_LCM
{
    S_DEMO_LCM() :
        lcm_fd(-1),
        pLCMBuffer(NULL),
        lcm_width(0),
        lcm_height(0),
        lcm_dpp(0),
        lcm_buffer_size(0)
    {
    }
    
    ~S_DEMO_LCM()
    {
        if (pLCMBuffer != NULL && pLCMBuffer != MAP_FAILED) {
            munmap(pLCMBuffer, lcm_buffer_size);
        }
        pLCMBuffer = NULL;
        close(lcm_fd);
        lcm_fd = -1;
    }

    bool init()
    {
        lcm_fd = open("/dev/fb0", O_RDWR);
        if (lcm_fd == -1) {
            printf( "### Error: cannot open LCM device, returns %d\n", lcm_fd);
            return false;
        }

        struct fb_var_screeninfo var;
        if (ioctl(lcm_fd, FBIOGET_VSCREENINFO, &var) == -1) {
            printf( "### Error: ioctl FBIOGET_VSCREENINFO: %s\n", strerror(errno));
        }

        lcm_width = var.xres;
        lcm_height = var.yres;
        lcm_dpp = var.bits_per_pixel;
        lcm_buffer_size = lcm_width * lcm_height * (lcm_dpp / 8);
        
        pLCMBuffer = mmap(NULL, lcm_buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, lcm_fd, 0);
        if (pLCMBuffer == MAP_FAILED) {
            printf("### Error: failed to map LCM device to memory: %s\n", strerror(errno));
            return false;
        }
        
        return true;
    }

    void post_buff(void *src_buff)
    {
        ioctl(lcm_fd, IOCTL_LCD_DISABLE_INT);
        // FIXME: Use BLT to copy to frame buffer.
        // FIXME: Support RGB888 display format.
        memcpy(pLCMBuffer, src_buff, lcm_buffer_size);
        ioctl(lcm_fd, IOCTL_LCD_ENABLE_INT );
    }

    int                     lcm_fd;
    void *                  pLCMBuffer;
    int                     lcm_width;
    int                     lcm_height;
    int                     lcm_dpp;
    int                     lcm_buffer_size;
};

static S_DEMO_LCM g_sDemo_Lcm;

struct S_OBJ
{
    S_OBJ() :
        u32PatternBase(0),
        i32X(0),
        i32Y(0),
        u32Width(0),
        u32Height(0),
        u32Stride(0)
    {
    }
    
    unsigned int    u32PatternBase;
    int             i32X;   // position of X
    int             i32Y;   // position of Y
    unsigned int    u32Width;   // crop width
    unsigned int    u32Height;  // crop height
    unsigned int    u32Stride;
};

static S_OBJ s_saObj[2];

static S_DRVBLT_BLIT_OP             s_sblitop;
static S_DRVBLT_BLIT_TRANSFORMATION s_stransformation;
static S_DRVBLT_FILL_OP             s_sfillop;

void _DemoBitBlt_Config
(
    unsigned int    DestPhysAddr,
    short           i16Width,
    short           i16Height,
    int             destn_dpp
)
{
    s_sblitop.dest.u32FrameBufAddr = DestPhysAddr;
    s_sblitop.dest.i32XOffset = 0;
    s_sblitop.dest.i32YOffset = 0;
    s_sblitop.dest.i32Stride = (i16Width * 2);
    s_sblitop.dest.i16Width = i16Width;
    s_sblitop.dest.i16Height = i16Height;

    s_stransformation.colorMultiplier.i16Blue = 0;
    s_stransformation.colorMultiplier.i16Green = 0;
    s_stransformation.colorMultiplier.i16Red = 0;
    s_stransformation.colorMultiplier.i16Alpha = 0;
    s_stransformation.colorOffset.i16Blue = 0;
    s_stransformation.colorOffset.i16Green = 0;
    s_stransformation.colorOffset.i16Red = 0;
    s_stransformation.colorOffset.i16Alpha = 0;
    s_stransformation.matrix.a = 0x00010000;
    s_stransformation.matrix.b = 0x00000000;
    s_stransformation.matrix.c = 0x00000000;
    s_stransformation.matrix.d = 0x00010000;
    s_stransformation.srcFormat = eDRVBLT_SRC_RGB565;
    s_stransformation.destFormat = (destn_dpp == 16) ? eDRVBLT_DEST_RGB565 : eDRVBLT_DEST_ARGB8888;
    s_stransformation.flags = eDRVBLT_HASTRANSPARENCY;
    s_stransformation.fillStyle = (E_DRVBLT_FILL_STYLE) (eDRVBLT_NOTSMOOTH | eDRVBLT_CLIP);
    s_stransformation.userData = NULL;

    s_sblitop.transformation = &s_stransformation;

    s_sfillop.color.u8Blue = 255;
    s_sfillop.color.u8Green = 0;
    s_sfillop.color.u8Red = 0;
    s_sfillop.color.u8Alpha = 0;
    s_sfillop.blend = false;
    s_sfillop.u32FrameBufAddr = s_sblitop.dest.u32FrameBufAddr;
    s_sfillop.rowBytes = s_sblitop.dest.i32Stride;
    s_sfillop.format = (destn_dpp == 16) ? eDRVBLT_DEST_RGB565 : eDRVBLT_DEST_ARGB8888;
    s_sfillop.rect.i16Xmin = 0;
    s_sfillop.rect.i16Xmax = s_sblitop.dest.i16Width;
    s_sfillop.rect.i16Ymin = 0;
    s_sfillop.rect.i16Ymax = s_sblitop.dest.i16Height;
}

void _DemoBitBlt_SetPosition
(
    unsigned int    u32Index,
    int             i32X,
    int             i32Y
)
{           
    s_saObj[u32Index].i32X = i32X;
    s_saObj[u32Index].i32Y = i32Y;
}

void _DemoBitBlt_SetCrop
(
    unsigned int    u32Index,
    unsigned int    u32Width,
    unsigned int    u32Height
)
{
    s_saObj[u32Index].u32Width = u32Width;
    s_saObj[u32Index].u32Height = u32Height;
}


static short color_multiplier_alpha_animate = 0;

void _DemoBitBlt_AnimateAlpha()
{   
    // Enable color transformation. Only enable alpha channel.
    s_stransformation.flags = s_stransformation.flags | (eDRVBLT_HASCOLORTRANSFORM | eDRVBLT_HASALPHAONLY);
    
    // New Alpha = Old Alpha x Alpha Multiplier + Alpha Offset.
    s_stransformation.colorMultiplier.i16Alpha = color_multiplier_alpha_animate;
    s_stransformation.colorOffset.i16Alpha = 0;
    
    // Animate alpha for next round.
    // Note Alpha Multiplier is in 8.8 fixed format
    color_multiplier_alpha_animate += 0x5;
    if (color_multiplier_alpha_animate > 0x100) {
        color_multiplier_alpha_animate = 0;
    }
}

void _DemoBitBlt_UnanimateAlpha()
{
    // Disable color transformation.
    s_stransformation.flags = eDRVBLT_HASTRANSPARENCY;
    s_stransformation.colorMultiplier.i16Alpha = 0;
    s_stransformation.colorOffset.i16Alpha = 0;
}

void _DemoBitBlt_Render(void)
{
    g_sDemo_Fps.wait_intvl();

    int i;

    if (!g_sDemo_BitBlt.start_fill(s_sfillop)) {    // Configure fill operation and then trigger.
        exit(1);
    }
    
    if (!g_sDemo_BitBlt.flush()) {  // Wait for fill done.
        exit(1);
    }
    
    for ( i = 0; i <= 1; i++ )
    {
        // Animate alpha through color transformation.
        if (g_sDemo_BitBlt.animate_alpha && i == 1) {
            _DemoBitBlt_AnimateAlpha();
        }
        
        s_sblitop.src.pSARGB8 = NULL;
        s_sblitop.src.u32SrcImageAddr = s_saObj[i].u32PatternBase;
        s_sblitop.src.i32Stride = (s_saObj[i].u32Stride << 1);
        s_sblitop.src.i32XOffset = (s_saObj[i].i32X << 16);
        s_sblitop.src.i32YOffset = (s_saObj[i].i32Y << 16);
        s_sblitop.src.i16Width = s_saObj[i].u32Width;
        s_sblitop.src.i16Height = s_saObj[i].u32Height;

        if (!g_sDemo_BitBlt.start_blit(s_sblitop)) {    // Configure blit operation and then trigger.
            exit(1);
        }
        
        if (!g_sDemo_BitBlt.flush()) {  // Wailt for blit done.
            exit(1);
        }
        
        // Unanimate alpha.
        if (g_sDemo_BitBlt.animate_alpha && i == 1) {
            _DemoBitBlt_UnanimateAlpha();
        }
    }

    g_sDemo_Lcm.post_buff(g_sDemo_BitBlt.pDestBuffer);  // VPOST.
}

static void my_sighdlr (int sig)
{
    if (sig == SIGPIPE) { // Ignore SIGPIPE.
        return;
    }
    
    g_sDemo_BitBlt.signo = sig;
    g_sDemo_BitBlt.go = false;
}

static bool _Demo_RegisterSigint()
{
    signal(SIGINT, my_sighdlr);
    signal(SIGHUP, my_sighdlr);
    signal(SIGQUIT, my_sighdlr);
    signal(SIGILL, my_sighdlr);
    signal(SIGABRT, my_sighdlr);
    signal(SIGFPE, my_sighdlr);
    signal(SIGKILL, my_sighdlr);
    //signal(SIGSEGV, my_sighdlr);
    signal(SIGPIPE, my_sighdlr);
    signal(SIGTERM, my_sighdlr);
  
    return true;
}

bool InitSystem()
{
    if (! _Demo_RegisterSigint())
    {
        printf("### Register Sigint Failed\n");
        return false;
    }

    return true;
}

/*
static int kbhit(void)  
{  
    struct termios termios_old, termios_new;  
    int c;  
    int oldf;  
    tcgetattr(STDIN_FILENO, &termios_old);  
    termios_new = termios_old;  
    termios_new.c_lflag &= ~(ICANON | ECHO);  
    tcsetattr(STDIN_FILENO, TCSANOW, &termios_new);  
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);  
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);  
    c = getchar();  
    tcsetattr(STDIN_FILENO, TCSANOW, &termios_old);  
    fcntl(STDIN_FILENO, F_SETFL, oldf);  
    if (c != EOF) {  
        ungetc(c, stdin);
        return 1;  
    }
    
    return 0;  
}*/

int main(int argc, char *argv[])
{
    while (1) {
        int c;
        int option_index = 0;
        
        static struct option long_options[] = {
            {"mmu", required_argument, 0, 'm'},
            {"color_key", required_argument, 0, 'k'},
            {"animate_alpha", required_argument, 0, 'a'},
            {0, 0, 0, 0}
        };
        
        c = getopt_long (argc, argv, "m:k:a:", long_options, &option_index);
        
        // Detect the end of the options.
        if (c == -1) {
            break;
        }
        
        switch (c) {
        case 'm':
            g_sDemo_BitBlt.mmu = strtol (optarg, NULL, 0);
            break;
        
        case 'k':
            g_sDemo_BitBlt.color_key = strtol (optarg, NULL, 0);
            break;
        
        case 'a':
            g_sDemo_BitBlt.animate_alpha = strtol (optarg, NULL, 0);
            break;
            
        default:
            printf ("Usage: %s [-m mmu] [-k color_key] [-a animate_alpha]\n", argv[0]);
            exit(1);
        }
    }
    
    if (!InitSystem()) {
        exit(1);
    }

    g_sDemo_Fps.fps = 30;

    if (!g_sDemo_BitBlt.init()) {
        exit(1);
    }

    if (!g_sDemo_Lcm.init()) {
        exit(1);
    }

    if (!g_sDemo_BitBlt.set_dest_buff(g_sDemo_Lcm.lcm_width, g_sDemo_Lcm.lcm_height, g_sDemo_Lcm.lcm_dpp)) {
        exit(1);
    }

    if (!g_sDemo_BitBlt.set_srcbg_buff("./BG.bin")) {
        exit( 1 );
    }
    s_saObj[0].u32PatternBase = g_sDemo_BitBlt.SrcBGGPUAddr;
    s_saObj[0].u32Stride = 480;

    if (!g_sDemo_BitBlt.set_srcsp_buff("./SP.bin")) {
        exit(1);
    }
    s_saObj[1].u32PatternBase = g_sDemo_BitBlt.SrcSPGPUAddr;
    s_saObj[1].u32Stride = 736;

    if (!g_sDemo_BitBlt.set_colkey(0xF81F)) {
        exit(1);
    }

    // set dest frame buffer address, width and height
    _DemoBitBlt_Config(g_sDemo_BitBlt.DestGPUAddr, g_sDemo_Lcm.lcm_width, g_sDemo_Lcm.lcm_height, g_sDemo_Lcm.lcm_dpp);

    // set position
    _DemoBitBlt_SetPosition( 0, 0, 0 );
    _DemoBitBlt_SetPosition( 1, -200, -100 );

    // set BG widht and height
    _DemoBitBlt_SetCrop( 0, 480, 272 );

    printf("Press Ctrl-C to exit ...\n");
    while ( g_sDemo_BitBlt.go )
    {   
        if ( (s_saObj[0].i32Y++) > 250 )
        {
            if (g_sDemo_BitBlt.color_key) {
                if ( !g_sDemo_BitBlt.ena_colkey( true ) )
                {
                    break;
                }
            }
            s_saObj[0].i32Y = -150;
        }

        if ( (s_saObj[1].i32X++) > 0 )
        {
            if (g_sDemo_BitBlt.color_key) {
                if ( !g_sDemo_BitBlt.ena_colkey( false ) )
                {
                    break;
                }
            }
            s_saObj[1].i32X = -300;
        }

        // crop SPs from SP
        switch ( g_sDemo_BitBlt.u16Index++ )
        {
        case 0:
            s_saObj[1].u32PatternBase = g_sDemo_BitBlt.SrcSPGPUAddr;
            _DemoBitBlt_SetCrop( 1, 96, 160 );
            break;
        case 1:
            s_saObj[1].u32PatternBase = g_sDemo_BitBlt.SrcSPGPUAddr + 96 * 2;
            _DemoBitBlt_SetCrop( 1, 96, 160 );
            break;
        case 2:
            s_saObj[1].u32PatternBase = g_sDemo_BitBlt.SrcSPGPUAddr + (96 + 96) * 2;
            _DemoBitBlt_SetCrop( 1, 112, 160 );
            break;
        case 3:
            s_saObj[1].u32PatternBase = g_sDemo_BitBlt.SrcSPGPUAddr + (96 + 96 + 112) * 2;
            _DemoBitBlt_SetCrop( 1, 112, 160 );
            break;
        case 4:
            s_saObj[1].u32PatternBase = g_sDemo_BitBlt.SrcSPGPUAddr + (96 + 96 + 112 + 112) * 2;
            _DemoBitBlt_SetCrop( 1, 112, 160 );
            break;
        case 5:
            s_saObj[1].u32PatternBase = g_sDemo_BitBlt.SrcSPGPUAddr + (96 + 96 + 112 + 112 + 112) * 2;
            _DemoBitBlt_SetCrop( 1, 112, 160 );
            break;
        case 6:
            s_saObj[1].u32PatternBase = g_sDemo_BitBlt.SrcSPGPUAddr + (96 + 96 + 112 + 112 + 112 + 112) * 2;
            g_sDemo_BitBlt.u16Index = 0;
            _DemoBitBlt_SetCrop( 1, 96, 160 );
            break;
        }

        _DemoBitBlt_Render();   // Render.
    }
    
    return 0;
}
