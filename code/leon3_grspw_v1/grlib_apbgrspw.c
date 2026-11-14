/* SPDX-License-Identifier: GPL-2.0-or-later */
/********************************************************************************
 *                                                                              *
 *    ▀▄   ▄▀                                                                   *
 *   ▄█▀███▀█▄    Created on: Jun 16, 2015                                      *
 *  █▀███████▀█   Author: Stephane GONZALEZ                                     *
 *  █ █▀▀▀▀▀█ █                                                                 *
 *     ▀▀ ▀▀                                                                    *
 *                                                                              *
 * Copyright (C) 2015-2026 Stephane GONZALEZ                                    *
 * Released under the GNU GPL v2 or later, see LICENSE                          *
 ********************************************************************************/

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/core/sysbus.h"
#include "hw/core/qdev-properties.h"
#include "hw/core/qdev-properties-system.h"
#include "chardev/char-fe.h"
#include "qemu/module.h"
#include "qom/object.h"

#include "grlib_apbgrspw.h"

#include "grlib_typesgrspw.h"
#include "grlib_reggrspw.h"
#include "grlib_loggrspw.h"
#include "grlib_coregrspw.h"

#define GRSPW_CORE_SIZE             (0x100)                                   ///< Size of the address window of one core
#define GRSPW_REG_SIZE              (0xFF)                                    ///< Register offset mask inside one core window
#define GRSPW_GET_COREID(addr)      ((t_grspw_id)((addr >> 8) & 0x3))       ///< Get core selection from address
#define GRSPW_GET_REGOFFSET(addr)   ((t_grspw_reg_offset)(addr & GRSPW_REG_SIZE))  ///< Get register selection from address
#define GRSPW_GET_CORE(addr,grspw)  ((t_grspw_coreptr)(&grspw->core[GRSPW_GET_COREID(addr)]))       ///< Get core selection
#define GRSPW_GET_GRSPWREG(addr,grspw)    ((t_grspw_regptr)&GRSPW_GET_CORE(addr,grspw)->device.grspw_reg)

OBJECT_DECLARE_SIMPLE_TYPE(GRSPW, GRLIB_APB_GRSPW)

struct GRSPW
{
  SysBusDevice parent_obj;

  MemoryRegion iomem;
  qemu_irq irq;
  t_grspw_core core[e_grspw_corenb];

  int32_t sys_freq_hz; // system frequency
  int nb_core; // number of spacewire core

};


static uint64_t grlib_apbgrspw_read(void *opaque, hwaddr addr, unsigned size)
{
  uint64_t value = 0;

  TRACELOGDBG("")
  // trace into the console for debug
  GRSPW *grspwptr = opaque;

  t_grspw_id  grspw_id = GRSPW_GET_COREID(addr);
  t_grspw_regptr grspw_regptr = GRSPW_GET_GRSPWREG(addr,grspwptr);
  t_grspw_reg_offset reg_offset = GRSPW_GET_REGOFFSET(addr); // get the selected register


  // action after reading a register : < DO NOTHING >
  value = (uint64_t) grlib_reggrspw_read_registers(grspw_id, grspw_regptr, reg_offset);
  return value;

}

static void grlib_apbgrspw_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
  TRACELOGDBG("")
// trace into the console for debug

  GRSPW *grspwptr = opaque;

  t_grspw_id  grspw_id = GRSPW_GET_COREID(addr);
  t_grspw_regptr grspw_regptr = GRSPW_GET_GRSPWREG(addr,grspwptr);
  t_grspw_reg_offset reg_offset = GRSPW_GET_REGOFFSET(addr); // get the selected register

  t_grsw_deviceptr deviceptr = &GRSPW_GET_CORE(addr,grspwptr)->device;
  boolean link_run = ((grspw_regptr->str & C_SPWSTR_LS) == C_SPWSTR_LSRUN) ? e_true : e_false;

  grlib_reggrspw_write_registers( grspw_id, grspw_regptr, reg_offset, (uint32) value);

  if (e_ctr == reg_offset && (value & C_SPWCTR_RS))
  {
    grlib_comgrspw_reset(deviceptr);
  }
  else if (link_run && (grspw_regptr->str & C_SPWSTR_LS) != C_SPWSTR_LSRUN)
  {
    // leaving the run state during a transmission is a link error [M 77.5.7]
    grlib_comgrspw_linkerror(deviceptr);
  }

  if (e_chn == reg_offset && (value & C_SPWCHN_AT))
  {
    // the pending packet is truncated and completed with LE [M 77.5.7]
    grlib_comgrspw_txabort(deviceptr);
  }

  grlib_comgrspw_rxflush(deviceptr);
  grlib_comgrspw_txenable(deviceptr);
}

