#ifndef __pinmux_defs_h
#define __pinmux_defs_h

#ifndef REG_RD
#define REG_RD( scope, inst, reg ) \
  REG_READ( reg_##scope##_##reg, \
            (inst) + REG_RD_ADDR_##scope##_##reg )
#endif

#ifndef REG_WR
#define REG_WR( scope, inst, reg, val ) \
  REG_WRITE( reg_##scope##_##reg, \
             (inst) + REG_WR_ADDR_##scope##_##reg, (val) )
#endif

#ifndef REG_RD_VECT
#define REG_RD_VECT( scope, inst, reg, index ) \
  REG_READ( reg_##scope##_##reg, \
            (inst) + REG_RD_ADDR_##scope##_##reg + \
	    (index) * STRIDE_##scope##_##reg )
#endif

#ifndef REG_WR_VECT
#define REG_WR_VECT( scope, inst, reg, index, val ) \
  REG_WRITE( reg_##scope##_##reg, \
             (inst) + REG_WR_ADDR_##scope##_##reg + \
	     (index) * STRIDE_##scope##_##reg, (val) )
#endif

#ifndef REG_RD_INT
#define REG_RD_INT( scope, inst, reg ) \
  REG_READ( int, (inst) + REG_RD_ADDR_##scope##_##reg )
#endif

#ifndef REG_WR_INT
#define REG_WR_INT( scope, inst, reg, val ) \
  REG_WRITE( int, (inst) + REG_WR_ADDR_##scope##_##reg, (val) )
#endif

#ifndef REG_RD_INT_VECT
#define REG_RD_INT_VECT( scope, inst, reg, index ) \
  REG_READ( int, (inst) + REG_RD_ADDR_##scope##_##reg + \
	    (index) * STRIDE_##scope##_##reg )
#endif

#ifndef REG_WR_INT_VECT
#define REG_WR_INT_VECT( scope, inst, reg, index, val ) \
  REG_WRITE( int, (inst) + REG_WR_ADDR_##scope##_##reg + \
	     (index) * STRIDE_##scope##_##reg, (val) )
#endif

#ifndef REG_TYPE_CONV
#define REG_TYPE_CONV( type, orgtype, val ) \
  ( { union { orgtype o; type n; } r; r.o = val; r.n; } )
#endif

#ifndef reg_page_size
#define reg_page_size 8192
#endif

#ifndef REG_ADDR
#define REG_ADDR( scope, inst, reg ) \
  ( (inst) + REG_RD_ADDR_##scope##_##reg )
#endif

#ifndef REG_ADDR_VECT
#define REG_ADDR_VECT( scope, inst, reg, index ) \
  ( (inst) + REG_RD_ADDR_##scope##_##reg + \
    (index) * STRIDE_##scope##_##reg )
#endif


typedef struct {
  unsigned int eth       : 1;
  unsigned int eth_mdio  : 1;
  unsigned int geth      : 1;
  unsigned int tg        : 1;
  unsigned int tg_clk    : 1;
  unsigned int vout      : 1;
  unsigned int vout_sync : 1;
  unsigned int ser1      : 1;
  unsigned int ser2      : 1;
  unsigned int ser3      : 1;
  unsigned int ser4      : 1;
  unsigned int sser      : 1;
  unsigned int pwm0      : 1;
  unsigned int pwm1      : 1;
  unsigned int pwm2      : 1;
  unsigned int timer0    : 1;
  unsigned int timer1    : 1;
  unsigned int pio       : 1;
  unsigned int i2c0      : 1;
  unsigned int i2c1      : 1;
  unsigned int i2c1_sda1 : 1;
  unsigned int i2c1_sda2 : 1;
  unsigned int i2c1_sda3 : 1;
  unsigned int i2c1_sen  : 1;
  unsigned int dummy1    : 8;
} reg_pinmux_rw_hwprot;
#define REG_RD_ADDR_pinmux_rw_hwprot 0
#define REG_WR_ADDR_pinmux_rw_hwprot 0

typedef struct {
  unsigned int pa0  : 1;
  unsigned int pa1  : 1;
  unsigned int pa2  : 1;
  unsigned int pa3  : 1;
  unsigned int pa4  : 1;
  unsigned int pa5  : 1;
  unsigned int pa6  : 1;
  unsigned int pa7  : 1;
  unsigned int pa8  : 1;
  unsigned int pa9  : 1;
  unsigned int pa10 : 1;
  unsigned int pa11 : 1;
  unsigned int pa12 : 1;
  unsigned int pa13 : 1;
  unsigned int pa14 : 1;
  unsigned int pa15 : 1;
  unsigned int pa16 : 1;
  unsigned int pa17 : 1;
  unsigned int pa18 : 1;
  unsigned int pa19 : 1;
  unsigned int pa20 : 1;
  unsigned int pa21 : 1;
  unsigned int pa22 : 1;
  unsigned int pa23 : 1;
  unsigned int pa24 : 1;
  unsigned int pa25 : 1;
  unsigned int pa26 : 1;
  unsigned int pa27 : 1;
  unsigned int pa28 : 1;
  unsigned int pa29 : 1;
  unsigned int pa30 : 1;
  unsigned int pa31 : 1;
} reg_pinmux_rw_gio_pa;
#define REG_RD_ADDR_pinmux_rw_gio_pa 4
#define REG_WR_ADDR_pinmux_rw_gio_pa 4

typedef struct {
  unsigned int pb0  : 1;
  unsigned int pb1  : 1;
  unsigned int pb2  : 1;
  unsigned int pb3  : 1;
  unsigned int pb4  : 1;
  unsigned int pb5  : 1;
  unsigned int pb6  : 1;
  unsigned int pb7  : 1;
  unsigned int pb8  : 1;
  unsigned int pb9  : 1;
  unsigned int pb10 : 1;
  unsigned int pb11 : 1;
  unsigned int pb12 : 1;
  unsigned int pb13 : 1;
  unsigned int pb14 : 1;
  unsigned int pb15 : 1;
  unsigned int pb16 : 1;
  unsigned int pb17 : 1;
  unsigned int pb18 : 1;
  unsigned int pb19 : 1;
  unsigned int pb20 : 1;
  unsigned int pb21 : 1;
  unsigned int pb22 : 1;
  unsigned int pb23 : 1;
  unsigned int pb24 : 1;
  unsigned int pb25 : 1;
  unsigned int pb26 : 1;
  unsigned int pb27 : 1;
  unsigned int pb28 : 1;
  unsigned int pb29 : 1;
  unsigned int pb30 : 1;
  unsigned int pb31 : 1;
} reg_pinmux_rw_gio_pb;
#define REG_RD_ADDR_pinmux_rw_gio_pb 8
#define REG_WR_ADDR_pinmux_rw_gio_pb 8

typedef struct {
  unsigned int pc0  : 1;
  unsigned int pc1  : 1;
  unsigned int pc2  : 1;
  unsigned int pc3  : 1;
  unsigned int pc4  : 1;
  unsigned int pc5  : 1;
  unsigned int pc6  : 1;
  unsigned int pc7  : 1;
  unsigned int pc8  : 1;
  unsigned int pc9  : 1;
  unsigned int pc10 : 1;
  unsigned int pc11 : 1;
  unsigned int pc12 : 1;
  unsigned int pc13 : 1;
  unsigned int pc14 : 1;
  unsigned int pc15 : 1;
  unsigned int dummy1 : 16;
} reg_pinmux_rw_gio_pc;
#define REG_RD_ADDR_pinmux_rw_gio_pc 12
#define REG_WR_ADDR_pinmux_rw_gio_pc 12

typedef struct {
  unsigned int pa0  : 1;
  unsigned int pa1  : 1;
  unsigned int pa2  : 1;
  unsigned int pa3  : 1;
  unsigned int pa4  : 1;
  unsigned int pa5  : 1;
  unsigned int pa6  : 1;
  unsigned int pa7  : 1;
  unsigned int pa8  : 1;
  unsigned int pa9  : 1;
  unsigned int pa10 : 1;
  unsigned int pa11 : 1;
  unsigned int pa12 : 1;
  unsigned int pa13 : 1;
  unsigned int pa14 : 1;
  unsigned int pa15 : 1;
  unsigned int pa16 : 1;
  unsigned int pa17 : 1;
  unsigned int pa18 : 1;
  unsigned int pa19 : 1;
  unsigned int pa20 : 1;
  unsigned int pa21 : 1;
  unsigned int pa22 : 1;
  unsigned int pa23 : 1;
  unsigned int pa24 : 1;
  unsigned int pa25 : 1;
  unsigned int pa26 : 1;
  unsigned int pa27 : 1;
  unsigned int pa28 : 1;
  unsigned int pa29 : 1;
  unsigned int pa30 : 1;
  unsigned int pa31 : 1;
} reg_pinmux_rw_iop_pa;
#define REG_RD_ADDR_pinmux_rw_iop_pa 16
#define REG_WR_ADDR_pinmux_rw_iop_pa 16

typedef struct {
  unsigned int pb0 : 1;
  unsigned int pb1 : 1;
  unsigned int pb2 : 1;
  unsigned int pb3 : 1;
  unsigned int pb4 : 1;
  unsigned int pb5 : 1;
  unsigned int pb6 : 1;
  unsigned int pb7 : 1;
  unsigned int dummy1 : 24;
} reg_pinmux_rw_iop_pb;
#define REG_RD_ADDR_pinmux_rw_iop_pb 20
#define REG_WR_ADDR_pinmux_rw_iop_pb 20

typedef struct {
  unsigned int d0    : 1;
  unsigned int d1    : 1;
  unsigned int d2    : 1;
  unsigned int d3    : 1;
  unsigned int d4    : 1;
  unsigned int d5    : 1;
  unsigned int d6    : 1;
  unsigned int d7    : 1;
  unsigned int rd_n  : 1;
  unsigned int wr_n  : 1;
  unsigned int a0    : 1;
  unsigned int a1    : 1;
  unsigned int ce0_n : 1;
  unsigned int ce1_n : 1;
  unsigned int ce2_n : 1;
  unsigned int rdy   : 1;
  unsigned int dummy1 : 16;
} reg_pinmux_rw_iop_pio;
#define REG_RD_ADDR_pinmux_rw_iop_pio 24
#define REG_WR_ADDR_pinmux_rw_iop_pio 24

typedef struct {
  unsigned int usb0 : 1;
  unsigned int dummy1 : 31;
} reg_pinmux_rw_iop_usb;
#define REG_RD_ADDR_pinmux_rw_iop_usb 28
#define REG_WR_ADDR_pinmux_rw_iop_usb 28


enum {
  regk_pinmux_no                           = 0x00000000,
  regk_pinmux_rw_gio_pa_default            = 0x00000000,
  regk_pinmux_rw_gio_pb_default            = 0x00000000,
  regk_pinmux_rw_gio_pc_default            = 0x00000000,
  regk_pinmux_rw_hwprot_default            = 0x00000000,
  regk_pinmux_rw_iop_pa_default            = 0x00000000,
  regk_pinmux_rw_iop_pb_default            = 0x00000000,
  regk_pinmux_rw_iop_pio_default           = 0x00000000,
  regk_pinmux_rw_iop_usb_default           = 0x00000001,
  regk_pinmux_yes                          = 0x00000001
};
#endif 
