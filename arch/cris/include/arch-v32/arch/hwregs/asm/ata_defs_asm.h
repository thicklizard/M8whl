#ifndef __ata_defs_asm_h
#define __ata_defs_asm_h


#ifndef REG_FIELD
#define REG_FIELD( scope, reg, field, value ) \
  REG_FIELD_X_( value, reg_##scope##_##reg##___##field##___lsb )
#define REG_FIELD_X_( value, shift ) ((value) << shift)
#endif

#ifndef REG_STATE
#define REG_STATE( scope, reg, field, symbolic_value ) \
  REG_STATE_X_( regk_##scope##_##symbolic_value, reg_##scope##_##reg##___##field##___lsb )
#define REG_STATE_X_( k, shift ) (k << shift)
#endif

#ifndef REG_MASK
#define REG_MASK( scope, reg, field ) \
  REG_MASK_X_( reg_##scope##_##reg##___##field##___width, reg_##scope##_##reg##___##field##___lsb )
#define REG_MASK_X_( width, lsb ) (((1 << width)-1) << lsb)
#endif

#ifndef REG_LSB
#define REG_LSB( scope, reg, field ) reg_##scope##_##reg##___##field##___lsb
#endif

#ifndef REG_BIT
#define REG_BIT( scope, reg, field ) reg_##scope##_##reg##___##field##___bit
#endif

#ifndef REG_ADDR
#define REG_ADDR( scope, inst, reg ) REG_ADDR_X_(inst, reg_##scope##_##reg##_offset)
#define REG_ADDR_X_( inst, offs ) ((inst) + offs)
#endif

#ifndef REG_ADDR_VECT
#define REG_ADDR_VECT( scope, inst, reg, index ) \
         REG_ADDR_VECT_X_(inst, reg_##scope##_##reg##_offset, index, \
			 STRIDE_##scope##_##reg )
#define REG_ADDR_VECT_X_( inst, offs, index, stride ) \
                          ((inst) + offs + (index) * stride)
#endif

#define reg_ata_rw_ctrl0___pio_hold___lsb 0
#define reg_ata_rw_ctrl0___pio_hold___width 6
#define reg_ata_rw_ctrl0___pio_strb___lsb 6
#define reg_ata_rw_ctrl0___pio_strb___width 6
#define reg_ata_rw_ctrl0___pio_setup___lsb 12
#define reg_ata_rw_ctrl0___pio_setup___width 6
#define reg_ata_rw_ctrl0___dma_hold___lsb 18
#define reg_ata_rw_ctrl0___dma_hold___width 6
#define reg_ata_rw_ctrl0___dma_strb___lsb 24
#define reg_ata_rw_ctrl0___dma_strb___width 6
#define reg_ata_rw_ctrl0___rst___lsb 30
#define reg_ata_rw_ctrl0___rst___width 1
#define reg_ata_rw_ctrl0___rst___bit 30
#define reg_ata_rw_ctrl0___en___lsb 31
#define reg_ata_rw_ctrl0___en___width 1
#define reg_ata_rw_ctrl0___en___bit 31
#define reg_ata_rw_ctrl0_offset 12

#define reg_ata_rw_ctrl1___udma_tcyc___lsb 0
#define reg_ata_rw_ctrl1___udma_tcyc___width 4
#define reg_ata_rw_ctrl1___udma_tdvs___lsb 4
#define reg_ata_rw_ctrl1___udma_tdvs___width 4
#define reg_ata_rw_ctrl1_offset 16

#define reg_ata_rw_ctrl2___data___lsb 0
#define reg_ata_rw_ctrl2___data___width 16
#define reg_ata_rw_ctrl2___dma_size___lsb 19
#define reg_ata_rw_ctrl2___dma_size___width 1
#define reg_ata_rw_ctrl2___dma_size___bit 19
#define reg_ata_rw_ctrl2___multi___lsb 20
#define reg_ata_rw_ctrl2___multi___width 1
#define reg_ata_rw_ctrl2___multi___bit 20
#define reg_ata_rw_ctrl2___hsh___lsb 21
#define reg_ata_rw_ctrl2___hsh___width 2
#define reg_ata_rw_ctrl2___trf_mode___lsb 23
#define reg_ata_rw_ctrl2___trf_mode___width 1
#define reg_ata_rw_ctrl2___trf_mode___bit 23
#define reg_ata_rw_ctrl2___rw___lsb 24
#define reg_ata_rw_ctrl2___rw___width 1
#define reg_ata_rw_ctrl2___rw___bit 24
#define reg_ata_rw_ctrl2___addr___lsb 25
#define reg_ata_rw_ctrl2___addr___width 3
#define reg_ata_rw_ctrl2___cs0___lsb 28
#define reg_ata_rw_ctrl2___cs0___width 1
#define reg_ata_rw_ctrl2___cs0___bit 28
#define reg_ata_rw_ctrl2___cs1___lsb 29
#define reg_ata_rw_ctrl2___cs1___width 1
#define reg_ata_rw_ctrl2___cs1___bit 29
#define reg_ata_rw_ctrl2___sel___lsb 30
#define reg_ata_rw_ctrl2___sel___width 2
#define reg_ata_rw_ctrl2_offset 0

#define reg_ata_rs_stat_data___data___lsb 0
#define reg_ata_rs_stat_data___data___width 16
#define reg_ata_rs_stat_data___dav___lsb 16
#define reg_ata_rs_stat_data___dav___width 1
#define reg_ata_rs_stat_data___dav___bit 16
#define reg_ata_rs_stat_data___busy___lsb 17
#define reg_ata_rs_stat_data___busy___width 1
#define reg_ata_rs_stat_data___busy___bit 17
#define reg_ata_rs_stat_data_offset 4

#define reg_ata_r_stat_data___data___lsb 0
#define reg_ata_r_stat_data___data___width 16
#define reg_ata_r_stat_data___dav___lsb 16
#define reg_ata_r_stat_data___dav___width 1
#define reg_ata_r_stat_data___dav___bit 16
#define reg_ata_r_stat_data___busy___lsb 17
#define reg_ata_r_stat_data___busy___width 1
#define reg_ata_r_stat_data___busy___bit 17
#define reg_ata_r_stat_data_offset 8

#define reg_ata_rw_trf_cnt___cnt___lsb 0
#define reg_ata_rw_trf_cnt___cnt___width 17
#define reg_ata_rw_trf_cnt_offset 20

#define reg_ata_r_stat_misc___crc___lsb 0
#define reg_ata_r_stat_misc___crc___width 16
#define reg_ata_r_stat_misc_offset 24

#define reg_ata_rw_intr_mask___bus0___lsb 0
#define reg_ata_rw_intr_mask___bus0___width 1
#define reg_ata_rw_intr_mask___bus0___bit 0
#define reg_ata_rw_intr_mask___bus1___lsb 1
#define reg_ata_rw_intr_mask___bus1___width 1
#define reg_ata_rw_intr_mask___bus1___bit 1
#define reg_ata_rw_intr_mask___bus2___lsb 2
#define reg_ata_rw_intr_mask___bus2___width 1
#define reg_ata_rw_intr_mask___bus2___bit 2
#define reg_ata_rw_intr_mask___bus3___lsb 3
#define reg_ata_rw_intr_mask___bus3___width 1
#define reg_ata_rw_intr_mask___bus3___bit 3
#define reg_ata_rw_intr_mask_offset 28

#define reg_ata_rw_ack_intr___bus0___lsb 0
#define reg_ata_rw_ack_intr___bus0___width 1
#define reg_ata_rw_ack_intr___bus0___bit 0
#define reg_ata_rw_ack_intr___bus1___lsb 1
#define reg_ata_rw_ack_intr___bus1___width 1
#define reg_ata_rw_ack_intr___bus1___bit 1
#define reg_ata_rw_ack_intr___bus2___lsb 2
#define reg_ata_rw_ack_intr___bus2___width 1
#define reg_ata_rw_ack_intr___bus2___bit 2
#define reg_ata_rw_ack_intr___bus3___lsb 3
#define reg_ata_rw_ack_intr___bus3___width 1
#define reg_ata_rw_ack_intr___bus3___bit 3
#define reg_ata_rw_ack_intr_offset 32

#define reg_ata_r_intr___bus0___lsb 0
#define reg_ata_r_intr___bus0___width 1
#define reg_ata_r_intr___bus0___bit 0
#define reg_ata_r_intr___bus1___lsb 1
#define reg_ata_r_intr___bus1___width 1
#define reg_ata_r_intr___bus1___bit 1
#define reg_ata_r_intr___bus2___lsb 2
#define reg_ata_r_intr___bus2___width 1
#define reg_ata_r_intr___bus2___bit 2
#define reg_ata_r_intr___bus3___lsb 3
#define reg_ata_r_intr___bus3___width 1
#define reg_ata_r_intr___bus3___bit 3
#define reg_ata_r_intr_offset 36

#define reg_ata_r_masked_intr___bus0___lsb 0
#define reg_ata_r_masked_intr___bus0___width 1
#define reg_ata_r_masked_intr___bus0___bit 0
#define reg_ata_r_masked_intr___bus1___lsb 1
#define reg_ata_r_masked_intr___bus1___width 1
#define reg_ata_r_masked_intr___bus1___bit 1
#define reg_ata_r_masked_intr___bus2___lsb 2
#define reg_ata_r_masked_intr___bus2___width 1
#define reg_ata_r_masked_intr___bus2___bit 2
#define reg_ata_r_masked_intr___bus3___lsb 3
#define reg_ata_r_masked_intr___bus3___width 1
#define reg_ata_r_masked_intr___bus3___bit 3
#define reg_ata_r_masked_intr_offset 40


#define regk_ata_active                           0x00000001
#define regk_ata_byte                             0x00000001
#define regk_ata_data                             0x00000001
#define regk_ata_dma                              0x00000001
#define regk_ata_inactive                         0x00000000
#define regk_ata_no                               0x00000000
#define regk_ata_nodata                           0x00000000
#define regk_ata_pio                              0x00000000
#define regk_ata_rd                               0x00000001
#define regk_ata_reg                              0x00000000
#define regk_ata_rw_ctrl0_default                 0x00000000
#define regk_ata_rw_ctrl2_default                 0x00000000
#define regk_ata_rw_intr_mask_default             0x00000000
#define regk_ata_udma                             0x00000002
#define regk_ata_word                             0x00000000
#define regk_ata_wr                               0x00000000
#define regk_ata_yes                              0x00000001
#endif 