static int grlib_apbgrspw_can_receive(void *opaque)
{
  return 0;
}

static void grlib_apbgrspw_receive(void *opaque, const uint8_t *data, int datalen)
{
}

static void grlib_apbgrspw_event(void *opaque, QEMUChrEvent event)
{
}

static const MemoryRegionOps grlib_apbgrspw_ops = {
    .write = grlib_apbgrspw_write,
    .read = grlib_apbgrspw_read,
    .endianness = DEVICE_BIG_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

static void grlib_apbgrspw_realize(DeviceState *dev, Error **errp)
{
  TRACELOGDBG("")
  GRSPW *grspw = GRLIB_APB_GRSPW(dev);
  SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
  t_grspw_id grspw_id;

  TRACELOG_OPEN()


  for (grspw_id = e_grspw_core1; grspw_id < e_grspw_corenb; grspw_id++)
  {
    t_grspw_coreptr core = &grspw->core[grspw_id];
    /* One IRQ line for each core */
    sysbus_init_irq(sbd, &core->device.irq);
    core->device.grspw_id = grspw_id;
    g_queue_init(&core->device.rx_pending);

    qemu_chr_fe_set_handlers(&core->chr, grlib_apbgrspw_can_receive,
                             grlib_apbgrspw_receive, grlib_apbgrspw_event,
                             NULL, grspw, NULL, true);

    // initialise registers to default value (XXX all un-define states {X=0} and all reserved bits are set to 0)
    t_grspw_regptr grspw_regptr = &core->device.grspw_reg;

    grlib_reggrspw_reset_registers(grspw_id, grspw_regptr);

  }

  sysbus_init_irq(sbd, &grspw->irq);

  memory_region_init_io(&grspw->iomem, OBJECT(grspw), &grlib_apbgrspw_ops, grspw, "grspw",
      (GRSPW_CORE_SIZE * e_grspw_corenb) );

  sysbus_init_mmio(sbd, &grspw->iomem);

  grlib_coregrspw_init(grspw->core);
}

static void grlib_apbgrspw_reset(DeviceState *d)
{
  TRACELOGDBG("")
}

static const Property grlib_apbgrspw_properties[] = {
    DEFINE_PROP_INT32("frequency", GRSPW, sys_freq_hz, 40000000),
    DEFINE_PROP_CHR("chrdev0", GRSPW, core[0].chr),
    DEFINE_PROP_CHR("chrdev1", GRSPW, core[1].chr),
    DEFINE_PROP_CHR("chrdev2", GRSPW, core[2].chr),
    DEFINE_PROP_CHR("chrdev3", GRSPW, core[3].chr),
};

static void grlib_apbgrspw_class_init(ObjectClass *klass, const void *data)
{
  TRACELOGDBG("")
  DeviceClass *dc = DEVICE_CLASS(klass);

  dc->realize = grlib_apbgrspw_realize;
  device_class_set_legacy_reset(dc, grlib_apbgrspw_reset);
  device_class_set_props(dc, grlib_apbgrspw_properties);
}

static const TypeInfo grlib_apbgrspw_info = {
    .name = TYPE_GRLIB_APB_GRSPW,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(GRSPW),
    .class_init = grlib_apbgrspw_class_init,
};

static void grlib_apbgrspw_register_types(void)
{
  TRACELOGDBG("")
  type_register_static(&grlib_apbgrspw_info);
}

type_init(grlib_apbgrspw_register_types)
