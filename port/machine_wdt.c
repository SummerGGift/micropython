/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 ChenYong (chenyong@rt-thread.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <string.h>

#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "modmachine.h"
#include "mphalport.h"

#ifdef MICROPYTHON_USING_MACHINE_WDT

#include <rtthread.h>
#include <drivers/watchdog.h>
#include "machine_wdt.h"

typedef struct _machine_wdt_obj_t {
    mp_obj_base_t base;
    rt_device_t wdt_device;
}machine_wdt_obj_t;

const mp_obj_type_t machine_wdt_type;

STATIC void error_check(bool status, const char *msg) {
    if (!status) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, msg));
    }
}

STATIC mp_obj_t machine_wdt_make_new(const mp_obj_type_t *type_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
#define MP_WDT_DEV_NAME "wdt"
    machine_wdt_obj_t *self = m_new_obj(machine_wdt_obj_t);
    rt_err_t result = RT_EOK;
    mp_int_t timeout = 5;

    // check arguments
    mp_arg_check_num(n_args, n_kw, 0, 1, false);

    if (n_args > 0) {
        timeout = mp_obj_get_int(args[0]);
        error_check(timeout >= 1, "input timeout value error");
    }

    self->base.type = &machine_wdt_type;
    // find WDT device
    self->wdt_device = rt_device_find(MP_WDT_DEV_NAME);
    if (self->wdt_device == RT_NULL || self->wdt_device->type != RT_Device_Class_Miscellaneous) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "WDT(%s) don't exist", MP_WDT_DEV_NAME)); 
    }

    result = rt_device_init(self->wdt_device);
    error_check(result == RT_EOK, "WDT init error");

    // set WDT device timout
    result = rt_device_control(self->wdt_device, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, (void *)&timeout);
    error_check(result == RT_EOK, "WDT set timout error");

    // return constant object
    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t machine_wdt_feed(mp_obj_t self_in) {
    /* idle task feed */
    machine_wdt_obj_t *self = MP_OBJ_TO_PTR(self_in);
    rt_err_t result = RT_EOK;

    result = rt_device_control(self->wdt_device, RT_DEVICE_CTRL_WDT_KEEPALIVE, RT_NULL);
    error_check(result == RT_EOK, "WDT feed failed");
    
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_wdt_feed_obj, machine_wdt_feed);

STATIC const mp_rom_map_elem_t machine_wdt_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_feed), MP_ROM_PTR(&machine_wdt_feed_obj) },
};
STATIC MP_DEFINE_CONST_DICT(machine_wdt_locals_dict, machine_wdt_locals_dict_table);

const mp_obj_type_t machine_wdt_type = {
    { &mp_type_type },
    .name = MP_QSTR_WDT,
    .make_new = machine_wdt_make_new,
    .locals_dict = (mp_obj_t) &machine_wdt_locals_dict,
};

#endif // MICROPYTHON_USING_MACHINE_WDT

